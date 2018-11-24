#ifndef HEADER_H
#define HEADER_H

/* Declarations relating to the header of the compressed file */

#include <stdlib.h>    /* Need size_t */
#include <stdio.h>
#include "dataBlocks.h"

#define ENCODING_RUN_LENGTH (0x1)
#define ENCODING_FLIPPED (0x2)
#define ENCODING_HUFFMAN (0x4)

size_t getHeaderSize();

void writeHeader(FILE* file,
                 BlockDescriptor* blockDescriptor);

Boolean isFlipped(BlockDescriptor* blockDescriptor);
Boolean isRleCompressed(BlockDescriptor* blockDescriptor);
Boolean isHuffmanCompressed(BlockDescriptor* blockDescriptor);

unsigned char getCompressionFlags(const char* filename,
                                  Boolean printDescription);

#endif
