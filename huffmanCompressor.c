/* huffmanCompressor.c
 *
 * This implements Huffman compression and decompression.
 */

#include <stdio.h>
#include "compression.h"
#include "dataBlocks.h"
#include "header.h"
#include "huffmanCompressor.h"

/* initFrequencyTable()
 *
 * Initialise the frequency table.
 *
 * Parameters:
 * frequencyTable - Frequency table array
 */
static void initFrequencyTable(FrequencyTable frequencyTable) {
  unsigned symbol;
  for (symbol = 0; symbol < FREQUENCY_TABLE_SIZE; symbol++) {
    frequencyTable[symbol].symbol = symbol;
    frequencyTable[symbol].frequency = 0;
    frequencyTable[symbol].huffmanBits = 0;
    frequencyTable[symbol].huffmanBitCount = 0;
  }

}

/* populateFrequencyTable()
 *
 * Populate the frequency table by counting how many of each character
 * occurs in the input block.
 *
 * Parameters:
 * inputBlock - Descriptor of input block to scan
 * frequencyTable - Frequency table array to populate.
 */
static void populateFrequencyTable(BlockDescriptor* inputBlock,
				   FrequencyTable frequencyTable) {
  size_t offset;
  initFrequencyTable(frequencyTable);
  for (offset = 0; offset < inputBlock->usedSize; offset++) {
    frequencyTable[*(inputBlock->address + offset)].frequency++;
  }
}

/* readFrequencyTableFromBlock()
 *
 * Read the frequency table from the data block.
 *
 * The frequency table has the following format. [UC] means "unsigned
 * char", [UL] means "unsigned long"
 *
 * <number of bytes in original input [UL]>
 * <number of entries in frequency table less one [UC]>
 * <character [UC]> <number of bytes of frequency [UC]> <frequency>
 * <character [UC]> <number of bytes of frequency [UC]> <frequency>
 * <character [UC]> <number of bytes of frequency [UC]> <frequency>
 * <character [UC]> <number of bytes of frequency [UC]> <frequency>
 * <character [UC]> <number of bytes of frequency [UC]> <frequency>
 * ...
 *
 * The number of entries is stored less one because there can be at
 * most 256 entries (i.e. every possible character is present), which
 * is one more than can fit into an unsigned char.  As there MUST be
 * at least one character in the frequency table otherwise the
 * original file would be empty, we don't need the 0 value.  So
 * decrement the number of frequency entries before saving it, then
 * 256 => 255, which conveniently fits into an unsigned char.
 *
 * Parameters:
 * inputBlock - Descriptor for input block to read
 * frequencyTable - Frequency table array to populate.
 */
static void readFrequencyTableFromBlock(BlockDescriptor* inputBlock,
				    FrequencyTable frequencyTable) {
  unsigned index;
  unsigned symbolCount = readFromBlock(inputBlock);
  symbolCount++;

  /* Zero the whole frequency table because we won't read any entries
   * for characters which aren't present in the file.
   */
  initFrequencyTable(frequencyTable);

  for (index = 0; index < symbolCount; index++) {
    unsigned byteCount;
    unsigned byteNumber;
    frequencyTable[index].symbol = readFromBlock(inputBlock);
    byteCount = readFromBlock(inputBlock);
    frequencyTable[index].frequency = 0;

    for (byteNumber = 0; byteNumber < byteCount; byteNumber++) {
      frequencyTable[index].frequency |= 
	(readFromBlock(inputBlock) << (byteNumber * 8));
    }
  }
}


/* writeFrequencyTableToBlock()
 *
 * Write the frequency table to the output block.  The format used is
 * described above. Note that entries for characters which don't
 * appear in the file (i.e. have a frequency of 0) are not written.
 *
 * Parameters:
 * frequencyTable - Frequency table array to write
 * outputBlock - Output block descriptor.
 */
static void writeFrequencyTableToBlock(FrequencyTable frequencyTable,
			      BlockDescriptor* outputBlock) {
  unsigned numberOfEntries = 0;
  size_t symbolCountOffset = 0;
  size_t index = 0;

  /* Also make space for the number of frequency table entries
   * 0 means 1 entry, so we can fit this into an unsigned char
   */
  symbolCountOffset = writeToBlock(outputBlock, 0x0);

  /* Now write the frequency table */
  for (index = 0; index < FREQUENCY_TABLE_SIZE; index++) {
    unsigned char byteCount = 0;
    unsigned long frequency = frequencyTable[index].frequency;

    if (frequency) {
      unsigned byteIndex;
      /* This is just a sanity check! It doesn't have much relevence here
       * but we do it each time we touch the frequency table
       */
      if (frequencyTable[index].symbol != index) {
	error(False, "Frequency table out of order");
      }

      /* Symbol */
      writeToBlock(outputBlock, index);
      
      while (frequency) {
	byteCount++;
	frequency = frequency >> 8;
      }
      frequency = frequencyTable[index].frequency;
      writeToBlock(outputBlock, byteCount);
      for (byteIndex = 0; byteIndex < byteCount; byteIndex++) {
	unsigned long frequencyByte = frequency & 0xff;
	writeToBlock(outputBlock, (unsigned char)frequencyByte);
	frequency = frequency >> 8;
      }
      numberOfEntries++;
    }
  }
  /* Fill in number of entries */
  numberOfEntries--;
  *(outputBlock->address + symbolCountOffset) = (unsigned char)numberOfEntries;

}

