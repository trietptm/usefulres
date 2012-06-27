#! /usr/bin/perl

$fname = "makefile.unx";
if ( not -e $fname )
{
  $fname = "/tmp/makefile.$$";
  open IN, "<makefile" or die "No makefile?!";
  open OUT, ">$fname" or die "Can't create temp file: $fname";

  while ( <IN> )
  {
    next if m/\!include .+objdir.mak/;
    s/objdir//;     # remove objdir
    s/^!//;         # remove ! at the beginning
    s#\\(..)#/\1#g; # replace backslashes by slashes
    s/\.mak/.unx/;  # replace .mak by .unx
    print OUT $_;
  }

  close IN;
  close OUT;
  $code = system("__LINUX__=1 make -f $fname @ARGV") != 0;
  system("rm -f $fname");
#  print "exit code=$code\n";
  exit $code;
}
exec("__LINUX__=1 make -f $fname @ARGV");

