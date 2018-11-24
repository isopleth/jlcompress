/*
 * This file contains the code for handling data blocks and the descriptors
 * generated to describe the data blocks. A data block is the entire contents
 * of a compressed or decompressed file.  Compressed files have the magic
 * number and encoding flags hidden from client code so that they can just
 * work on the compressed data.
 *
 * Ideally all access to the blocks would be through functions like this one
 * but for speed of implementation code using the blocks sometimes directly
 * accesses the blocks.  If it all went through functions like the ones here
 * the structure of the block descriptors could bemore easily improved.
 *
 * Input files are mapped rather than read into memory.
 */
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dataBlocks.h"
#include "header.h"
#include "compression.h"

/* makeBlockDescriptor()
 *
 * Constructs an empty block descriptor.
 */
static BlockDescriptor* makeBlockDescriptor(void) {

  BlockDescriptor* blockDescriptor = (BlockDescriptor*)malloc(sizeof(BlockDescriptor));
  if (blockDescriptor == NULL) {
    error(True, "malloc failed to make block descriptor in makeBlockDescriptor");
  }

  blockDescriptor-> address = NULL;
  blockDescriptor->allocatedSize = 0;

  blockDescriptor->usedSize = 0;

  blockDescriptor->nextFreeByte = 0;
  blockDescriptor->nextFreeBit = 0;

  blockDescriptor->nextByteToRead = 0;
  blockDescriptor->nextBitToRead = 0;

  blockDescriptor->fileDescriptor = 0;
  blockDescriptor->type = UNDEFINED_TYPE;
  blockDescriptor->encoding = 0;
  return blockDescriptor;
}



/***** Creating and mapping blocks *****/

/* mapUncompressedFile()
 *
 * Maps an uncompressed file and returns a newly constructed data block
 * describing it.
 *
 * Parameters:
 * filename - filename of uncompressed file
 *
 * Return value:
 * Block descriptor describing mapped file
 */

BlockDescriptor* mapUncompressedFile(const char* filename) {
  BlockDescriptor* blockDescriptor = makeBlockDescriptor();

  blockDescriptor->allocatedSize = getFileSize(filename);

  blockDescriptor->fileDescriptor =  open(filename, O_RDONLY, 0);
  if (!blockDescriptor->fileDescriptor) {
    error(True, "Unable to open file %s", filename); 
  }

  /* mmap() to save memory - sections of the file get paged in
   * by the operating system as we need them.
   */
  blockDescriptor->address = (unsigned char*)mmap(NULL,
                                  blockDescriptor->allocatedSize,
                                  PROT_READ,
                                  MAP_PRIVATE,
				  blockDescriptor->fileDescriptor,
                                  0);
  if (blockDescriptor->address == MAP_FAILED) {
    error(True, "Unable to map file %s", filename); 
  }

  blockDescriptor->usedSize = blockDescriptor->allocatedSize;
  blockDescriptor->type = UNCOMPRESSED_FILE_TYPE;

  return blockDescriptor;
}

/* mapCompressedFile()
 *
 * Maps a compressed file and returns a newly constructed data block
 * describing it. The difference between a compressed file mapping and
 * an uncompressed file is that the code using the compressed file doesn't
 * see the magic number at the start or the compression flags.  This means
 * that decompression functions can be run without carring whether they are
 * the first one to touch the file and have to skip over this stuff.
 *
 * Parameters:
 * filename - filename of uncompressed file
 *
 * Return value:
 * Block descriptor describing mapped file
 */

BlockDescriptor* mapCompressedFile(const char* filename) {
  BlockDescriptor* blockDescriptor = makeBlockDescriptor();
  unsigned char* address = NULL;

  unsigned fileSize = getFileSize(filename);
  if (fileSize < getHeaderSize()) {
      error(False, "File too small to be a compressed file");
  }
  else {
    blockDescriptor->encoding = getCompressionFlags(filename, False);
  }

  blockDescriptor->fileDescriptor =  open(filename, O_RDONLY, 0);
  if (!blockDescriptor->fileDescriptor) {
    error(True, "Unable to open file %s", filename); 
  }

  /* mmap() to save memory - bits of the file get paged in
   * as we need them.
   */
  address = (unsigned char*)mmap(NULL,
                 fileSize,
                 PROT_READ,
                 MAP_PRIVATE,
                 blockDescriptor->fileDescriptor,
                 0);
  if (blockDescriptor->address == MAP_FAILED) {
    error(True, "Unable to map file %s", filename); 
  }

/* Adjust the block descriptor to point to the data in the file
 * skipping the header. In principle we could use the last parameter
 * of mmap() to do this but that seems to need to operate on page
 * boundaries.
 */

  blockDescriptor->type = COMPRESSED_FILE_TYPE;

  /* Adjust things so that the user of the block doesn't see the
   * header.
   */

  blockDescriptor->address = address + getHeaderSize();
  blockDescriptor->allocatedSize = fileSize - getHeaderSize();
  blockDescriptor->usedSize = blockDescriptor->allocatedSize;
  return blockDescriptor;
}

