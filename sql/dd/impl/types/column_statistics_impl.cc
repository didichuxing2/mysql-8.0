/* Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "dd/impl/types/column_statistics_impl.h"

#include <stdint.h>

#include "current_thd.h"                   // current_thd
#include "dd/impl/dictionary_impl.h"       // Dictionary_impl
#include "dd/impl/raw/object_keys.h"       // id_key_type
#include "dd/impl/raw/raw_record.h"        // Raw_record
#include "dd/impl/sdi_impl.h"              // sdi read/write functions
#include "dd/impl/tables/column_statistics.h"   // Column_statistics
#include "dd/impl/transaction_impl.h"      // Open_dictionary_tables_ctx
#include "histograms/histogram.h"          // histograms::Histogram
#include "include/my_md5.h"                // array_to_hex
#include "include/sha1.h"                  // compute_sha1_hash
#include "json_dom.h"                      // Json_*
#include "m_string.h"                      // STRING_WITH_LEN
#include "mysqld.h"                        // system_charset_info
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"

namespace dd {

const Entity_object_table &Column_statistics::OBJECT_TABLE()
{
  return dd::tables::Column_statistics::instance();
}

///////////////////////////////////////////////////////////////////////////

const Object_type &Column_statistics::TYPE()
{
  static Column_statistics_type s_instance;
  return s_instance;
}

///////////////////////////////////////////////////////////////////////////

String_type Column_statistics::create_name(const String_type &schema_name,
                                           const String_type &table_name,
                                           const String_type &column_name)
{
  String_type output;

  output.assign(schema_name);
  output.push_back('\037');
  output.append(table_name);
  output.push_back('\037');

  /*
    Column names are always case insensitive, so convert it to lowercase.
    Lookups in the dictionary is always done using the name, so this should
    ensure that we always get back our object.
  */
  DBUG_ASSERT(column_name.length() <= NAME_LEN);
  char lowercase_name[NAME_LEN + 1]; // Max column length name + \0
  memcpy(lowercase_name, column_name.c_str(), column_name.length() + 1);
  my_casedn_str(system_charset_info, lowercase_name);
  output.append(lowercase_name, column_name.length());

  return output;
}

///////////////////////////////////////////////////////////////////////////

String_type Column_statistics::create_mdl_key(const String_type &schema_name,
                                              const String_type &table_name,
                                              const String_type &column_name)
{
  String_type name= Column_statistics::create_name(schema_name, table_name,
                                                  column_name);

  // Temporary buffer to store 160bit digest.
  uint8 digest[SHA1_HASH_SIZE];
  compute_sha1_hash(digest, name.data(), name.length());

  char output[SHA1_HASH_SIZE * 2];
  array_to_hex(output, digest, SHA1_HASH_SIZE);
  return String_type(output, SHA1_HASH_SIZE * 2);
}

///////////////////////////////////////////////////////////////////////////
// Column_statistics_impl implementation.
///////////////////////////////////////////////////////////////////////////

bool Column_statistics_impl::restore_attributes(const Raw_record &r)
{
  restore_id(r, dd::tables::Column_statistics::FIELD_ID);
  restore_name(r, dd::tables::Column_statistics::FIELD_NAME);

  m_schema_name= r.read_str(dd::tables::Column_statistics::FIELD_SCHEMA_NAME);
  m_table_name= r.read_str(dd::tables::Column_statistics::FIELD_TABLE_NAME);
  m_column_name= r.read_str(dd::tables::Column_statistics::FIELD_COLUMN_NAME);

  Json_wrapper wrapper;
  if (r.read_json(dd::tables::Column_statistics::FIELD_HISTOGRAM, &wrapper))
    return true;  /* purecov: deadcode */

  Json_dom *json_dom= wrapper.to_dom(current_thd);
  if (json_dom->json_type() != enum_json_type::J_OBJECT)
    return true;  /* purecov: deadcode */

  const Json_object *json_object= down_cast<const Json_object*>(json_dom);
  m_histogram=
    histograms::Histogram::json_to_histogram(&m_mem_root,
                                             { m_schema_name.data(),
                                               m_schema_name.size() },
                                             { m_table_name.data(),
                                               m_table_name.size() },
                                             { m_column_name.data(),
                                               m_column_name.size() },
                                             *json_object);
  if (m_histogram == nullptr)
    return true;  /* purecov: deadcode */
  return false;
}

///////////////////////////////////////////////////////////////////////////

bool Column_statistics_impl::store_attributes(Raw_record *r)
{
  Json_object json_object;
  m_histogram->histogram_to_json(&json_object);

  Json_wrapper wrapper(&json_object);
  wrapper.set_alias();

  return store_id(r, dd::tables::Column_statistics::FIELD_ID) ||
         store_name(r, dd::tables::Column_statistics::FIELD_NAME) ||
         r->store(dd::tables::Column_statistics::FIELD_CATALOG_ID,
                  Dictionary_impl::instance()->default_catalog_id()) ||
         r->store(dd::tables::Column_statistics::FIELD_SCHEMA_NAME,
                  m_schema_name) ||
         r->store(dd::tables::Column_statistics::FIELD_TABLE_NAME,
                  m_table_name) ||
         r->store(dd::tables::Column_statistics::FIELD_COLUMN_NAME,
                  m_column_name) ||
         r->store_json(dd::tables::Column_statistics::FIELD_HISTOGRAM, wrapper);
}

///////////////////////////////////////////////////////////////////////////

void Column_statistics_impl::serialize(Sdi_wcontext *wctx, Sdi_writer *w) const
{
  /*
    We only write metadata about column statistics, which includes:
    - schema name
    - table name
    - column name
    - number of buckets originally specified by the user
  */
  w->StartObject();
  Entity_object_impl::serialize(wctx, w);
  write(w, m_schema_name, STRING_WITH_LEN("schema_name"));
  write(w, m_table_name, STRING_WITH_LEN("table_name"));
  write(w, m_column_name, STRING_WITH_LEN("column_name"));
  write(w, m_histogram->get_num_buckets_specified(),
        STRING_WITH_LEN("number_of_buckets_specified"));
  w->EndObject();
}

///////////////////////////////////////////////////////////////////////////

bool Column_statistics_impl::deserialize(Sdi_rcontext *rctx,
                                         const RJ_Value &val)
{
  Entity_object_impl::deserialize(rctx, val);
  read(&m_schema_name, val, "schema_name");
  read(&m_table_name, val, "table_name");
  read(&m_column_name, val, "column_name");

  /*
    TODO: Re-create histogram data. This will be done in a later worklog, when
    we actually need to use the histogram data.
  */
  return false;
}

///////////////////////////////////////////////////////////////////////////
bool Column_statistics::update_id_key(id_key_type *key, Object_id id)
{
  key->update(id);
  return false;
}

///////////////////////////////////////////////////////////////////////////

bool Column_statistics::update_name_key(name_key_type *key,
                                        const String_type &name)
{
  return dd::tables::Column_statistics::update_object_key(
    key, Dictionary_impl::instance()->default_catalog_id(), name);
}

///////////////////////////////////////////////////////////////////////////
// Column_statistics_type implementation.
///////////////////////////////////////////////////////////////////////////

void
Column_statistics_type::register_tables(Open_dictionary_tables_ctx *otx) const
{
  otx->add_table<dd::tables::Column_statistics>();
}

///////////////////////////////////////////////////////////////////////////

}
