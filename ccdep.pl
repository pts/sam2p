#! /bin/sh --
eval '(exit $?0)' && eval 'PERL_BADLANG=x;export PERL_BADLANG;: \
;exec perl -T -S -- "$0" ${1+"$@"};#'if 0;
eval 'setenv PERL_BADLANG x;exec perl -x -S -- "$0" $argv:q;#'.q
#!perl -w
+($0=~/(.*)/s);do$1;die$@if$@;__END__+if 0;
# Don't touch/remove lines 1--7: http://www.inf.bme.hu/~pts/Magic.Perl.Header
#
# ccdep.pl v0.32 -- semiautomatic dependency discovery for C/C++ programs
# by pts@math.bme.hu at Fri May 31 13:36:29 CEST 2002
# 0.31 by pts@math.bme.hu at Sat Jun  1 15:19:55 CEST 2002
# 0.32 by pts@math.bme.hu at Tue Sep  3 19:12:20 CEST 2002
# 0.33 by pts@math.bme.hu at Thu Oct 31 09:47:25 CET 2002
#
# Dat: no -T (tainting checks) anymore, does bad to our readpipe()
# OK : emit `TARGETS_yes = ...'
# Imp: make #warning etc. in *.h files work as expected
# OK : generated.h
# Imp: add external libraries (-L...)
# Imp: abort if gcc command not found...
# Imp: abort if not all .o are mentioned in gcc output
# Imp: avoid $ etc. in Makefile
# OK : all 8 combinations of PROVIDES|REQUIRES|CONFLICTS
#
BEGIN { eval { require integer; import integer } }
BEGIN { eval { require strict ; import strict  } }

# Typical invocation: ccdep.pl --FAL=assert,no,yes,checker $(CXX)
my @FAL=();
if (@ARGV and $ARGV[0]=~/\A--FAL=(.*)/s) { @FAL=split/\W+/,$1; shift@ARGV }
my $GCCP; # 'g++' or 'gcc': C/C++ preproc with `-E -MG' switches
$GCCP="@ARGV";
$GCCP="gcc" if $GCCP!~y/ \t//c;

# Make sure we get English error messages from gcc.
delete $ENV{LANG};
delete $ENV{LANGUAGE};
$ENV{LC_ALL} = "C";

# ---

# Returns true iff the passed filename is a C or C++ source file (and not .o
# or .h or anything else).
sub is_ds($) {
  my $FN = $_[0];
  return $FN =~ /\.(c(|c|xx|pp|[+][+])|C)\Z(?!\n)/;
}

sub find_ds() {
  #** @return a list of .ds file in the current directory
  my @L;  my $E;
  die unless opendir DIR, '.';
  while (defined($E=readdir DIR)) {
    push @L, $E if is_ds($E) and -f $E;
  }
  @L
}

sub expand_glob($$ $) {
  #** @param $_[0] string containing globs (? and *)
  #** @param $_[1] listref of names
  #** @param $_[2] dest listref
  my $S=quotemeta($_[0]);
  if (0==($S=~s@\\([?*])@.$1@g)) { push @{$_[2]}, $_[0]; return }
  my $RE=eval"+sub{\$_[0]=~/$S/}";
  die$@if$@;
  for my $E (@{$_[1]}) { push @{$_[2]}, $E if $RE->($E) }
}

sub mustbe_subset_of($$ $$) {
  #** A must be a subset (sublist) of B
  #** @param $_[0] caption A
  #** @param $_[1] ref(array(string)) A
  #** @param $_[2] caption B
  #** @param $_[3] ref(array(string)) B
  my @A=sort @{$_[1]};
  my @B=sort @{$_[3]};
  my($AV,$BV);
  while (defined($AV=pop@A)) {
    1 while defined($BV=pop@B) and $BV gt $AV;
    if (!defined($BV) or $BV ne $AV) {
      print STDERR "$0: $_[0] are: @{$_[1]}\n";
      print STDERR "$0: $_[2] are: @{$_[3]}\n";
      die "$0: $_[0] must be a subset of $_[2]\n";
    }
  }
}

# ---

print "$0: running.\n";

sub unix_shq($) {
  my $S=$_[0];
  $S=~s@'@'\\''@g;
  "'$S'"
}

