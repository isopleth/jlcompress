#include <stdio.h>
#include <string.h>
#include "header.h"

/* header.c
 *
 * Processing of compressed file header.
 */

#define HEADER_SIZE (5)

size_t getHeaderSize(void) {
  return HEADER_SIZE;
}

void writeHeader(FILE* file,
                 BlockDescriptor* blockDescriptor) {

  char header[HEADER_SIZE] = { 'J', 'L', 'C', 'M', 0 };
  header[HEADER_SIZE - 1] = blockDescriptor->encoding;

  if (fwrite(header, 1, HEADER_SIZE, file) != HEADER_SIZE) {
    error(True, "Unable to write header to output file");
  }
}


/* describeCompressionFlags
 *
 * Describe the file compression.
 *
 * Parameters:
 * Compression flags from header.
 */
static unsigned describeCompressionFlags(unsigned char flags) {
  if (flags == 0) {
    printf("* File is not compressed\n");
  }
  else {
    if (flags & ENCODING_FLIPPED) printf("* File is flipped\n");
    if (flags & ENCODING_RUN_LENGTH) printf("* File is run length encoded\n");
    if (flags & ENCODING_HUFFMAN) printf("* File is Huffman encoded\n");
  }
  return flags;
}


/* getCompressionFlags
 *
 * Return the compression flags of a file
 *
 * Parameters:
 * filename - input filename
 * outputDescription - True if a description of the flags is to be printed
 *
 * Return:
 * Compression flags mask
 */
unsigned char getCompressionFlags(const char* filename,
                                  Boolean outputDescription) {
  unsigned char buffer[HEADER_SIZE];
  FILE* file = fopen(filename, "rb");
  int bytesRead = 0;
  const char* expectedHeader = "JLCM";

  if (file == NULL) {
    error(True, "Unable to open file %s", filename); 
  }

  bytesRead = fread(buffer, 1, sizeof(buffer), file);
  if (ferror(file)) {
      error(True, "Unable to read file %s", filename); 
    }

  if (fclose(file)) {
    error(True, "Unable to close file %s", filename); 
  }

  if (bytesRead < HEADER_SIZE) {
    return 0;
  }

  if (memcmp(buffer, expectedHeader, strlen(expectedHeader))) {
    return 0;
  }
  
  if (buffer[HEADER_SIZE-1] & (!ENCODING_RUN_LENGTH)) {
    error(False,"Looks like a compressed file, but cannot understand encoding\n");
  }


  return outputDescription ? describeCompressionFlags(buffer[HEADER_SIZE-1]) :
    buffer[HEADER_SIZE-1];
}

/* isFlipped
 *
 * Returns True if the block descriptor indicatates the that file contents
 * have been flipped.
 *
 * Parameters:
 * blockDescriptor - descriptor
 *
 * Return:
 * True if the file has been flipped.
 */
Boolean isFlipped(BlockDescriptor* blockDescriptor) {
  return (blockDescriptor->encoding & ENCODING_FLIPPED) ? True : False;
}

/* isRleCompressed
 *
 * Returns True if the block descriptor indicatates the that file contents
 * have been run length encoded.
 *
 * Parameters:
 * blockDescriptor - descriptor
 *
 * Return:
 * True if the file has been run length encoded.
 */
Boolean isRleCompressed(BlockDescriptor* blockDescriptor) {
  return (blockDescriptor->encoding & ENCODING_RUN_LENGTH) ? True : False;
}


/* isHuffmanCompressed
 *
 * Returns True if the block descriptor indicatates the that file contents
 * have been Huffman encoded.
 *
 * Parameters:
 * blockDescriptor - descriptor
 *
 * Return:
 * True if the file has been Huffman encoded encoded.
 */
Boolean isHuffmanCompressed(BlockDescriptor* blockDescriptor) {
  return (blockDescriptor->encoding & ENCODING_HUFFMAN) ? True : False;
}

