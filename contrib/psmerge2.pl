#! /bin/sh --
eval '(exit $?0)' && eval 'PERL_BADLANG=x;export PERL_BADLANG;: \
;exec perl -x -S -- "$0" ${1+"$@"};#'if 0;
eval 'setenv PERL_BADLANG x;exec perl -x -S -- "$0" $argv:q;#'.q
#!perl -w
+($0=~/(.*)/s);do$1;die$@if$@;__END__+if 0;
# Don't touch/remove lines 1--7: http://www.inf.bme.hu/~pts/Magic.Perl.Header

#
# psmerge2.pl v0.01 -- merge (concatenate) PostScript documents
# by pts@math.bme.hu at Tue Dec 17 13:24:13 CET 2002
# -- Tue Dec 17 13:51:20 CET 2002
#
# This program has several limitations, including:
# -- it may fail when any of the source files contains binary data
# -- it may run out of memory if an input file contains long lines
# -- keeps the %!PS-Adobe, %%LanguageLevel, %%BoundingBox, %%Pages: etc.
#    Adobe DSC comments from the first input file; ignores other such comments
#    in subsequent input files
# -- keeps fonts, resources and procsets only from the first input file;
#    ignores these elements of subsequent input files
# -- silengtly removes all %%Trailer sections
#

use integer;
use strict;


sub usage($) {
  die "$_[0]Usage: $0 [-oOUTFILE.ps] [--] INFILE.ps [...]\n"
}

my $outfn;
my $stdin_ok=1;

{ my $I;
  for ($I=0; $I<@ARGV; $I++) {
    if ($ARGV[$I] eq '-') { last }
    elsif ($ARGV[$I] eq '--') { $I++; $stdin_ok=0; last }
    elsif ($ARGV[$I] eq '-o' and $I<$#ARGV) { $outfn=$ARGV[++$I] }
    elsif (substr($ARGV[$I],0,2) eq '-o') { $outfn=substr($ARGV[$I],2) }
    elsif (substr($ARGV[$I],0,1) eq '-') { usage("error: unknown switch: $ARGV[$I]\n") }
    else { last }
  }
  splice @ARGV, 0, $I;
}
usage("error: no input files\n") if !@ARGV;
die "$0: open out $outfn: $!\n" if defined($outfn) and !open STDOUT, "> $outfn";

#** Are we reading the first input file?
my $is_first=1;
#** Are we skipping, looking for %%Page comments?
my $is_skipping=0;
#** The page number we are scanning
my $pagec=0;

for my $infn (@ARGV) {
  die "$0: open $infn: $!\n" if! open F, ($infn eq '-' && $stdin_ok ? "<&STDIN" : "< $infn");
  while (<F>) {
    if (/^%!/) {
      if ($is_first) { print } else { $is_skipping=1 }
    } elsif (/^%%Pages:\s*(\d+)\s*override/i) {
      if ($is_first) { print "%%Pages: $1\n" } else { $is_skipping=1 }
    } elsif (/^%%Pages:/i) { # ignore this
    } elsif (/^%%Page:/i) {
      $pagec++;
      print "%%Page: $pagec $pagec\n";
      $is_skipping=0
    } elsif (/^%%(Trailer|EOF)\b/i) {
      $is_skipping=1
    } else {
      print if !$is_skipping;
    }
  } ## WHILE
  close F;
  $is_first=0;
}
print "%%EOF\n";

__END__
