#ifndef DATABLOCKS_H
#define DATABLOCKS_H

/*
 * Declarations for code which handles data blocks and data block
 * descriptors, in dataBlocks.c
 */

#include "compression.h"

BlockDescriptor* mapCompressedFile(const char* filename);
BlockDescriptor* mapUncompressedFile(const char* filename);
BlockDescriptor* makeMemoryBlock(size_t size);
void freeBlock(BlockDescriptor* blockDescriptor);

size_t writeToBlock(BlockDescriptor* outputBlock,
		    unsigned char character);
void writeBitToBlock(BlockDescriptor* outputBlock, Boolean value);
Boolean readBitFromBlock(BlockDescriptor* inputBlock);
unsigned char readFromBlock(BlockDescriptor* inputBlock);
void resetBlockOffsets(BlockDescriptor* blockDescriptor);
void createFile(const char* filename,
                BlockDescriptor* blockDescriptor,
                Boolean outputHeader);

void displayStatistics(const char* operation,
		       const BlockDescriptor* originalBlock,
		       const BlockDescriptor* finalBlock);

Boolean getBit(unsigned char bitNumber,
               unsigned char byte);

void setBit(unsigned char bitNumber,
            unsigned char* byte,
            Boolean value);

#endif
