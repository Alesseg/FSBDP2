/*
 Created by roberto on 3/5/21.
*/

#ifndef NCOURSES_BPASS_H
#define NCOURSES_BPASS_H

#include "windows.h"
#include <string.h>
/*#include <unistd.h>*/

/**/
#include <sql.h>
#include <sqlext.h>
/**/

void results_bpass(SQLHDBC dbc,
                   SQLHSTMT stmt_find_pending,
                   SQLHSTMT stmt_find_seat,
                   SQLHSTMT stmt_insert,
                   SQLHSTMT stmt_get_departure,
                   char * bookID, int * n_choices,
                   char *** choices, int max_length, int max_rows);

#endif /*NCOURSES_BPASS_H */
