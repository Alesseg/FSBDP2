/*
 Created by roberto on 3/5/21.
*/

#ifndef NCOURSES_SEARCH_H
#define NCOURSES_SEARCH_H
#include "windows.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"
/*#include <unistd.h>*/

/*stmt como primer parámetro*/

void results_search(SQLHSTMT stmt, char * from, char *to, char *date, int * n_choices,
                    char *** choices, int max_length, int max_rows);

#endif /*NCOURSES_SEARCH_H*/
