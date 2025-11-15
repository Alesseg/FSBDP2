#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "library.h"


int main() {
  char input[MAX_LENGHT]; /* Tamaño a comprobar */
  Index  * index = NULL;

  /* Print messages for test */
  printf("Type command and argument/s.\n");
  printf("exit\n");

  /* Allocate memory for the index */
  if(!(index = malloc(sizeof(Index)))) {
    printf("Error allocating memory for the index\n");
    return 1;
  }

  initIndex(index, INITSIZE_INDEX);

  /* Gets the input */
  while(fgets(input, MAX_LENGHT ,stdin) != NULL) {
    /* Extract the information */
    if(insertBookInfoIndex(input, index) == ERR) {
      printf("Nothing inserted\n");
    }
    printIndex(index);
  }

  freeIndex(index);
  free(index);

  return 0;
}

/**
 * @brief Initialice the indexbook of a record. This function does not allocates memory
 * 
 * @param index pointer to the indes
 * @param key value of the key (bookID)
 * @param offset value of the offset (position in the file)
 * @param size size of the index
 */
short initIndexbook(Indexbook * index, int key, long int offset, size_t size) {
  /* Error control */
  if(!index || key < 0 || offset < 0 || size <= 0) return ERR;

  /* Init the index */
  index->key = key;
  index->offset = offset;
  index->size = size;
  return OK;
}

/*****************************************
 * Puede que se tenga que ampliar la fucnión 'getBookInfo'
 *  al implementar el interfaz de usuario y se
 *  tenga que obetenr la info del libre
 * 
*/
/**
 * @brief Get the Book Info from a char and insert it into the index
 * 
 * @param array pointer to the array
 * @param index pointer to the index
 * @return OK, or ERR in case of error
 */
short insertBookInfoIndex(char * array, Index * index) {
  /* Error control */
  if(!array || !index) return ERR;

  char * bookID;
  char * isbn;
  char * title;
  char * edit;
  
  /* Gets the book id */
  bookID = strtok(array, "|");
  /* Gets the book id */
  isbn = strtok(NULL, "|");
  /* Gets the book id */
  title = strtok(NULL, "|");
  /* Gets the book id */
  edit = strtok(NULL, "\r");

  int key = atoi(bookID);
  size_t size = strlen(bookID) + strlen(isbn) + strlen(title) + strlen(edit) - 1;

  insertIndex(index, key, size);

  return OK;
}
/**
 * @brief Create initial empty array of size initSize. This function does NOT allocates memory
 * 
 * @param index pointer to the structure
 * @param initSize initial size of the array
 */
void initIndex(Index * index, size_t initSize) {
  if(!index) return;

  /* Init the index */
  if(!(index->index = malloc(sizeof(Indexbook) * initSize))) {
    free(index);
    return;
  }
  index->size = initSize;
  index->used = 0;
}
/**
 * @brief Free memory allocated for the index
 * 
 * @param index pointer to the index
 */
void freeIndex(Index * index) {
  if(index) {
    if(index->index) free(index->index);
    index->index = NULL;
    index->size = 0;
    index->used = 0;
  }
}
/**
 * @brief Insert an indexbook into the index respecting the order
 * 
 * @param index pointer to the indes
 * @param key bookID
 * @param size size of the indexbook
 * @return OK, or ERR in case of error
 */
short insertIndex(Index * index, int key, size_t size) {
  /* Error control */
  if(!index || key < 0 || size <= 0) return ERR;

  /* If there is no space, add more */
  if(index->used == index->size) {
    index->size *= FACTOR_MULT;
    if(!(index->index = realloc(index->index, index->size * sizeof(Indexbook)))) return ERR;
  }

  /* Search the position where the index must be inserted */
  long int pos = binarySearchPositionToInsert(index, index->used, key);
  /* Move the indexbook that are after the new indexbook */
  for(int i = index->used; i > pos ; i--) {
    index->index[i] = index->index[i - 1];
    index->index[i].offset += size + sizeof(size_t);
  }

  /* If is the first element the offset is 0, in the other cases we have to add the size of the offset */
  int offset = 0;
  if(pos)  {
    offset = index->index[pos - 1].offset + index->index[pos - 1].size + sizeof(size_t);
  }
  /* Initialice the index of the record */
  if(initIndexbook(&(index->index[pos]), key, offset, size) == ERR) return ERR;
  
  index->used++;  
  return OK;
}
/**
 * @brief Print the index
 * 
 * @param index pointer to the index
 */
void printIndex(Index * index) {
  /* Error control */
  if(!index) {
    return;
  }
  for(int i = 0; i < (int)index->used ;i++) {
    printf("Entry #%d\n", i);
    printf("\tkey: #%d\n", index->index[i].key);
    printf("\toffset: #%ld\n", index->index[i].offset);
  }
}

/**
 * @brief Search the position where the new index must be inserted
 * 
 * @param index pointer to the index
 * @param n number of used entities (nº index)
 * @param key bookID of the new index
 * @return Position where the index must be inserted.
 */
long int binarySearchPositionToInsert(Index * index, size_t n, int key) {
    int low = 0;
    int high = n - 1;

    while (low <= high) {
        int mid = low + (high - low) / 2;  // avoid overflow

        if (index->index[mid].key == key) {
            return mid;           // key found at index mid
        }
        else if (index->index[mid].key < key) {
            low = mid + 1;        // search in right half
        }
        else {
            high = mid - 1;       // search in left half
        }
    }

    return low;  // Position to insert
}

