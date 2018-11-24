#ifndef HUFFMAN_COMPRESSOR_H
#define HUFFMAN_COMPRESSOR_H

/* 
 * Declarations of structures and functions used for Huffman compression.
 */

#include <stdlib.h>
#include "boolean.h"

typedef struct HuffmanNodeStruct {
  struct HuffmanNodeStruct* left;
  struct HuffmanNodeStruct* right;
  unsigned char symbol;
  unsigned frequency;
} HuffmanNode;

/* The frequency table is an array of these elements */
typedef struct FreqEntryStruct {
  unsigned char symbol;
  size_t frequency;
  unsigned long huffmanBits;
  unsigned char huffmanBitCount;  /* Explicitly make this as big as possible */
} FrequencyTableEntry;

/* As the frequency table can contain one entry per byte the maximum
 * size is 256 entries.
 */
#define FREQUENCY_TABLE_SIZE (256)
typedef FrequencyTableEntry FrequencyTable[FREQUENCY_TABLE_SIZE];

HuffmanNode* buildHuffmanTree(FrequencyTable frequencyTable);

void walkHuffmanTree(HuffmanNode* tree, FrequencyTable frequencytable,
		     unsigned pattern, unsigned patternLength);


Boolean getHuffmanChar(Boolean bitRead,
		       HuffmanNode* huffmanNode,
		       unsigned char* character);

#endif
