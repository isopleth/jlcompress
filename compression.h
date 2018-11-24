#ifndef COMPRESSION_H
#define COMPRESSION_H
#include <stdlib.h>

/* General compression/decompression function declarations and structures */

#include "boolean.h"

typedef struct {
  unsigned char* address;
  size_t allocatedSize;

  size_t nextFreeByte;
  unsigned char nextFreeBit;

  size_t nextByteToRead;
  unsigned char nextBitToRead;

  size_t usedSize;

  int fileDescriptor;
  enum { UNDEFINED_TYPE = 0,
         MEMORY_TYPE,
         COMPRESSED_FILE_TYPE,
         UNCOMPRESSED_FILE_TYPE } type;
  unsigned char encoding;
} BlockDescriptor;


struct CompressionFlags {
  Boolean flip;
  Boolean rle;
  Boolean huffman;
};

void compress(const struct CompressionFlags* flags,
              const char* inputFilename,
              const char* outputFilename);

void decompress(const char* inputFilename,
                const char* outputFilename);

void displayFinalStatistics(const char* inputFilename,
                            const char* outputFilename);

unsigned getFileSize(const char* filename);

char* makeOutputFilename(const char* inputFilename,
			 Boolean compressing);

Boolean isCompressedFile(const char* filename);

void error(Boolean displayErrno, char* format, ...);


BlockDescriptor* runLengthCompress(BlockDescriptor* inputBlock);
BlockDescriptor* runLengthDecompress(BlockDescriptor* inputBlock);

BlockDescriptor* flipBitOrder(BlockDescriptor* inputBlock);
BlockDescriptor* unflipBitOrder(BlockDescriptor* inputBlock);

BlockDescriptor* huffmanCompress(BlockDescriptor* inputBlock);
BlockDescriptor* huffmanDecompress(BlockDescriptor* inputBlock);

#endif
