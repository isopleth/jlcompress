#!/usr/bin/env perl
# This test makes a series of files with a single character repeated
# in them and RLE compresses and decompresses the files. It tries
# a "normal" character, the jlcompress RLE repeat character and the
# jlcompress RLE escape character. It tries files of 1->999 of these
# characters. Strictly speaking anything interesting (i.e. problems)
# should happen with up to 256 characters but it keeps going to 999
# characters anyway.

use strict;
use warnings;
use Carp;
use File::Copy;

sub printAndUnderline($) {
    my $text = shift();
    print "$text\n";

    for (my $index = 0; $index < length($text); ++$index) {
        print "=";
    }
    print "\n";
}


# repeatTests()
#
# Generate a file containing the specified character, compress and
# uncompress and see if the result is the same.  Repeat with two
# characters. And so on 999 times.
#
# Parameter:
# $char - character to repeat
#
sub repeatTests($$) {
    my $char = shift();
    my $description = shift();

    for (my $count = 1; $count < 1000; $count++) {
        unlink("test.txt") if -f "test.txt";
        unlink("test.txt.compressed") if -f "test.txt.compressed";
        unlink("test.txt.decompressed") if -f "test.txt.decompressed";

        print "\n\n";
        printAndUnderline("$count bytes of $description" . (($count == 1) ? "" : "s"));

        open(ORIGINAL,">test.original") or croak("$!");
        for (my $charNo = 0; $charNo < $count; $charNo++) {
            print ORIGINAL $char;
        }
        close ORIGINAL;
        copy("test.original", "test.txt") or croak("$!");
        printAndUnderline("Compressing file");
        system("./jlcompress --rle test.txt");
        printAndUnderline("Decompressing file");
        system("./jlcompress test.txt.compressed");
        printAndUnderline("Comparing files");
        
        if (system("diff -s test.txt test.txt.decompressed") != 0) {
            print("*** Error: original file and file after compression/decompression differ\n");
            exit(-1);
        }

    }
}

# Repeat with file just made up of escape symbols
repeatTests(chr(235), "escape character");
# repeat with file just made up of repeat symbols
repeatTests(chr(236), "repeat character");
# Repeat with file made up of symbols which don't have any special meaning
repeatTests('c', "normal character");


