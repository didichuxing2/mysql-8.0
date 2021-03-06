# This include file needs two variables to be set
# IBD_FILE		The ibd file to be used by tool
# DUMP_FILE		The output file generated from tool
# SKIP_LARGE_DATA_FILE	The file prepended with ">" to redirect large output from SDI
#			For example: --let SKIP_LARGE_DATA_FILE = > $MYSQLD_VARDIR/log/ibd2sdi-large.err

--echo # 1. Read from copy 0 with long option
--exec $IBD2SDI --read=0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 2. Read from copy 1 with short option
--exec $IBD2SDI -r 1 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 3. Read from copy 0 with specific id & type (long option)
--exec $IBD2SDI -r0 --id=3 --type=3 $IBD_FILE 2>&1

--echo # 4. Read from copy 1 with specific id & type (short option)
--exec $IBD2SDI -r1 -i 4 -t 4 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 5. Read from copy 0 & skip data
--exec $IBD2SDI -r0 --skip-data $IBD_FILE $SKIP_LARGE_DATA_FILE
--exec $IBD2SDI -r1 -s $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 6. Read from copy 0 and dump into FILE
--exec $IBD2SDI -r0 --dump-file=$DUMP_FILE $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
if (!$SKIP_LARGE_DATA_FILE) {
--cat_file $DUMP_FILE
}

--echo # 7. Read from copy 0 and dump into FILE using short option
--exec $IBD2SDI -r0 -s -d $DUMP_FILE  $IBD_FILE 2>&1
if (!$SKIP_LARGE_DATA_FILE) {
--cat_file $DUMP_FILE
}

--echo # 8. Print all records matching type (short option)
--exec $IBD2SDI -r0 -t 0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 9. Print all records matching id (long option)
--exec $IBD2SDI -r1 --id=3 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 10. Print version
--exec $IBD2SDI -v > $MYSQLTEST_VARDIR/tmp/ver_output

perl;
use strict;
use warnings;
use File::Copy;
my $dir = $ENV{'MYSQLTEST_VARDIR'};
my $in_file= $dir ."/tmp/ver_output";
# open file in write mode
open (IN_FILE, "<", $in_file) or die $!;

while(<IN_FILE>)
{
   s/^(.*)ibd2sdi(\s+)(Ver\s\d\.\d\.\d+(-.*))(\s+for.*)$/IBD2SDI Ver #.#.#/;
   print $_;
}
close(IN_FILE);
EOF
--remove_file $MYSQLTEST_VARDIR/tmp/ver_output

--echo # 11. Print help
--exec $IBD2SDI --help > $MYSQLTEST_VARDIR/tmp/help_output

perl;
use strict;
use warnings;
use File::Copy;
my $dir = $ENV{'MYSQLTEST_VARDIR'};
my $in_file= $dir ."/tmp/help_output";
# open file in write mode
open (IN_FILE, "<", $in_file) or die $!;

while(<IN_FILE>)
{
   # Replacing path in Version output
   s/^(.*)ibd2sdi(\s+)(Ver\s\d\.\d\.\d+(-.*))(\s+for.*)$/IBD2SDI Ver #.#.#/;
   # Replacing path in Other lines
   s/^Usage:(\s+.*)ibd2sdi(.*)$/IBD2SDI$2/;
   s/(Copyright\s\(c\))\s([0-9]*),\s([0-9]*)(.*)/$1 YEAR, YEAR$4/g;

   unless ($_=~ /^.*de?bug.*$/) {
     print $_;
   }
}
close(IN_FILE);
EOF
--remove_file $MYSQLTEST_VARDIR/tmp/help_output

--echo # 12. Print all records
--exec $IBD2SDI $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 13. Retrieve explicit id & type
--exec $IBD2SDI --id=3 --type=3 $IBD_FILE 2>&1

--echo # 14. Skip data and retrieve
--exec $IBD2SDI --skip-data $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 15. Dump all records into outfile
--exec $IBD2SDI --dump-file=$DUMP_FILE $IBD_FILE
if (!$SKIP_LARGE_DATA_FILE) {
--cat_file $DUMP_FILE
}

--echo # 16. Print all records matching type (short option)
--exec $IBD2SDI -t 0 $IBD_FILE 2>&1

--echo # 17. Print all records matching id (long option)
--exec $IBD2SDI --id=3 $IBD_FILE 2>&1

--echo # 18. Pass --no-check & --strict-check together
--error 1
--exec $IBD2SDI --no-check --strict-check="crc32" $IBD_FILE 2>&1

--echo # 19. Test for reading form more than one copy example (invalid): -r 0,1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --read=0,1 $IBD_FILE 2>&1

--echo # 20. Test for Test for invalid values for -r or --read like

--echo # 20.1 non numeric/Alphanumeric
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --read=a $IBD_FILE  2>&1

--echo # 20.2 numbers with +/-
--exec $IBD2SDI --read=+0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --read=-0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --read=+1 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --read=-1 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --read=-15 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --read=+16 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 20.3 with special characters
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --read=@ $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI -r # $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --type=@ $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI -t # $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI -t = $IBD_FILE 2>&1

--echo # 20.4 with no permitted values
--exec $IBD2SDI  --read=14 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI  --read=-14 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 20.5 with very long value
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI  --read=14324932470234893204293032 $IBD_FILE 2>&1

--echo # 20.6 with blank value
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --read= 2>&1

--echo # 20.7 value with spaces
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --read=   1 $IBD2SDI 2>&1

--echo # 21. With misspelled --read
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --red=0 $IBD2SDI 2>&1

--echo # 22. Test --type option with

--echo # 22.1 non numeric/Alphanumeric
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --type=a $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI -t b $IBD_FILE 2>&1

--echo # 22.2 numbers with +/-
--exec $IBD2SDI --type=+0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --type=-0 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --type=+1 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --type=-1 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --type=-15 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI --type=+16 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 22.4 with no permitted values
--exec $IBD2SDI  --type=14 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1
--exec $IBD2SDI  --type=-14 $IBD_FILE $SKIP_LARGE_DATA_FILE 2>&1

--echo # 22.5 with very long value
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI  --type=14324932470234893204293032 $IBD_FILE 2>&1

--echo # 22.6 with blank value
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --type= $IBD_FILE 2>&1

--echo # 22.7 value with spaces
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --type=   1 $IBD2SDI 2>&1

--echo # 23. Test for reading form more than type example (invalid): -t 0,1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --type=0,1 $IBD_FILE 2>&1
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI -t 2,3  $IBD_FILE 2>&1

--echo # 24. With misspelled --type
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI --tpe=0 $IBD2SDI  2>&1

--echo # 25. With Invalid file types
--echo # 25.1 With non IBD file

--write_file $MYSQL_TMP_DIR/invalid_file.txt
test
EOF
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI $MYSQL_TMP_DIR/invalid_file.txt 2>&1
--remove_file $MYSQL_TMP_DIR/invalid_file.txt

--echo # 25.2 with non existant file
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI non_existant_file.txt 2>&1

--echo # 25.3 with empty file

--write_file $MYSQL_TMP_DIR/empty_file.txt
EOF
--replace_result $IBD2SDI IBD2SDI
--error 1
--exec $IBD2SDI $MYSQL_TMP_DIR/empty_file.txt 2>&1
--remove_file $MYSQL_TMP_DIR/empty_file.txt
