// Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, 51 Franklin
// Street, Suite 500, Boston, MA 02110-1335 USA.

/// @file
///
/// This file implements the covered_by functor and mbr_covered_by function.

#include "covered_by_functor.h"
#include "relops.h"

#include <boost/geometry.hpp>

#include "box.h"
#include "box_traits.h"
#include "dd/types/spatial_reference_system.h"  // dd::Spatial_reference_system
#include "geometries.h"
#include "geometries_traits.h"
#include "mbr_utils.h"
#include "sql_exception_handler.h"  // handle_gis_exception

namespace bg = boost::geometry;

namespace gis {

Covered_by::Covered_by(double semi_major, double semi_minor)
    : m_semi_major(semi_major), m_semi_minor(semi_minor) {}

bool Covered_by::operator()(const Geometry *g1, const Geometry *g2) const {
  return apply(*this, g1, g2);
}

bool Covered_by::operator()(const Box *b1, const Box *b2) const {
  DBUG_ASSERT(b1->coordinate_system() == b2->coordinate_system());
  switch (b1->coordinate_system()) {
    case Coordinate_system::kCartesian:
      return eval(down_cast<const Cartesian_box *>(b1),
                  down_cast<const Cartesian_box *>(b2));
    case Coordinate_system::kGeographic:
      return eval(down_cast<const Geographic_box *>(b1),
                  down_cast<const Geographic_box *>(b2));
  }

  DBUG_ASSERT(false);
  return false;
}

bool Covered_by::eval(const Geometry *g1, const Geometry *g2) const {
  // Currently only implemented for boxes (MBRs).
  DBUG_ASSERT(false);
  throw not_implemented_exception(g1->coordinate_system(), g1->type(),
                                  g2->type());
}

//////////////////////////////////////////////////////////////////////////////

// covered_by(Box, Box)

bool Covered_by::eval(const Cartesian_box *b1, const Cartesian_box *b2) const {
  return bg::covered_by(*b1, *b2);
}

bool Covered_by::eval(const Geographic_box *b1,
                      const Geographic_box *b2) const {
  return bg::covered_by(*b1, *b2);
}

//////////////////////////////////////////////////////////////////////////////

bool mbr_covered_by(const dd::Spatial_reference_system *srs, const Geometry *g1,
                    const Geometry *g2, const char *func_name, bool *covered_by,
                    bool *null) noexcept {
  try {
    DBUG_ASSERT(g1->coordinate_system() == g2->coordinate_system());
    DBUG_ASSERT(srs == nullptr ||
                ((srs->is_cartesian() &&
                  g1->coordinate_system() == Coordinate_system::kCartesian) ||
                 (srs->is_geographic() &&
                  g1->coordinate_system() == Coordinate_system::kGeographic)));

    if ((*null = (g1->is_empty() || g2->is_empty()))) return false;

    Covered_by covered_by_func(srs ? srs->semi_major_axis() : 0.0,
                               srs ? srs->semi_minor_axis() : 0.0);

    switch (g1->coordinate_system()) {
      case Coordinate_system::kCartesian: {
        Cartesian_box mbr1;
        box_envelope(g1, srs, &mbr1);
        Cartesian_box mbr2;
        box_envelope(g2, srs, &mbr2);
        *covered_by = covered_by_func(&mbr1, &mbr2);
        break;
      }
      case Coordinate_system::kGeographic: {
        Geographic_box mbr1;
        box_envelope(g1, srs, &mbr1);
        Geographic_box mbr2;
        box_envelope(g2, srs, &mbr2);
        *covered_by = covered_by_func(&mbr1, &mbr2);
        break;
      }
    }
  } catch (...) {
    handle_gis_exception(func_name);
    return true;
  }

  return false;
}

}  // namespace gis
