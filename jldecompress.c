/* jldecompress.c
 *
 * User interface for decompression.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "dataBlocks.h"
#include "header.h"
#include "compression.h"

const char* programName_g = "jldecompress";

int main(int argc, char** argv) {
  Boolean overwrite = False;
  const char* inputFilename = NULL;

  /* True if the output filename is stored in the heap and needs to be
   * freed before the program exits.
   */
  Boolean freeOutputFilename = False;
  char* outputFilename = NULL;

  /* For loop index */
  int index = 0;

  /* Parse command line */
  for (index = 1; index < argc; index++) {

    if (!strcmp(argv[index], "-h") ||
	     (!strcmp(argv[index], "--help"))) {
      printf("\n");
      printf("%s [switches] inputFilename [outputFilename]\n",
	     programName_g);
      printf("Switches:\n");
      printf("          -f or --force   Overwrite output file if it exists\n");
      printf("          -h or --help    Print this text\n");
      printf("\n");
      exit(0);
    }
    else if (!strcmp(argv[index], "-f") ||
	     !strcmp(argv[index], "--force")) {
      overwrite = True;
    }
    else if (*argv[index] != '-') {
      if (outputFilename) {
	error(False, "Too many filenames");
      }
      else if (inputFilename) {
	outputFilename = argv[index];
      }
      else {
        inputFilename = argv[index];
      }
    }
    else {
      error(False, "Unrecognised parameter %s", argv[index]);
    }
  }

  /* Find out if the file is compressed or not */
  if (!getCompressionFlags(inputFilename, True)) {
    printf("File %s is not compressed\n", inputFilename);
    exit(0);
  }

  /* If no output filename provided, then generate one. */
  if (outputFilename == NULL) {
    outputFilename = makeOutputFilename(inputFilename, False);
    freeOutputFilename = True;
  }

  /* Ensure output filename is not the same as the input filename */
  if (!strcmp(inputFilename, outputFilename)) {
    error(False, "cannot have same file for input and output");
  }

  /* Ensure the output file does not already exist */
  if (!overwrite) {
    struct stat fileStat;
    if (!stat(outputFilename, &fileStat)) {
      error(False, "output file %s already exists", outputFilename);
    }
  }

  decompress(inputFilename, outputFilename);

  displayFinalStatistics(inputFilename, outputFilename);
 
  /* Free heap storage */
  if (freeOutputFilename) {
    free(outputFilename);
    freeOutputFilename = False;
  }
 
  /* Always return success because program is aborted on error */
  return 0;
}

