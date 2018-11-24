#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "compression.h"
#include "dataBlocks.h"
#include "header.h"

extern const char* programName_g;

/* makeOutputFilename()
 *
 * Constructes the output filename from the input filename.
 * Caller is responsible for freeing output filename buffer.
 *
 * Parameters:
 * inputFilename input filename
 * compressing - True if a filename suitable for a compressed
 *               file is to be generated, False for uncompressed.
 */
char* makeOutputFilename(const char* inputFilename,
			 Boolean compressing) {
  static const char* compressedSuffix = ".compressed";
  static const char* decompressedSuffix = ".decompressed";
  char* outputFilename = NULL;
  char* dotPtr = NULL;

  const char* checkFor = NULL;
  const char* append = NULL;

  if (compressing) {
    outputFilename = malloc(strlen(inputFilename) + 1 +
			    strlen(compressedSuffix));
    checkFor = decompressedSuffix;
    append = compressedSuffix;
  }
  else {
    outputFilename = malloc(strlen(inputFilename) + 1 +
			    strlen(decompressedSuffix));
    checkFor = compressedSuffix;
    append = decompressedSuffix;
  }

  if (outputFilename == NULL) {
    error(True,"unable to malloc space for filename");
  }
  
  strcpy(outputFilename, inputFilename);
  dotPtr = strrchr(outputFilename, '.');
  if ((dotPtr != NULL) && (!strcmp(dotPtr, checkFor))) {
    *dotPtr = '\0';
    strcat(outputFilename, append);
  }
  else {
    strcat(outputFilename, append);
  }
  return outputFilename;
}



/* compress()
 *
 * Compress the file.
 *
 * Parameters:
 * flags - command line switches
 * inputFilename - file to compress
 * outputFilename - file to write compressed output to
 */
void compress(const struct CompressionFlags* flags,
              const char* inputFilename,
              const char* outputFilename) {

  BlockDescriptor* inputBlock = mapUncompressedFile(inputFilename);
  BlockDescriptor* outputBlock = NULL;
  
  if (flags->flip) {
    outputBlock = flipBitOrder(inputBlock);
    freeBlock(inputBlock);
    inputBlock = outputBlock;
  }


  if (flags->rle) {
    outputBlock = runLengthCompress(inputBlock);
    freeBlock(inputBlock);
    inputBlock = outputBlock;
  }

  if (flags->huffman) {
    outputBlock = huffmanCompress(inputBlock);
    freeBlock(inputBlock);
    inputBlock = outputBlock;
  }

  createFile(outputFilename, inputBlock, True);

  freeBlock(inputBlock);
}


/* decompress()
 *
 * Decompress the file.
 *
 * Parameters:
 * inputFilename - file to decompress
 * outputFilename - file to write decompressed output to
 */
void decompress(const char* inputFilename,
                const char* outputFilename) {
  BlockDescriptor* inputBlock = mapCompressedFile(inputFilename);
  BlockDescriptor* outputBlock = NULL;

  /* Will return NULL if block not Huffman compressed */
  outputBlock = huffmanDecompress(inputBlock);
  /* Replace inputBlock with outputBlock for next phase */
  if (outputBlock != NULL) {
    freeBlock(inputBlock);
    inputBlock = outputBlock;
    outputBlock = NULL;
  }

  /* Will return NULL if block not run length encoded */
  outputBlock = runLengthDecompress(inputBlock);
  /* Replace inputBlock with outputBlock for next phase */
  if (outputBlock != NULL) {
    freeBlock(inputBlock);
    inputBlock = outputBlock;
    outputBlock = NULL;
  }

  /* Will return NULL if block not had bit order flipped */
  outputBlock = unflipBitOrder(inputBlock);
  /* Replace inputBlock with outputBlock for next phase */
  if (outputBlock != NULL) {
    freeBlock(inputBlock);
    inputBlock = outputBlock;
    outputBlock = NULL;
  }

  createFile(outputFilename, inputBlock, False);

  freeBlock(inputBlock);
}


/* getFileSize()
 *
 * Return file size in bytes.
 *
 * Parameters:
 * filename - filename
 *
 * Return value:
 * File size in bytes
 */
unsigned getFileSize(const char* filename) {
  struct stat fileStat;
  if (stat(filename, &fileStat)) {
    error(True, "Unable to get file length for %s", filename);
  }
  return fileStat.st_size;
}


/* error()
 *
 * Print error message on stderr and terminate program.
 *
 * Parameters:
 * displayErrno - True if errno value is to be displayed
 * format,... - as for printf
 */
void error(Boolean displayErrno, char* format, ...) {
  va_list args;
  va_start(args, format);
  if (displayErrno) {
    fprintf(stderr, "Error: ");
    perror(programName_g);
  }
  else {
    fprintf(stderr, "Error: %s - ", programName_g);
  }
  vfprintf(stderr, format, args);
  va_end (args);
  fprintf(stderr, "\n");
  exit(-1);
}

/* displayFinalStatistics()
 *
 * Display final statistics of compression or decompression
 *
 * Parameters:
 * inputFilename - input filename - needed for file size
 * outputFilename - output filename - needed for file size
 */
void displayFinalStatistics(const char* inputFilename,
                            const char* outputFilename) {
  unsigned inSize = getFileSize(inputFilename);
  unsigned outSize = getFileSize(outputFilename);
  printf("Before %u bytes, after %u bytes = %4.1f%% change\n",
         inSize, outSize, 
         (100-(100.*(float)outSize/(float)inSize)));
}