sub shq($) {
  my $S=$_[0];
  return $S if $S!~/[^\w.-]/;
  if ($^O =~ /(?:win32|win64|windows)/i) {  # $^O eq 'MSWin32'
    # assume the called program is CygWin/Ming32; see later
    # Arguments are delimited by white space, which is either a space or a tab.
    # .       A string surrounded by double quotation marks is interpreted as a
    # single argument, regardless of white space contained within. A quoted
    # string can be embedded in an argument. Note that the caret (^) is not
    # recognized as an escape character or delimiter.
    # .       A double quotation mark preceded by a backslash, \", is interpreted as
    # a literal double quotation mark (").
    # .       Backslashes are interpreted literally, unless they immediately precede
    # a double quotation mark.
    # .       If an even number of backslashes is followed by a double quotation
    # mark, then one backslash (\) is placed in the argv array for every pair
    # of backslashes (\\), and the double quotation mark (") is interpreted as
    # a string delimiter.
    # .       If an odd number of backslashes is followed by a double quotation
    # mark, then one backslash (\) is placed in the argv array for every pair
    # of backslashes (\\) and the double quotation mark is interpreted as an
    # escape sequence by the remaining backslash, causing a literal double
    # quotation mark (") to be placed in argv.
    $S =~ s@(\\+)(?="|\Z(?!\n))@$1$1@;
    $S=~s@"@\\"@g; return qq{"$S"}
  } else {
    $S=~s@'@'\\''@g; return qq{'$S'}
  }
}
#die shq(q{foo\b"ar\\}) eq q{"foo\b\"ar\\\\"};  # For Win32.

sub backtick(@) {
  my $S=$_[0];
  if ($^O eq 'MSWin32') {
    # assume the called program is CygWin/Ming32; and we have proper /bin/sh
    $S="sh -c ".unix_shq($S); # /bin/sh can handle IO-redirections such as `2>&1' right
  } else {
    # assume UNIX
  }
  print "+ $S\n";
  #die unless $ENV{PATH}=~/(.*)/s; # Dat: untaint()
  #$ENV{PATH}=$1;
  #die "$ENV{PATH}";
  # Dat: if `.' is part of $ENV{PATH}, `Insecure directory...' is reported
  #die unless $S=~/(.*)/s; # Dat: untaint()
  readpipe $S # `` qx``
}

my @DS=find_ds();
my @DSQ=map{shq$_}@DS;
# g++-4.6 has it, clang++-3.4 doesn't but we don't care to rerun.
my $DIAG = "";
my $Q="$GCCP -DOBJDEP$DIAG -M -MG -E 2>&1 @DSQ";
my $R=backtick($Q);

if ($R!~/#\s*warning\s/) {
  # config2.h:314:4: warning: #warning REQUIRES: c_lgcc3.o
  # Dat: g++-3.3 and g++-4.8 omit #warning (implicit -w) with -M -MG -E.
  #      clang++-3.4 still emits warnings, so no need to rerun here.
  $R.="\n".backtick("$GCCP -DOBJDEP$DIAG -E 2>&1 >/dev/null @DSQ");
}

$R =~ s@\\\n@@g;  # Merge line continuations emitted by `gcc -M -MG'.

#** $idep{"x.ds"} contains the .h and .ds dependency line for x.ds
my %idep;
#** $odep{"x.o"} is "x.ds"
my %odep;
my $included_from;

my @instructions;

while ($R=~/\G(.*)\n?/g) {
  my $S=$1;

  if ($S=~/\AIn file included from (?:[.]\/)*([^:]+)/) {
    # If foo.cpp includes foo.hpp, which includes bar.hpp, then
    # clang++-3.4 emits these in a row: foo.cpp, ./foo.hpp, and no bar.hpp.
    # We want to keep only foo.cpp, hence we check
    # `if !defined($included_from)'.
    $included_from=$1 if !defined($included_from);
      # Bottommost includer.
  } elsif ($S=~/\A\s{3,}from (?:[.]\/)*([^:]+)/) {  # From gcc-3.2.
    $included_from=$1;  # Higher includer, we override the previous one, because we need the topmost one.
  } elsif ($S=~/\A(?:[.]\/)*([^:]+):\d+:(\d+:)? warning: (?:#warning )?([A-Z][-A-Z]{2,}):(.*)\Z/) {
    # ^^^ (\d+:)? added for gcc-3.1
    # ^^^ clang: appliers.cpp:554:6: warning: REQUIRES: out_gif.o [-W#warnings]
    my($DS,$B,$features)=($1, $3, $4);  # $B is e.g. 'PROVIDES'.
    if (defined $included_from) { $DS=$included_from; undef $included_from }
    die "$0: #include detection broken: expected non-header source file, got $DS in line: $S\n" if
        not is_ds($DS);
    die "$0: unknown dependency verb $B in line: $S\n" if
        $B !~ m@\A(NULL-PROVIDES|PROVIDES|CONFLICTS|REQUIRES)\Z(?!\n)@;
    for my $feature (split ' ',$features) {
      if ($feature !~ m@\A\[-W@) {  # g++-4.8 generates extra [-Wcpp] lines.
        #print "INSTR $DS $B $feature\n";
        push @instructions, [$DS, $B, $feature];
      }
    }
  } elsif ($S=~/\A([^: ]+)\.o:( ([^: ]+?)\.([^ \n]+).*)\Z/s and $1 eq $3) {
    # Dependency output of `gcc -M -MG'.

    # $O: The .o file.
    # $B: All the source dependencies (.cpp, .cxx, .cc, .C, .c, .ci etc.).
    # $DS: The .ds file: the source file corresponding to the .o file, e.g.
    #     $O is 'encoder.o', $DS is 'encoder.cpp'.
    my($O,$B,$DS)=("$1.o",$2,"$3.$4");
    # ^^^ Dat: maybe both t.c and t.cxx
    $B =~ s@ /[^ ]+@@g; # remove absolute pathnames
    $B =~ s@\s+@ @g;  $B =~ s@\A\s*@ @;  $B =~ s@\s*\Z(?!\n)@ @;
    die "$0: .o file in sources for $DS: $B" if $B =~ m@ [.]o @;
    # Example reason: foo.c and foo.cpp both present as source files.
    die "$0: .o file generated from multiple sources: $O and $odep{$O}" if
        exists $odep{$O};
    die if exists $idep{$DS};  # Shouldn't happen, $odep{$O} would exist first.
    $odep{$O}=$DS;
    $idep{$DS}=$B;
    push @instructions, [$DS, 'PROVIDES', $O];
    undef $included_from;
  } elsif ($S=~/: (?:warning|error|fatal error|fatal):/) {
    undef $included_from;
  } else {
    # Be resilient, and just ignore every other possible message here. This is
    # for future compatibility with compilers. The infamous `invalid depret'
    # error by ccdep.pl used to be here, but now it's gone.
  }
}
undef $included_from;  # Save memory.
%odep=();  # Save memory.

#** $pro{"x.ds"} is the list of features provided by "x.ds"; multiplicity
my %pro;
#** $req{"x.ds"} is the list of features required by "x.ds"; multiplicity
my %req;
#** $con{"x.ds"} is the list of features conflicted by "x.ds"; multiplicity
my %con;
#** $repro{"feature"} is a hash=>1 of .ds that provide "feature"
my %repro;
#** $mapro{"executable"} eq "x.ds" if "x.ds" provides main() for "executable".
#** It looks like values are never used.
my %mapro;
#** hash=>1 of "feature"s of NULL-PROVIDES
my %nullpro;
for my $instruction (@instructions) {
  my($DS, $verb, $feature) = @$instruction;
  if ($verb eq 'NULL-PROVIDES') {
    $nullpro{$feature} = 1;
  } elsif ($verb eq 'PROVIDES') {
    push @{$pro{$DS}}, $feature;
    push @{$con{$DS}}, $feature;
    $repro{$feature}{$DS}=1;
    if ($feature=~/\A(.*)_main\Z/) { $mapro{$1}=$DS }
  } elsif ($verb eq 'REQUIRES') {
    push @{$req{$DS}}, $feature;
  } elsif ($verb eq 'CONFLICTS') {
    push @{$con{$DS}}, $feature;
  } else {
    die "$0: unknown verb in instruction: @$instruction\n";  # Never happens.
  }
}
@instructions=();  # Save memory.

mustbe_subset_of "providers"=>[sort keys(%pro)], "dep_sources"=>[sort keys(%idep)];
mustbe_subset_of "dep_sources"=>[sort keys(%idep)], "sources"=>\@DS;

{ my @K=keys %repro;
  for my $DS (sort keys%con) {
    my $L = $con{$DS};
    my @R=();
    for my $feature (@$L) { expand_glob $feature, \@K, \@R }
    # ^^^ multiplicity remains
    $con{$DS}=\@R;
  }
}

my $outfn = "Makedep";
die unless open MD, "> $outfn";
die unless print MD '
ifndef CCALL
CCALL=$(CC) $(CFLAGS) $(CFLAGSB) $(CPPFLAGS) $(INCLUDES)
endif
ifndef CXXALL
CXXALL=$(CXX) $(CXXFLAGS) $(CXXFLAGSB) $(CPPFLAGS) $(INCLUDES)
endif
ifndef LDALL
LDALL=$(LDY) $(LDFLAGS) $(LIBS)
endif
ifndef CC
CC=gcc
endif
ifndef CXX
CXX=g++
endif
ifndef LD
LD=$(CC) -s
endif
ifndef LDXX
LDXX=$(CXX) -s
endif
ifndef LDY
LDY=$(LD)
endif
ifndef CFLAGS
CFLAGS=-O2 -W -Wall -fsigned-char
endif
ifndef CXXFLAGS
CXXFLAGS=-O2 -W -Wall -fsigned-char
endif
ifndef GLOBFILES
GLOBFILES=Makefile $outfn
endif
';

die unless print MD "ALL +=", join(' ',sort keys%mapro), "\n";
die unless print MD "TARGETS =", join(' ',sort keys%mapro), "\n";

# vvv Thu Oct 31 09:49:02 CET 2002
# (not required)
#my %targets_fal;
#for my $FA (@FAL) { $targets_fal{$FA}="TARGETS_$FA =" }

for my $EXE (sort keys%mapro) {
  print "exe $EXE\n";
  my @REQO  = (); # Will be list of .o files required by $EXE.
  my @REQDS = (); # Will be list of .ds files required $EXE.
  my @REQH  = (); # Will be list of .h files required $EXE.

  # Find the transitive dependencies of $EXE, save to @REQO, @REQDS, @REQH.
  {
    my %CON=(); # hash=>1 of features already conflicted
    my %PRO=%nullpro; # hash=>1 of features already provided
    my $feature;
    my @features_to_analyze=("${EXE}_main");
    while (defined($feature=pop@features_to_analyze)) {
      next if exists $PRO{$feature};
      #print "feat $feature (@features_to_analyze)\n"; ##
      # vvv Dat: r.b.e == required by executable
      die "$0: feature $feature r.b.e $EXE conflicts\n" if exists $CON{$feature};
      my @L=sort keys%{$repro{$feature}};
      die "$0: feature $feature r.b.e $EXE unprovided\n" if!@L;
      die "$0: feature $feature r.b.e $EXE overprovided: @L\n" if$#L>=1;
      # Now: $L[0] is a .ds providing the feature
      push @REQDS, $L[0];
      my $O=$L[0]; $O=~s@\.[^.]+\Z@.o@;
      push @REQO, $O;

      $PRO{$feature}=1;
      for my $feature2 (@{$pro{$L[0]}}) {
        die "$0: extra feature $feature2 r.b.e $EXE conflicts\n" if exists $CON{$feature2} and not exists $PRO{$feature2};
        $PRO{$feature2}=1;
      }
      for my $feature2 (@{$req{$L[0]}}) {
        push @features_to_analyze, $feature2 if!exists $PRO{$feature2}
      }
      for my $feature2 (@{$con{$L[0]}}) { $CON{$feature2}=1 }
      # die if! exists $PRO{$feature}; # assert
    }
    my %REQH;
    for my $DS (@REQDS) {
      for my $HDS (split' ', $idep{$DS}) {
        $REQH{$HDS} = 1 if !is_ds($HDS);
      }
    }
    push @REQH, sort keys %REQH;
  }

  die unless print MD "${EXE}_DS=@REQDS\n${EXE}_H=@REQH\n".
    "$EXE: \$(GLOBFILES) @REQO\n\t\$(LDALL) @REQO -o $EXE\n\t".
    q!@echo "Created executable file: !.$EXE.
    q! (size: `perl -e 'print -s "!.$EXE.q!"'`)."!. "\n";
  # vvv Sat Jun  1 15:40:19 CEST 2002
  for my $FA (@FAL) {
    die unless print MD
        "$EXE.$FA: \$(GLOBFILES) \$(${EXE}_DS) \$(${EXE}_H)\n\t".
        "\$(CXD_$FA) \$(CXDFAL) \$(${EXE}_DS) -o $EXE.$FA\n";
    # $targets_fal{$FA}.=" $EXE.$FA";
  }
}
print MD "\n";

# vvv Thu Oct 31 09:49:02 CET 2002
# (not required)
# for my $FA (@FAL) { print MD "$targets_fal{$FA}\n" }
# print MD "\n";

for my $K (sort keys%idep) {
  my $V = $idep{$K};
  my $O=$K; $O=~s@\.([^.]+)\Z@.o@; my $srcext = $1;
  my @V = sort split' ', $V;
  my $compiler = $srcext eq'c'?"CC":"CXX";
  print MD "$O: \$(GLOBFILES) @V\n\t\$(${compiler}ALL) -c $K\n"
}
print MD "\n";

die unless close MD;

print "$0: done, created $outfn\n";

__END__
