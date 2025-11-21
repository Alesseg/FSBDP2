#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "library.h"


int main(int argc, char * argv[]) {
  char input[MAX_LENGHT];
  char * bookInfo = NULL;
  char * strategy = NULL;
  Index  * index = NULL;
  
  /* Check arguments */
  if(argc < 2) {
    printf("Missing argument\n");
    return 0;
  }
  /* Get the strategy */
  strategy = strtok(argv[1], " \r\n");
  if(!strcmp(strategy, "best_fit")) 
  {

  } else if (!strcmp(strategy, "first_fit")) 
  {

  } else if (!strcmp(strategy, "worst_fit")) 
  {

  } else {
    printf("Unknown search strategy unknown_search_strategy\n");
    return 0;
  }



  /* Print messages for test */
  printf("Type command and argument/s.\n");
  printf("exit\n");
  /* Allocate memory for the index */
  if(!(index = malloc(sizeof(Index)))) {
    printf("Error allocating memory for the index\n");
    return 0;
  }

  initIndex(index, INITSIZE_INDEX);

  /* main loop */
  short status = OK;
  while(1) {
    
    /* Get the input fron the terminal */
    fgets(input, MAX_LENGHT ,stdin);
    
    /* Extract the command and information from the input */
    char * command = NULL;
    command = strtok(input, " \n\r");
    bookInfo = strtok(NULL, "\n\r");

    if(strcmp(command, "add") == 0)
    {
      status = insertBookInfoIndex(bookInfo, index);
    } else if (strcmp(command, "find") == 0) 
    {
    } else if (strcmp(command, "del") == 0) 
    {
    } else if (strcmp(command, "exit") == 0) 
    {
      break;
    } else if (strcmp(command, "printInd") == 0) 
    {
      printIndex(index);
    } else if (strcmp(command, "printLst") == 0) 
    {
    } else if (strcmp(command, "printRec") == 0) 
    {
    } else 
    {
    }
    if(status == ERR) {
      printf("Error\n");
      break;
    }
    printf("exit\n");
    fflush(stdout);
  }

  /* When the user exits or there is an error, the index must be saved in the file and the memory must be freed */
  indexToFile(argv[POS_FILENAME], index);
  freeIndex(index);
  free(index);

  return 0;
}


/**
 * Functions for index 
 */
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
/**
 * @brief Get the Book Info from a char and insert it into the index
 * @details Puede que se tenga que ampliar esta función
 *  al implementar el interfaz de usuario y se tenga que obetener tmb la info del libro al completo
 * 
 *  ej. 12346|978-2-12345086-3|La busca|Catedra\r
 *  bookID -> 12346, isbn -> 978-2-12345086, title -> La busca, editorial -> Catedra
 *  This info is inserted intot the index
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
  size_t size = strlen(bookID) + strlen(isbn) + strlen(title) + strlen(edit);

  insertIndex(index, key, size);
  printf("Record with BookID=%d has been added to the database\n", key);

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
    printf("    key: #%d\n", index->index[i].key);
    printf("    offset: #%ld\n", index->index[i].offset);
    //printf("    size: #%zu\n", index->index[i].size);
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
/**
 * @brief Insert into an index the data from a file
 * 
 * @param filename pointer to the name of the file
 * @param index pointer to the index
 * @return OK, or ERR in case of error
 */
short indexFromFile(char * filename, Index * index) {
  FILE * f = NULL;
  long int offset = 0;
  int key = 0;
  size_t size = 0;
  char filenameIndex[LENGHT_FILE];

  /* Error control */
  if(!filename || !index) return ERR;

  /* Open file */
  sprintf(filenameIndex, "%s.ind", filename);
  if(!(f = fopen(filenameIndex, "rb"))) return ERR;

  /* Read file and insert data into index */
  while(fread(&key, sizeof(int), 1, f) > 0) {  
    fread(&offset, sizeof(long int), 1, f);
    fread(&size, sizeof(size_t), 1, f);
    if(insertIndex(index, key, size) == ERR){
      fclose(f);
      return ERR;
    }
  }

  fclose(f);
  return OK;
}
/**
 * @brief Write the data from the index in a file
 * 
 * @param filename pointer to the name of the file
 * @param index pointer to the index
 * @return OK, or ERR in case of error
 */
short indexToFile(char * filename, Index * index) {
  FILE * f = NULL;
  char filenameIndex[LENGHT_FILE];

  /* Error control */
  if(!filename || !index) return ERR;

  /* Open file */
  sprintf(filenameIndex, "%s.ind", filename);
  if(!(f = fopen(filenameIndex, "rb"))) return ERR;

  for (size_t i = 0; i < index->used; i++)
  {  
    if(fwrite(&index->index[i].key, sizeof(int), 1, f) <= 0) {
      fclose(f);
      return ERR;
    }
    if(fwrite(&index->index[i].offset, sizeof(long int), 1, f) <= 0) {
      fclose(f);
      return ERR;
    }
    if(fwrite(&index->index[i].size, sizeof(size_t), 1, f) <= 0) {
      fclose(f);
      return ERR;
    }
  }

  fclose(f);
  return OK;
}









