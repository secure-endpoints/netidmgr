# Copyright (c) 2010 Secure Endpoints Inc.
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

print "Source file,Line,Allocation #,Size,Location,Data\n";
while (<>) {
    if (m/^Dumping objects ->$/) {
        MEMDUMP: while(<>) {
            if (m/(.*)\((\d*)\) : \{(\d*)\} normal block at (.*), (\d*) bytes long./) {
                $data = <>;
                print "$1,$2,$3,$5,$4,";
                $data =~ s/\s*$//;
                $data =~ s/\"/\?/g;
                print "\"$data\"\n";
            } elsif(m/(.*)\((\d*)\) : \{(\d*)\} client block at (.*), subtype (\d)*, (\d*) bytes long./) {
                $data = <>;
                print "$1,$2,$3,$6,$4,";
                $data =~ s/\s*$//;
                $data =~ s/\"/\?/g;
                print "\"$data\"\n";
            } elsif(m/\{(\d*)\} normal block at (.*), (\d*) bytes long./) {
                $data = <>;
                print "UNKNOWN,000,$1,$3,$2,";
                $data =~ s/\s*$//;
                $data =~ s/\"/\?/g;
                print "\"$data\"\n";
            } elsif(m/Object dump complete/) {
                last MEMDUMP;
            } else {
                die "Can't parse input: $_";
            }
        }
    }
}
