#!/usr/bin/env perl
# This test makes a 100 MB file and tries the different compression
# schemes on it.

use strict;
use warnings;
use Carp;
use constant FILE_SIZE => 100000000;
use Time::HiRes qw(time);

# printAndUnderline
# 
# Print and underling message
#
sub printAndUnderline($) {
    my $text = shift();
    print "\n\n$text\n";

    for (my $index = 0; $index < length($text); ++$index) {
        print "=";
    }
    print "\n";
}

# Line
#
# Just print a line
#
sub line() {
    print "\n--------------------------------------------------------\n";
}

# makeBigFile
#
# Generate a valid HTML file around 100 MB long.
#
sub makeBigFile() {
    print "Making the file\n";
    open(OUT, ">bigFile.html") or croak($!);
    print OUT "<html><title>A big file</title><body>\n";
    my $charsLeft = FILE_SIZE;
    while ($charsLeft > 0) {
	my $line = "This is a nice shiny new line<br>\n";
	print OUT $line;
	$charsLeft -= length($line);
    }
    print OUT "</body></html>\n";
    close(OUT) or croak($!);
}


# deleteFile
#
# Delete file specified by parameter
#
sub deleteFile($) {
    my $filename = shift();
    print "Deleting $filename\n";
    unlink($filename) or croak($!);
    # Make sure it has really gone
    croak("$filename not deleted!") if -f $filename;
}


printAndUnderline("Test compressing and decompressing a file about " .
    FILE_SIZE . " bytes long");

makeBigFile();

# Create empty versions of the file so that first unlink works
open(OUT,">bigFile.html.compressed") or croak($!);
close(OUT) or croak($!);
open(OUT,">bigFile.html.decompressed") or croak($!);
close(OUT) or croak($!);

foreach my $switches ("", "--rle", "--rle --flip",
                      "--huffman", "--huffman --flip",
                      "--huffman --flip --rle",
                      "--huffman --rle") {
    
    line();
    printAndUnderline(length($switches) ? "Compressing file, with switches $switches" :
                      "Compressing file, with default switches");

    # Get rid of the output files from the last iteration

    deleteFile("bigFile.html.compressed");
    deleteFile("bigFile.html.decompressed");

    my $command = "./jlcompress $switches bigFile.html bigFile.html.compressed";
    print "$command\n";
    my $startTime = time();
    system($command);
    my $elapsedTime = time() - $startTime;
    printf("Elapsed time %.1f seconds\n", $elapsedTime);
    printAndUnderline("Decompressing file again");
    $command = "./jldecompress bigFile.html.compressed bigFile.html.decompressed";
    print "$command\n";
    $startTime = time();
    system($command);
    $elapsedTime = time() - $startTime;
    printf("Elapsed time %.1f seconds\n", $elapsedTime);
    if (system("diff -s bigFile.html bigFile.html.decompressed") != 0) {
        print("*** Error: original file and file after compression/decompression differ\n");
        exit(-1);
    }
}

print "Deleting the file, and compressed/decompressed versions\n";
unlink("bigFile.html") or croak($!);
if (-f "bigFile.html.compressed") {
    unlink("bigFile.html.compressed") or croak($!);
}

if (-f "bigFile.html.decompressed") {
    unlink("bigFile.html.decompressed") or croak($!);
}

print "\n\nAll tests passed\n\n";

