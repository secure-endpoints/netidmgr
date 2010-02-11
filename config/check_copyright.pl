# Copyright (c) 2009-2010 Secure Endpoints Inc.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


# Update copyright strings

# Usage: check-copyright.pl list-file.txt
# 
# Assumes that list-file.txt is a text file with one filename
# (relative path) per line.  The path will be assumed to be relative
# to the current directory.
# 
# For each path in list-file.txt, the script would attempt to locate
# the copyright notice and then update it to refer to the $targetyear.
# 
# The changes will be made in a temporary file named 'tempwork' in the
# current directory.  Once completed, the original file will be
# renamed to have .bak extension and 'tempwork' will be moved to
# original path.
# 
# Meanwhile, a file named no-copyright.txt will be created in the
# current directory that contains a list of files that were found to
# contain no copyright notices.

use File::Copy;

$targetyear = 2010;

open NC, '>', "no-copyright.txt" or die "Can't open no-copyright.txt";

sub check_for_copyright {

    my $found = 0;
    my $mitc;
    my $sec;
    my $extra;
    my $docopyright;
    my $y;
    my $modified = 0;

    $fn = shift;

    open F, '<', $fn or die "Can't open $fn\n";
    open T, '>', "tempwork" or die "Can't open temporary file\n";

    FLINE: while (<F>) {
        $docopyright = 0;
        $extra = "";

        if (/Copyright.*Massachusetts.*$/) {
            $mitc = $_;
            $sec = <F>;
            if ($sec =~ /Copyright.*Secure.*$/) {
                # MIT+SE
            } else {
                # MIT
                $extra = $sec;
                $sec = "";
            }
            $found = 1;
            $docopyright = 1;
        }

        if (/Copyright.*Secure.*$/) {
            $mitc = "";
            $sec = $_;
            # SE
            $found = 1;
            $docopyright = 1;
        }

        if ($docopyright) {
            print "[$fn] ";
            if ($sec eq "") {
                # MIT -> MIT+SE
                if ($mitc =~ /^(.*Copyright.*) \d+.*((?:by )?)Massachusetts Institute of Technology(.*)$/) {
                    print "+ $1 2006-$targetyear $2Secure Endpoints Inc.$3\n";
                    print T $mitc;
                    print T "$1 2006-$targetyear $2Secure Endpoints Inc.$3\n";
                    if ($extra ne "") {
                        print T $extra;
                    }
                    $modified = 1;
                } else {
                    print "MIT->MIT+SE NOT FOUND: [$mitc]\n";
                    print T $mitc;
                    if ($extra ne "") {
                        print T $extra;
                    }
                }
            } else {
                # SE -> SE
                if ($mitc ne "") {
                    print T $mitc;
                }

                if ($sec =~ /^(.*Copyright \(c\))\s*(\d+)\s+((?:by )?)(Secure.*)$/) {
                    $y = int($2);
                    if ($y < $targetyear) {
                        print "! $1 $2-$targetyear $3$4\n";
                        print T "$1 $2-$targetyear $3$4\n";
                        $modified = 1;
                    } else {
                        print "= No change\n";
                        print T $sec;
                    }
                } elsif ($sec =~ /^(.*Copyright.*),\s*(\d+)\s+((?:by )?)(Secure.*)$/) {
                    $y = int($2);
                    if ($y < $targetyear) {
                        print "! $1, $2";
                        for ($i = $y + 1; $i <= $targetyear; $i++) {
                            print ", $i";
                        }
                        print " $3$4\n";

                        print T "$1, $2";
                        for ($i = $y + 1; $i <= $targetyear; $i++) {
                            print T ", $i";
                        }
                        print T " $3$4\n";
                        $modified = 1;
                    } else {
                        print "= No change\n";
                        print T $sec;
                    }
                } elsif ($sec =~ /^(.*Copyright.*)(\d+)-(\d+)\s+((?:by )?)(Secure.*)$/) {
                    $y = int($3);
                    if ($y < $targetyear) {
                        print "! $1$2-$targetyear $4$5\n";
                        print T "$1$2-$targetyear $4$5\n";
                        $modified = 1;
                    } else {
                        print "= No change\n";
                        print T $sec;
                    }
                }  else {
                    print "Can't find pattern! [$sec]\n";
                    print T $sec;
                }
            }
        } else {
            print T $_;
        }
    }

    close T;
    close F;

    if ($found) {
        if ($modified) {
            move ($fn, $fn.".bak") or die "Can't move $fn to $fn.bak\n";
            move ("tempwork",$fn) or die "Can't move tempwork to $fn\n";
            #move("tempwork",$fn.".modified") or die "Can't move to $fn\n";
        }
    } else {
        print NC $fn."\n";
    }
}

$filelist = shift;

open FL, '<', $filelist or die "Can't open $filelist";

LOOP: while (<FL>) {

    next LOOP if /^$/;

    s/\s*$//;
    s/^\s*//;

    $nextfile = $_;

    check_for_copyright($nextfile);
}

close FL;
close NC;