/* makeMemoryBlock()
 *
 * This allocates a memory block of the specified size and creates a
 * descriptor block for it.
 *
 * Parameters:
 * size - size of the block in bytes
 *
 * Return value:
 * descriptor block for newly allocated block
 */
extern BlockDescriptor* makeMemoryBlock(size_t size) {
  BlockDescriptor* blockDescriptor = makeBlockDescriptor();
  blockDescriptor->address = malloc(size);
  if (blockDescriptor == NULL) {
    error(True, "malloc failed in makeMemoryBlock");
  }

  blockDescriptor->allocatedSize = size;
  blockDescriptor->type = MEMORY_TYPE;
  blockDescriptor->encoding = 0;
  return blockDescriptor;
}

/* freeBlock()
 *
 * This deallocates a memory block, or unmaps a file.  It uses information
 * stored in the block descriptor to tell it what to do.  It then deallocates
 * the block descriptor.
 *
 * Parameters:
 * blockDescriptor - block descriptor describing block, and to be deallocated
 *    itself.
 *
 * Return value:
 * descriptor block for newly allocated block
 */
extern void freeBlock(BlockDescriptor* blockDescriptor) {

  if (blockDescriptor != NULL) {
  switch (blockDescriptor->type) {
  case MEMORY_TYPE:
    free(blockDescriptor->address);
    break;

  case COMPRESSED_FILE_TYPE:
    /* Adjust address and size to add back in header */
    blockDescriptor->address -= getHeaderSize();
    blockDescriptor->allocatedSize += getHeaderSize();
    /* DELIBERATELY RUN ONTO NEXT CASE STATEMENT */

  case UNCOMPRESSED_FILE_TYPE:
    if (munmap((void*)blockDescriptor->address,
                     blockDescriptor->allocatedSize) == -1) {
      error(True, "Unable to unmap file");
    }
    if (close(blockDescriptor->fileDescriptor)) {
      error(True, "Unable to close file");
    }
    break;

  default:
    error(False, "Illegal block type in freeBlock() - %d\n",
          blockDescriptor->type);
    break;
  }
  free(blockDescriptor);
  }
}

/***** Reading and writing data ******/

/* writeToBlock()
 *
 * This writes a byte to a block, recording in the block descriptor the
 * next free location for the following write.  It also resizes the block
 * if necessary.
 *
 * Parameters:
 * blockDescriptor - block descriptor describing block to be written to
 * character - byte to write
 *
 * Return value:
 * Offset into block of next unused byte in it.
 */

size_t writeToBlock(BlockDescriptor* blockDescriptor, unsigned char character) {
  size_t returnIndex = 0;
  if (blockDescriptor->nextFreeByte >= blockDescriptor->allocatedSize) {
    size_t newSize = blockDescriptor->allocatedSize + ((blockDescriptor->allocatedSize + 1) / 2);
    blockDescriptor->address = realloc(blockDescriptor->address, newSize);
    if (blockDescriptor == NULL) {
      error(True, "realloc failed for new size %lu", (size_t)newSize);
    }
    blockDescriptor->allocatedSize = newSize;
  }
  returnIndex = blockDescriptor->nextFreeByte;
  blockDescriptor->address[blockDescriptor->nextFreeByte++] = character;
  blockDescriptor->usedSize = blockDescriptor->nextFreeByte;
  return returnIndex;
}

/* writeBitToBlock()
 *
 * This appends a bit to a block.
 *
 * Parameters:
 * blockDescriptor - descriptor of block to write to.
 * value - bit value to write
 */
