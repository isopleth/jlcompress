
.PHONY: clean all generalTests tests

all: jlcompress jldecompress
	@echo
	@echo "  Type \"make test\" to run the quick general test"
	@echo
	@echo "  Type \"make alltests\" to run general test + more RLE tests + big file tests"
	@echo "  Note that the big file tests need 300 MB of free disk space"
	@echo "  (100 MB for each of original file, worst case compressed file and"
	@echo "  the decompressed file)"
	@echo
	@echo "  See description.pdf (or description.txt) for more information"
	@echo

clean:
	-rm *.o jlcompress jldecompress *.compressed *.decompressed bigFile.html test.txt test.original

rebuild: clean all

test: jlcompress jldecompress
	perl generalTests.pl

alltests: jlcompress jldecompress
	perl generalTests.pl
	perl rleTests.pl
	perl bigFile.pl

CC = gcc
CFLAGS = -W -Wall -pedantic
HEADERS = compression.h  dataBlocks.h  header.h  huffmanCompressor.h

# These are the object files used by both programs
COMMON_OBJECTS = \
	dataBlocks.o \
	huffmanCompressor.o \
	flipper.o \
	header.o \
	compression.o \
	runLengthCompressor.o \
	huffmanTree.o

compression.o : compression.c $(HEADERS)
dataBlocks.o : dataBlocks.c  $(HEADERS)
flipper.o : flipper.c  $(HEADERS)
header.o : header.c  $(HEADERS)
huffmanCompressor : huffmanCompressor.c $(HEADERS)
huffmanTree : huffmanTree.c $(HEADERS)
jlcompress.o : jlcompress.c $(HEADERS)
jldecompress.o : jldecompress.c $(HEADERS)
runLengthCompressor.o : runLengthCompressor.c $(HEADERS)

jlcompress : jlcompress.o $(COMMON_OBJECTS)
	gcc jlcompress.o $(COMMON_OBJECTS) -o jlcompress

jldecompress : jldecompress.o $(COMMON_OBJECTS)
	gcc jldecompress.o $(COMMON_OBJECTS) -o jldecompress

