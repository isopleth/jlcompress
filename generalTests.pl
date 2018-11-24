#! /bin/perl
use strict;
use warnings;
use Carp;

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

# line
#
# Just print a line
#
sub line() {
    print "\n\n--------------------------------------------------------\n\n";
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

# Create the two output files so that they can be deleted.
# Make them different in case the delete doesn't happen and the program
# doesn't do anything - then the diff will fail.
open FILE1,">Huffman_coding.html.compressed" or croak($!);
print FILE1 "One\n";
close FILE1;
open FILE2,">Huffman_coding.html.decompressed" or croak($!);
print FILE2 "Two\n";
close FILE2;
# Sanity check - check that diff is working!
# 0 status code means that tjhe files are identical, which they are not
if (system("diff -s Huffman_coding.html.compressed Huffman_coding.html.decompressed > /dev/null") == 0) {
    print("*** Error: diff doesn't seem to be working properly\n");
    exit(-1);
}

foreach my $decompress ("./jlcompress", "./jldecompress") {

    foreach my $switches ("", "--rle", "--rle --flip",
			  "--huffman", "--huffman --flip",
			  "--huffman --flip --rle",
			  "--huffman --rle") {
        
	line();
	printAndUnderline(length($switches) ? "Compressing HTML page with switches $switches" :
                          "Compressing HTML page with default switches");

        # Get rid of the output files from the last iteration
        deleteFile("Huffman_coding.html.compressed");
        deleteFile("Huffman_coding.html.decompressed");

 	system("./jlcompress $switches Huffman_coding.html Huffman_coding.html.compressed");
	printAndUnderline("Decompressing file again");
	system("$decompress Huffman_coding.html.compressed Huffman_coding.html.decompressed");
	if (system("diff -s Huffman_coding.html Huffman_coding.html.decompressed") != 0) {
	    print("*** Error: original file and file after compression/decompression differ\n");
	    exit(-1);
	}
    }
}

print "\n\nAll tests passed\n\n";




