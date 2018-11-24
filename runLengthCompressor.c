/* runLengthCompressor.c
 *
 * This contains the code for run length encoding and decoding.
 */

#include "compression.h"
#include "dataBlocks.h"
#include "header.h"

/* Use characters which don't often appear in text files, so that they
 * don't need to be escaped often.
 */

#define REPEAT_SYMBOL (235)
#define ESCAPE_SYMBOL (236)

/* writeByte()
 *
 * Writes a single byte to the output file, escaping it if necessary.
 *
 * Parameters:
 * outputBlock - descriptor of output block to write character to
 * character - byte to write
 */
static void writeByte(BlockDescriptor* outputBlock,
                      unsigned char character) {
  if ((character == REPEAT_SYMBOL) || (character == ESCAPE_SYMBOL)) {
    writeToBlock(outputBlock, ESCAPE_SYMBOL);
  }
  writeToBlock(outputBlock, character);
}

/* writeRepeat()
 *
 * Writes a repeated byte to the output file.
 *
 * Parameters:
 * outputBlock - descriptor of output block to write character to
 * repeatCount - number of times that the byte is repeated. 1 means that
 *               the character is repeated once - i.e. occurs twice
 * character - byte to write
 */
static void writeRepeat(BlockDescriptor* outputBlock,
                        unsigned repeatCount,
                        unsigned char character) {
  /* repeatCount of 1 means that the character is repeated once -
   * i.e. occurs twice.
   */
  if ((repeatCount < 3) &&
      (character != ESCAPE_SYMBOL)
      && (character != REPEAT_SYMBOL)) {
    /* If repeat count is less than three (i.e. the character appears
     * less than four times in succession) then this is the same
     * length as the repeat encoding so don't bother encoding. If the
     * characters would need escaping then use the repeat always
     * since <ESC>X<ESC>X is longer than <RPT>2X.
     */
    writeByte(outputBlock, character);
    while (repeatCount-- != 0) {
      writeByte(outputBlock, character);
    }
  }
  else {
      writeToBlock(outputBlock, REPEAT_SYMBOL);
      writeToBlock(outputBlock, repeatCount);
      writeToBlock(outputBlock, character);
  }
}

/* runLengthCompress()
 *
 * Run length encode the input block, returning a new block containing
 * run length encoded data.
 *
 * Parameters:
 * inputBlock - Descriptor of input block to encode
 *
 * Return value:
 * Pointer to heap allocated output block descripor pointing to
 * heap allocated block which has been encoded.
 */
BlockDescriptor* runLengthCompress(BlockDescriptor* inputBlock) {
  BlockDescriptor* outputBlock = makeMemoryBlock(inputBlock->allocatedSize);
  const unsigned char* charPointer = inputBlock->address;
  int lastCharacter = -1;
  unsigned char repeatCount = 0;
  unsigned long offset;

  if (isRleCompressed(inputBlock)) {
    error(False, "File already run length encoded");
  }

  outputBlock->encoding = inputBlock->encoding | ENCODING_RUN_LENGTH;

  lastCharacter = -1;
  for (offset = 0; offset < inputBlock->allocatedSize; offset++) {
    if (*charPointer == lastCharacter) {
      if (++repeatCount == 0x00)  {
	/* Repeat count overflowed */
	writeRepeat(outputBlock, 256, lastCharacter);
	repeatCount = 0;
	lastCharacter = -1;
      }
    }
    else if (repeatCount) {
      /* character changed */
      writeRepeat(outputBlock, repeatCount, lastCharacter);
      repeatCount = 0;
      lastCharacter = -1;
    }
    else if (lastCharacter != -1) {
      writeByte(outputBlock, lastCharacter);
    }
    lastCharacter = *charPointer++;
  }
  
  /* Handle last character in file */
  if (*charPointer == lastCharacter) {
    writeRepeat(outputBlock, repeatCount, lastCharacter);
    repeatCount = 0;
  }
  else if (repeatCount) {
    writeRepeat(outputBlock, repeatCount, lastCharacter);
    repeatCount = 0;
  }
  else if (lastCharacter != -1) {
    writeByte(outputBlock, lastCharacter);
  }

  displayStatistics("Run length encoding", inputBlock, outputBlock);
  return outputBlock;
}

/* runLengthDecompress()
 *
 * Run length decode the input block, returning a new block containing
 * decoded data.
 *
 * Parameters:
 * inputBlock - Descriptor of input block to decode
 *
 * Return value:
 * Pointer to heap allocated output block descriptor pointing to
 * heap allocated block which has been decoded.
 */
BlockDescriptor* runLengthDecompress(BlockDescriptor* inputBlock) {
  BlockDescriptor* outputBlock = makeMemoryBlock(inputBlock->usedSize);
  const unsigned char* charPointer = NULL;

  unsigned long offset = 0;

  if (!isRleCompressed(inputBlock)) {
    return NULL;
  }

  outputBlock->encoding = inputBlock->encoding & ~ENCODING_RUN_LENGTH;

  charPointer = inputBlock->address;
  offset = 0;
  while (offset < inputBlock->usedSize) {
    if (charPointer[offset] == ESCAPE_SYMBOL) {
      if (++offset >= inputBlock->usedSize) {
        error(False, "Damaged input file - ends with escape symbol");
      }
      writeToBlock(outputBlock, charPointer[offset]);
    }
    else if (charPointer[offset] == REPEAT_SYMBOL) {
      unsigned numberOfCharsToOutput = 0;
      unsigned count = 0;
      unsigned char repeatCount = 0;

      if (++offset >= inputBlock->usedSize) {
        error(False, "Damaged input file - ends with repeat symbol");
      }
      repeatCount = charPointer[offset];
      /* Repeat count means number of repeats of char - i.e. 1 means the
       * char occurs twice. As a repeat count of 0 is pointless, 0 means
       * the char repeats 255 times - i.e. there are 256 of them.
       */
      numberOfCharsToOutput = repeatCount;
      if (numberOfCharsToOutput == 0) {
        numberOfCharsToOutput = 255;
      }

      if (++offset >= inputBlock->usedSize) {
        error(False, "Damaged input file - ends with repeat count");
      }
      for (count = 0; count <= numberOfCharsToOutput; count++) {
        writeToBlock(outputBlock, charPointer[offset]);
      }

    }
    else {
      writeToBlock(outputBlock, charPointer[offset]);
    }
      offset++;
  }

  displayStatistics("Run length encoding", inputBlock, outputBlock);
  return outputBlock;
}
