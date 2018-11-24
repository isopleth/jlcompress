/* huffmanTree.c
 *
 * Code for constructing and manipulating the Huffman tree.
 */

#include <stdio.h>
#include "huffmanCompressor.h"
#include "compression.h"

/* A priority ordered queue is used to build the tree.
 * This is the code used for that. The queue is ordered
 * by descending frequency.
 */

/* Each priority order queue node looks like this */
typedef struct PQueue {
  struct PQueue* next;
  HuffmanNode* node;
} PriorityQueueNode;

/* Pointer to the head of the queue */
static PriorityQueueNode* head = NULL;

/* pqueueAdd()
 *
 * Adds a node to the tree.
 *
 * Parameters:
 * node - pointer to node to add.
 */
static void pqueueAdd(HuffmanNode* node) {
  if (head == NULL) {
    head = malloc(sizeof(PriorityQueueNode));
    head->next = NULL;
    head->node = node;
  }
  else {
    /* Zip down through list looking for where to stick it */

    PriorityQueueNode* newPriorityQueueNode = NULL; 
    PriorityQueueNode* thisPQueueEntry = head;
    PriorityQueueNode* previousPQEntry = NULL;
    while (thisPQueueEntry->node->frequency < node->frequency) {
      previousPQEntry = thisPQueueEntry;
      thisPQueueEntry = thisPQueueEntry->next;

      /* Hit end of queue */
      if (thisPQueueEntry == NULL) {
	break;
      }
    }

    newPriorityQueueNode = malloc(sizeof(PriorityQueueNode));
    if (newPriorityQueueNode == NULL) {
      error(True, "unable to malloc new priority queue node");
    }

    newPriorityQueueNode->node = node;
    if (previousPQEntry == NULL) {
      /* Insert at head */
      newPriorityQueueNode->next = head;
      head = newPriorityQueueNode;
    }
    else {
      newPriorityQueueNode->next = thisPQueueEntry;
      previousPQEntry->next = newPriorityQueueNode;
    }
  }
}

/* pqueuePop()
 *
 * Pops the entry at the front of the queue off of the queue.
 *
 * Return value:
 * Node from front of queue
 */
static HuffmanNode* pqueuePop(void) {
  PriorityQueueNode* thisPQueueEntry;
  HuffmanNode* node;
  if (head == NULL) {
    error(False, "Popping from empty queue");
    return NULL;
  }
  thisPQueueEntry = head;
  head = head->next;
  node = thisPQueueEntry->node;
  free(thisPQueueEntry);
  thisPQueueEntry = 0;
  return node;
}

/* buildHuffmanTree()
 *
 * Constructs Huffman coding tree from frequency table.
 *
 * Parameters:
 * frequencyTable - Frequency table array to build tree from
 *
 * Return value:
 * Root node of tree
 */

HuffmanNode* buildHuffmanTree(FrequencyTable frequencyTable) {
  HuffmanNode* returnNode;
  unsigned index;

  for (index = 0; index < FREQUENCY_TABLE_SIZE; index++) {
    if (frequencyTable[index].frequency) {

      HuffmanNode* node = malloc(sizeof(HuffmanNode));
      if (node == NULL) {
	error(True, "unable to malloc new huffman node");
      }

      node->left = NULL;
      node->right = NULL;
      node->symbol = frequencyTable[index].symbol;
      node->frequency = frequencyTable[index].frequency;
      pqueueAdd(node);
    }
  }

  /* While there is more than one node in the tree pull the
   * front two nodes off and replace them with a new node which
   * has them as children
   */
  while (head->next != NULL) {
    HuffmanNode* newNode = malloc(sizeof(HuffmanNode));
    newNode->right = pqueuePop();
    newNode->left = pqueuePop();
    newNode->frequency = newNode->left->frequency + newNode->right->frequency;
    pqueueAdd(newNode);
  }

  /* Now have a single node in the queue, and it is the root of the
   * whole tree
   */
  returnNode = head->node;
  free(head);
  head = NULL;

  return returnNode;
}

/* walkHuffmanTree
 *
 * Recursively walks the tree filling in the bit pattern and bit pattern
 * length fields in the frequency table. It deletes the table as it walks
 * it.  When it returns the frequency tabole will be fully populated with
 * the bit patterns needed to do the compression.
 *
 * Parameters:
 * node - root node of tree (and when called recursively the root node of
 *        the subtree to walk.
 * frequencyTable - Frequency table array
 * pattern - bit pattern accumulated so far (ignored by initial call since
 *           any value in it will be shifted into irrelevence, but
 *           specify as 0)
 * patternLength - bit pattern length accumulated so far (set to 0 for
 *           initial call).  Corresponds to current tree depth.
 */
void walkHuffmanTree(HuffmanNode* node, FrequencyTable frequencyTable,
		     unsigned pattern, unsigned patternLength) {

  if (node->left == NULL) {
    unsigned char symbol = node->symbol;

    if (frequencyTable[symbol].symbol  != symbol) {
      error(False, "Frequency table bad order");
    }
    if (frequencyTable[symbol].huffmanBits || 
	frequencyTable[symbol].huffmanBitCount) {      
      error(False, "Been here already");
    }
    if (sizeof(frequencyTable[symbol].huffmanBits) < 
	frequencyTable[symbol].huffmanBitCount) {
      error(False, "Huffman bit count field overflow");
    }
    frequencyTable[symbol].huffmanBits = pattern;
    frequencyTable[symbol].huffmanBitCount = patternLength;
    
  }
  else {
    /* It's just a jump to the left. */
    walkHuffmanTree(node->left, frequencyTable,
		    (pattern << 1) | 0, patternLength + 1);
    /* And then a step to the right. */
    walkHuffmanTree(node->right, frequencyTable,
		    (pattern << 1) | 1, patternLength + 1);
  }
  free(node);
}

/* getHuffmanChar()
 *
 * Use the next bit read from the file to walk one level of the
 * Huffman tree. If it reaches a leaf then it returns the character
 * at that leaf. It maintains an internal state recording where it has
 * reached in the tree, and resets the state when it reaches a leaf.
 *
 * Parameters:
 * bitRead - Bit read from file
 * huffmanNode - root node of Huffman tree (used in initial call
 *               and when resetting state)
 * character - Character at leaf - valid if return value is True.
 *
 * Return:
 * True if reached a leaf, False if still traversing tree
 */
Boolean getHuffmanChar(Boolean bitRead,
		       HuffmanNode* rootNode,
		       unsigned char* character) {
  static HuffmanNode* state = NULL;
  if (state == NULL) {
    state = rootNode;
  }

  state = bitRead ? state->right : state->left;

  if ((state->left == NULL) &&
      (state->right == NULL)) {
    *character = state->symbol;
    state = NULL;
    return True;
  }
  return False;
}
