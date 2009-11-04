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
