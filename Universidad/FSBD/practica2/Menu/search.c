/*
* Created by roberto on 3/5/21.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "search.h"

void    results_search(SQLHSTMT stmt, char * from, char *to, char *date,
                       int * n_choices, char *** choices,
                       int max_length,
                       int max_rows)
   /**here you need to do your query and fill the choices array of strings
 *
 * @param from form field from
 * @param to form field to
 * @param n_choices fill this with the number of results
 * @param choices fill this with the actual results
 * @param max_length output win maximum width
 * @param max_rows output win maximum number of rows
  */
{
  SQLRETURN ret; /* ODBC API return status */
  int t;
  char aux[1024];
  char from_param[4], to_param[4], date_param[11];

  SQLINTEGER flight_id;
  SQLCHAR    aircraft_code[4];
  SQLINTEGER conexion;
  SQLCHAR    scheduled_departure[64];
  SQLCHAR    scheduled_arrival[64];
  SQLINTEGER asientosvacios;
  SQLCHAR    time_elapsed[32];

  strncpy(from_param, from, 3);
  from_param[3] = '\0';
  strncpy(to_param, to, 3);
  to_param[3] = '\0';
  strncpy(date_param, date, 10);
  date_param[10] = '\0';

  SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, from_param, 0, NULL);
  SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, to_param, 0, NULL);
  SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 10, 0, date_param, 0, NULL);
  SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, from_param, 0, NULL);
  SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 3, 0, to_param, 0, NULL);
  SQLBindParameter(stmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 10, 0, date_param, 0, NULL);

  ret=SQLExecute(stmt);
  if (!SQL_SUCCEEDED(ret)) {
    odbc_extract_error("SQLExecute", stmt, SQL_HANDLE_STMT);
    *n_choices = 1;
    strncpy((*choices)[0], "Error: No se encontraron vuelos.", max_length);
    SQLCloseCursor(stmt);
    return;
  }

  SQLBindCol(stmt, 1, SQL_C_SLONG, &flight_id, 0, NULL);
  SQLBindCol(stmt, 2, SQL_C_CHAR,  aircraft_code, sizeof(aircraft_code), NULL);
  SQLBindCol(stmt, 3, SQL_C_SLONG, &conexion, 0, NULL);
  SQLBindCol(stmt, 4, SQL_C_CHAR,  scheduled_departure, sizeof(scheduled_departure), NULL);
  SQLBindCol(stmt, 5, SQL_C_CHAR,  scheduled_arrival, sizeof(scheduled_arrival), NULL);
  SQLBindCol(stmt, 6, SQL_C_SLONG, &asientosvacios, 0, NULL);
  SQLBindCol(stmt, 7, SQL_C_CHAR,  time_elapsed, sizeof(time_elapsed), NULL);
  
  /* Loop through the rows in the result-set */
  *n_choices = 0;
 while (SQL_SUCCEEDED(ret = SQLFetch(stmt)) && (*n_choices) < max_rows) {
      
      snprintf(aux, max_length, "%-6d %-4s %-2d %-25s %-25s %-4d %-10s",
                (int)flight_id,
                (char*)aircraft_code,
                (int)conexion,
                (char*)scheduled_departure,
                (char*)scheduled_arrival,
                (int)asientosvacios,
                (char*)time_elapsed);
    
    t = strlen(aux)+1;
    t = MIN(t, max_length);
    strncpy((*choices)[*n_choices], aux, t);
    (*n_choices)++;
  }

/*  
  char *query_result_set = NULL;
  if(!(query_result_set = (char *)malloc(sizeof(char) * max_rows)))
  {
          "1. Thou shalt have no other gods before me.",
          "2. Thou shalt not make unto thee any graven image,"
          " or any likeness of any thing that is in heaven above,"
          " or that is in the earth beneath, or that is in the water "
          "under the earth.",
          "3. Remember the sabbath day, to keep it holy.",
          "4. Thou shalt not take the name of the Lord thy God in vain.",
          "5. Honour thy father and thy mother.",
          "6. Thou shalt not kill.",
          "7. Thou shalt not commit adultery.",
          "8. Thou shalt not steal.",
          "9. Thou shalt not bear false witness against thy neighbor.",
          "10. Thou shalt not covet thy neighbour's house, thou shalt not"
          " covet thy neighbour's wife, nor his manservant, "
          "nor his maidservant, nor his ox, nor his ass, "
          "nor any thing that is thy neighbour's."
  };

  *n_choices = sizeof(query_result_set) / sizeof(query_result_set[0]);

  max_rows = MIN(*n_choices, max_rows);
  for (i = 0 ; i < max_rows ; i++) {
      t = strlen(query_result_set[i])+1;
      t = MIN(t, max_length);
      strncpy((*choices)[i], query_result_set[i], t);
  }
*/    

  if (*n_choices == 0) {
      strncpy((*choices)[0], "No se encontraron vuelos para esa ruta y fecha.", max_length);
      *n_choices = 1;
  }

  SQLCloseCursor(stmt);
}

