#ifndef LIBRARY_H
#define LIBRARY_H

#include <stddef.h>
#include <stdio.h>

#define MAX_LENGHT 256 /* Max lenght for a line */
#define MAX_LENGHT_VAR 128 /* Max lenght of the variable part */
#define LENGHT_BOOKID 5 /* Number of characters of the bookid */
#define LENGHT_ISBN 16 /* Number of characters of the isbn */
#define LENGHT_FILE 64 /* Lenght of the name of the file */
#define POS_FILENAME 2 /* Position of the filename on the input */
#define LENGHT_CODE 16 /* Lenght of the code (ej. add, find, ...) */

#define FACTOR_MULT 2 /* realloc factor */
#define INITSIZE_INDEX 20 /* inital size of the index */

#define OK 0 /* Correct output */
#define ERR -1 /* Error */

/**
 * @brief Structure of the index of a record
 */
typedef struct _Indexbook
{
  int key;  /* book id */
  long int offset;  /* book is stored in disk at this position */
  size_t size;  /* book record size. This is a redundant field that helps in the  implementation */
} Indexbook;

short initIndexbook(Indexbook * index, int key, long int offset, size_t size);

/**
 * @brief Structure for the array of index
 */
typedef struct _Index
{
  Indexbook * index;
  size_t size;  /* Number of entries */
  size_t used;  /* Used entries */
} Index;

void initIndex(Index * index, size_t initSize);
short insertIndex(Index * index, int key, size_t size, long int offset);
void freeIndex(Index * index);
short insertBookInfo(char * array, Index * index, char * filename);
void printIndex(Index * index);
size_t binarySearchPositionToInsert(Index * index, size_t n, int key);
short indexFromFile(char * filename, Index * index);
short indexToFile(char * filename, Index * index);
short saveBookToFile(char * filename, int bookID, char * isbn, char * title, char * editorial, long int * offset, size_t * size);

/**
 * @brief Strucutre of the records
 */
typedef struct _Record
{
  long int offset;
  size_t size;
  int bookID;
  int isbn;
  char * title;
  char * printedBy;
} Record;

#endif