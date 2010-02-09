#!/usr/bin/perl

#
# Copyright (c) 2007-2009 Secure Endpoints Inc.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
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
#

use Text::ParseWords;

if ($#ARGV < 1 || $#ARGV > 2) {
    print "Usage: schema2html.pl <schema-filename> <output-filename> [<tags>]\n";
    die;
}

$schemafile = $ARGV[0];
$outputfile = $ARGV[1];

@regpath = ( "HKCU|HKLM", "Software", "MIT", "NetIDMgr" );
@dodocs = ( 0, 0, 0, 1 );
@allowtags = ();

if ($#ARGV == 2) {
    my $tags = $ARGV[2];

    @allowtags = split(',',$tags);
}

$regrootpos = $#regpath;

# Called with one string parameter containing a documentation
# string. This string may contain a tag prefix indicating which tags
# are applicable to the documentation.  Returns two strings, one
# contains a list of tab separated tags and the other contains the
# documentation sans the tag prefix.

sub parse_doc_string
{ my $s = shift; my $t, $d;

    if ($s =~ /\[\S*\]/) {
        ($t, $d) = ($s =~ m/\[(\S*)\](.*)/);
        chomp $t;
        chomp $d;

        $d =~ s/^\s+//;

        return ($t, $d);
    } else {
        chomp $s;

        return ("", $s);
    }
}

sub show_this_element {
    my $t = shift;
    my @tags = split(',', $t);

    return 0 if ($dodocs[$#dodocs] == 0);

    chomp $t;

    return 1 if ($#tags < 0);

    for my $tag (@tags) {
        for my $allowtag (@allowtags) {
            return 1 if ($allowtag eq $tag);
        }
    }
    return 0;
}

$skip_lines = 1;

open(IN, "<".$schemafile) or die "Can't open input file:".$schemafile;
open(OUT, ">".$outputfile) or die "Can't open output file:".$outputfile;

#

$thispath = "<Undefined>";
$thisline = 0;

$anchor_found = 0;
$header_done = 0;

# parameters for templates

$scope = "";
$space_name = "";
$regpath_string = "";
$schema_doc = "";

$value_name = "";
$value_type = "";
$value_def = "";
$value_doc = "";

# templates;

$tmpl_header = <<'T_HEADER';
<html>
<head>
  <title>Registry Settings for $scope</title>
  <link rel="stylesheet" type="text/css" href="nidmgr.css">
</head>

<body>

<h1>Registry Settings for $scope</h1>

<p>
  This document describes the registry settings for $scope.
  Configuration information can be maintained in both the user and
  machine registry hives.  If a single setting is defined in both the
  user and machine hives, the user setting will override the machine
  setting.
</p>
T_HEADER
    ;

$tmpl_footer = <<'T_FOOTER';
</body>
</html>
T_FOOTER
    ;

$tmpl_space_begin = <<'T_SP_BEGIN';
<div class="schema-space">
  <p class="schema-space">$space_name</p>

  <p class="schema-reg">
    <span class="schema-label">Registry path:</span>
    <span class="schema-value-pre">$regpath_string</span>
  </p>

  <div class="schema-space-doc">$schema_doc</div>
T_SP_BEGIN
    ;

$tmpl_space_end = <<'T_SP_END';
</div>
T_SP_END
    ;

$tmpl_space_s_begin = <<'T_SPS_BEGIN';
<div class="schema-space schema-templ">
  <p class="schema-space">(Schema)</p>

  <p class="schema-reg">
    <span class="schema-label">Registry path:</span>
    <span class="schema-value-pre">$regpath_string</span>
  </p>

  <div class="schema-space-doc">$schema_doc</div>

T_SPS_BEGIN
    ;

$tmpl_space_s_end = <<'T_SPS_END';
</div>
T_SPS_END
    ;

$tmpl_value = <<'T_VALUE';
  <div class="schema-value">

    <p class="schema-value-name"><span class="schema-value-name">$value_name</span></p>

    <p class="schema-value-type">
      <span class="schema-label">Type:</span>
      <span class="schema-value pre">$value_type</span>
    </p>

    <p class="schema-value-def">
      <span class="schema-label">Default:</span>
      <span class="schema-value">$value_def</span>
    </p>

    <div class="schema-value-doc">
    $value_doc
    </div>
  </div>
T_VALUE
    ;

while(<IN>) {

    $thisline ++;

    chomp $_;

    if (m/^\#\!(\w+)=(.*)/) {
        ($op, $val) = m/^\#\!(\w+)=(.*)/;

      SWITCH: for ($op) {
          /ANCHOR/ && do {
              my @spaces = split('\\\\', $val);
              my $first = shift(@spaces);

              die "First token of an ANCHOR must be ROOT at line $thisline\n" unless $first eq "ROOT";

              push @regpath, @spaces;
              $anchor_found = 1;
              $regrootpos = $#regpath;
              last;
          };

          /SCOPE/ && do {
              $scope = $val;

              last;
          };

          die "Unknown operator [".$op."] at line $thisline\n";
      }
    } elsif (m/^\#/) {
# ignore
    } elsif ($skip_lines > 0) {
        $skip_lines--;
    } else {

        die "The file must specify an ANCHOR operator before spaces are defined." if $anchor_found == 0;

        if ($header_done == 0) {
            my $t;

            eval "\$t = <<E;\n$tmpl_header\nE";

            print OUT $t;
            $header_done = 1;
        }

        @fields = &parse_line(',',0,$_);
        for (@fields) {
            chomp;
            s/^\s*//;
        }

      SWITCH:
        for ($fields[1]) {
            /KC_SPACE/ && do {

                my $doc, $tags, $show;

                ($tags, $doc) = parse_doc_string($fields[3]);

                push @regpath, $fields[0];
                $show = show_this_element($tags);
                push @dodocs, $show;

                $thispath = join ('\\', @regpath);

                $schema_doc = $doc;
                $space_name = $fields[0];
                $regpath_string = $thispath;

                if ($show) {
                    if ($fields[0] eq "_Schema") {
                        my $t;

                        eval "\$t = <<E;\n$tmpl_space_s_begin\nE";

                        print OUT $t;

                    } else {
                        my $t;

                        eval "\$t = <<E;\n$tmpl_space_begin\nE";

                        print OUT $t;
                    }
                }

                last;
            };

            /KC_ENDSPACE/ && do {
                my $thisspace = pop @regpath;
                my $show = pop @dodocs;

                if ($show) {

                    if ($thisspace eq "_Schema") {

                        print OUT $tmpl_space_s_end;

                    } else {

                        print OUT $tmpl_space_end;

                    }

                }

                last;
            };

            my $data_type = "<undefined>";
            my $doc, $tags;

            ($tags, $doc) = parse_doc_string($fields[3]);

            if (show_this_element($tags)) {

                /KC_INT32/ && do {$data_type = "REG_DWORD";};
                /KC_STRING/ && do {$data_type = "REG_SZ";};
                /KC_INT64/ && do {$data_type = "REG_QWORD";};
                /KC_BINARY/ && do {$data_type = "REG_BINARY";};

                die "Unknown type $fields[1] at line $thisline\n" if $data_type eq "<undefined>";

                $value_name = $fields[0];
                $value_type = $data_type;
                $value_def = $fields[2];
                $value_doc = $doc;

                {
                    my $t;

                    eval "\$t = <<E;\n$tmpl_value\nE";

                    print OUT $t;
                }
            }
        }
    }
}

print OUT $tmpl_footer;

die "Configuration spaces don't match up." if $#regpath != $regrootpos;

close(IN);
close(OUT);