BlockDescriptor* huffmanCompress(BlockDescriptor* inputBlock) {
  
  size_t offset;
  FrequencyTable frequencyTable;
  BlockDescriptor* outputBlock = makeMemoryBlock(inputBlock->usedSize);
  HuffmanNode* huffmanNode;

  union {
    unsigned long bytesInFile;
    char ch[sizeof(unsigned long)];
  } bytesInFile;


  if (isHuffmanCompressed(inputBlock)) {
    error(False, "File already Huffman encoded");
  }

  /* Get the frequency table */
  populateFrequencyTable(inputBlock, frequencyTable);
  
  /* Generate the Huffman table */
  huffmanNode = buildHuffmanTree(frequencyTable);
  
  /* Walk the tree and fill in the bit patterns into the
   * frequency table. This also deletes the tree
   */
  walkHuffmanTree(huffmanNode, frequencyTable, 0, 0);
  huffmanNode = NULL;
  
  /* Check that the frequency table is filled in properly */
  for (offset = 0; offset < FREQUENCY_TABLE_SIZE; offset++) {
    if (frequencyTable[offset].frequency) { 
      if (frequencyTable[offset].huffmanBitCount == 0) {
	error(False, "Not all characters in file have patterns");
      }
    }
    else if (frequencyTable[offset].huffmanBitCount != 0) {
      error(False, "Unused characters in file have patterns");
    }
  }
  
  /* Make space for the number of bytes written. We could just
   * write the padding bits in the last byte, but this is easier
   */
  for (offset = 0; offset < sizeof(unsigned long); offset++) {
    writeToBlock(outputBlock, 0);
  }

  /* Write the frequency table to the output block */
 writeFrequencyTableToBlock(frequencyTable, outputBlock);
  
  /* Encode */
  for (offset = 0; offset < inputBlock->usedSize; offset++) {
    unsigned char symbol = *(inputBlock->address + offset);
    unsigned patternLength = frequencyTable[symbol].huffmanBitCount;

     while (patternLength) {
       writeBitToBlock(outputBlock, ((1 << (patternLength-1)) & 
				     frequencyTable[symbol].huffmanBits) ?
		       True : False);
       patternLength--;
     }
  }

  bytesInFile.bytesInFile = inputBlock->usedSize;  
  for (offset = 0; offset < sizeof(unsigned long); offset++) {
    *(outputBlock->address + offset) = bytesInFile.ch[offset];
  }
  outputBlock->encoding = inputBlock->encoding | ENCODING_HUFFMAN;
  
  displayStatistics("Huffman compressing", inputBlock, outputBlock);
  return outputBlock;
}


BlockDescriptor* huffmanDecompress(BlockDescriptor* inputBlock) {
  FrequencyTable frequencyTable;
  HuffmanNode* huffmanNode;
  BlockDescriptor* outputBlock = NULL;

  union {
    unsigned long bytesInFile;
    char ch[sizeof(unsigned long)];
  } bytesInFile;

  size_t offset;
  if (!isHuffmanCompressed(inputBlock)) {
    return NULL;
  }

  /* Read the number of bytes in the original input file */
  for (offset = 0; offset < sizeof(unsigned long); offset++) {
    bytesInFile.ch[offset] = readFromBlock(inputBlock);
  }

  outputBlock = makeMemoryBlock(bytesInFile.bytesInFile);

  readFrequencyTableFromBlock(inputBlock, frequencyTable);
  
  huffmanNode = buildHuffmanTree(frequencyTable);

  while (bytesInFile.bytesInFile) { 
    unsigned char character;
    Boolean bitRead = readBitFromBlock(inputBlock);
    if (getHuffmanChar(bitRead, huffmanNode, &character)) {
      bytesInFile.bytesInFile--;
      writeToBlock(outputBlock, character);
    }
  }

  outputBlock->encoding = inputBlock->encoding & (~ENCODING_HUFFMAN);

  displayStatistics("Huffman decompressing", inputBlock, outputBlock);
  return outputBlock;
}
