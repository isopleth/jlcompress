#include <stdio.h>
#include "dataBlocks.h"
#include "compression.h"
#include "header.h"

/* flipBlock
 *
 * Flip the data in the block so that the block starts with all the
 * most significant bits concatenated together, then the next most
 * significant bits etc.
 *
 * Parameters:
 * inputBlock - descriptor of block to be flipped
 *
 * Return:
 * Output block descriptor for new block with bit order flipped
 */
static BlockDescriptor* flipBlock(BlockDescriptor* inputBlock) {
  BlockDescriptor* outputBlock;
  unsigned long offset;
  unsigned bitNumber = 8;
  outputBlock = makeMemoryBlock(inputBlock->usedSize);
  while (bitNumber-- != 0) {
    for (offset = 0; offset < inputBlock->usedSize; offset++) {
      writeBitToBlock(outputBlock,
                      getBit(bitNumber, *(inputBlock->address + offset)));
    }
  }
  inputBlock->usedSize = outputBlock->usedSize;

  displayStatistics("Flipping bit order", inputBlock, outputBlock);
  return outputBlock;
}


/* unflipBlock
 *
 * Reverse the effect of flipBlock()
 *
 * Parameters:
 * inputBlock - descriptor of block to be unflipped
 *
 * Return:
 * Output block descriptor for new block with bit order
 * returned to original state
 */
static BlockDescriptor* unflipBlock(BlockDescriptor* inputBlock) {
  BlockDescriptor* outputBlock;
  unsigned long offset;
  unsigned bitNumber = 8;

  outputBlock = makeMemoryBlock(inputBlock->usedSize);
  while (bitNumber-- != 0) {
    for (offset = 0; offset < inputBlock->usedSize; offset++) {
      Boolean bit = readBitFromBlock(inputBlock);
      setBit(bitNumber, outputBlock->address + offset, bit);
    }
  }
  outputBlock->usedSize = inputBlock->usedSize;
  
  displayStatistics("Unflipping bit order", inputBlock, outputBlock);
  return outputBlock;
}


/* flipBitOrder()
 *
 * Convert input block to flipped input block and set ENCODING_FLIPPED flag.
 * The "compression" will always produce a block the same size as the original
 * since the number of bits does not change.
 *
 * Parameters:
 * inputBlock - block to be flipped
 *
 * Return value:
 * resulting flipped block
 */
BlockDescriptor* flipBitOrder(BlockDescriptor* inputBlock) {
 
  BlockDescriptor* outputBlock = NULL;
  if (isFlipped(inputBlock)) {
    error(False, "File already has bit order flipped");
  }

  outputBlock = flipBlock(inputBlock);

  outputBlock->encoding = inputBlock->encoding | ENCODING_FLIPPED;
  return outputBlock;
}


/* unflipBitOrder()
 *
 * Convert input block back to having its bit order unflipped and clear
 * ENCODING_FLIPPED flag. The "compression" will always be 0 since the
 * number of bits does not change.
 * The "decompression" will always produce a block the same size as the original
 * since the number of bits does not change.
 *
 * Parameters:
 * inputBlock - block to be flipped
 *
 * Return value:
 * resulting flipped block
 */
BlockDescriptor* unflipBitOrder(BlockDescriptor* inputBlock) {
  BlockDescriptor* outputBlock = NULL;

  if (!isFlipped(inputBlock)) {
    return NULL;
  }

  outputBlock = unflipBlock(inputBlock);
  outputBlock->encoding = inputBlock->encoding & (~ENCODING_FLIPPED);
  return outputBlock;
}