void writeBitToBlock(BlockDescriptor* blockDescriptor, Boolean value) {
    /* See if we need to enlarge the block. Only need to check when
     * nextFreeBit is 0 because otherwise we will be writing to a byte
     * we have already written to, so we know it is OK
     */
  if (blockDescriptor->nextFreeBit == 0) {
    blockDescriptor->usedSize = blockDescriptor->nextFreeByte + 1;
    if (blockDescriptor->nextFreeByte >= blockDescriptor->allocatedSize) {
      size_t newSize = blockDescriptor->allocatedSize + ((blockDescriptor->allocatedSize + 1) / 2);
      blockDescriptor->address = realloc(blockDescriptor->address, newSize);
      if (blockDescriptor == NULL) {
	error(True, "realloc failed for new size %lu", (size_t)newSize);
      }
      blockDescriptor->allocatedSize = newSize;
    }
  }

  /* Set the next bit to the desired value */
  setBit(blockDescriptor->nextFreeBit,
         blockDescriptor->address + blockDescriptor->nextFreeByte,
         value);
  
  /* Move onto the next bit next time */
  blockDescriptor->nextFreeBit++;
  if (blockDescriptor->nextFreeBit > 7) {
    /* Bit rolled over. Move onto next byte */
    blockDescriptor->nextFreeBit = 0;
    blockDescriptor->nextFreeByte++;
  }
}

/* readBitFromBlock()
 *
 * This reads the next bit from the block.
 *
 * Parameters:
 * blockDescriptor - descriptor of block to write to.
 *
 * Return value:
 * Bit read
 */
Boolean readBitFromBlock(BlockDescriptor* inputBlock) {
  Boolean value;
  if (inputBlock->nextByteToRead >= inputBlock->allocatedSize) {
    error(False, "attempt to read past end of data block");
  }

  value = getBit(inputBlock->nextBitToRead++,
                 *(inputBlock->address + inputBlock->nextByteToRead));
  
  if (inputBlock->nextBitToRead > 7) {
    inputBlock->nextBitToRead = 0;
    inputBlock->nextByteToRead++;
  }
  return value;
}

/* readBitFromBlock()
 *
 * This reads the next byte from the block.
 *
 * Parameters:
 * blockDescriptor - descriptor of block to write to.
 *
 * Return value:
 * Byte read
 */
unsigned char readFromBlock(BlockDescriptor* inputBlock) {
  if (inputBlock->nextByteToRead >= inputBlock->allocatedSize) {
    error(False, "attempt to read past end of data block");
  }
  
  return *(inputBlock->address + inputBlock->nextByteToRead++);
}

/* createFile()
 *
 * This creates a file and writes the data block to it.
 *
 * Parameters:
 * filename - output filename
 * blockDescriptor - descriptor of block to write
 * outputHeader - True if the magic number and header are to be
 *   written - i.e. this is a compressed file. False for uncompressed
 *  file.
 */

void createFile(const char* filename,
                BlockDescriptor* blockDescriptor,
                Boolean outputHeader) {
  FILE* outputFile = fopen(filename, "wb");

  if (outputFile == NULL) {
    error(True, "Unable to create %s", filename);
  }

  if (outputHeader) {
    writeHeader(outputFile, blockDescriptor);
  }

  if (fwrite(blockDescriptor->address, 1,
             blockDescriptor->usedSize, outputFile) !=
      blockDescriptor->usedSize) {
    error(True, "Unable to write to %s", filename);
  }
  if (fclose(outputFile) == EOF) {
    error(True, "Unable to close %s", filename);
  }
}

/* displayStatistics()
 *
 * Display some statistics about how the data has grown or shrunk.
 *
 * Parameters:
 * operation - Brief description of the operation performed
 * originalBlock - original block
 * finalBlock - modified block
 */
void displayStatistics(const char* operation,
		       const BlockDescriptor* originalBlock,
		       const BlockDescriptor* finalBlock) {
  size_t originalSize = originalBlock->usedSize;
  size_t finalSize = finalBlock->usedSize;

  float percentage = (100-(100.*(float)finalSize/(float)originalSize));
  printf("- %s - in %lu bytes, out %lu bytes - %s %4.1f%%\n",
	 operation, (unsigned long)originalSize,
         (unsigned long)finalSize, 
	 originalSize >= finalSize ? "saving" : "bigger by",
	 fabs(percentage));
}

/* getBit()
 *
 * Get a bit from a byte
 *
 * Parameters:
 * bitNumber - bit number 0-7.
 * byte - pointer to byte to be modified
 *
 * Return:
 * Value of bit
 */
Boolean getBit(unsigned char bitNumber,
               unsigned char byte) {
  unsigned char mask = 1 << bitNumber;
  return (byte & mask) ? True : False;
}

/* setBit()
 *
 * Set a bit in a byte
 *
 * Parameters:
 * bitNumber - bit number 0-7.
 * byte - pointer to byte to be modified
 * value - new value for bit.
 */
void setBit(unsigned char bitNumber,
            unsigned char* byte,
            Boolean value) {
  unsigned char mask = 1 << bitNumber;
  *byte = value ? (*byte | mask) : (*byte & ~mask);
}
