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
                       int max_rows,
                       char *** msg,
                       char ***result,
                       int *rows_result)
/** here you need to do your query and fill the choices array of strings
 *
 * @param from form field from
 * @param to form field to
 * @param n_choices fill this with the number of results
 * @param choices fill this with the actual results
 * @param max_length output win maximum width
 * @param max_rows output win maximum number of rows
 * @param msg pointer to the message
  */
{
  SQLRETURN ret; /* ODBC API return status */
  int t = 0;
  char aux[1024];
  char from_param[4], to_param[4], date_param[11];

  SQLINTEGER flight_id_1, flight_id_2;
  SQLCHAR    aircraft_code_1[4], aircraft_code_2[4];
  SQLINTEGER conexion;
  SQLCHAR    scheduled_departure_1[64], scheduled_departure_2[64];
  SQLCHAR    scheduled_arrival_1[64], scheduled_arrival_2[64];
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

  SQLBindCol(stmt, 1, SQL_C_SLONG, &flight_id_1, 0, NULL);
  SQLBindCol(stmt, 2, SQL_C_CHAR,  aircraft_code_1, sizeof(aircraft_code_1), NULL);
  SQLBindCol(stmt, 3, SQL_C_SLONG, &conexion, 0, NULL);
  SQLBindCol(stmt, 4, SQL_C_CHAR,  scheduled_departure_1, sizeof(scheduled_departure_1), NULL);
  SQLBindCol(stmt, 5, SQL_C_CHAR,  scheduled_arrival_2, sizeof(scheduled_arrival_2), NULL);
  SQLBindCol(stmt, 6, SQL_C_SLONG, &asientosvacios, 0, NULL);
  SQLBindCol(stmt, 7, SQL_C_CHAR,  time_elapsed, sizeof(time_elapsed), NULL);
  SQLBindCol(stmt, 8, SQL_C_SLONG, &flight_id_2, 0, NULL);
  SQLBindCol(stmt, 9, SQL_C_CHAR,  scheduled_departure_2, sizeof(scheduled_departure_2), NULL);
  SQLBindCol(stmt, 10, SQL_C_CHAR,  scheduled_arrival_1, sizeof(scheduled_arrival_1), NULL);
  SQLBindCol(stmt, 11, SQL_C_CHAR,  aircraft_code_2, sizeof(aircraft_code_2), NULL);

  /* Loop through the rows in the result-set */
  /* Go to the starting row of the result wanted */
  *n_choices = 0;
  (*rows_result) = 0;
  while (SQL_SUCCEEDED(ret = SQLFetch(stmt)) && (*rows_result) < TOTAL_ROWS) {
      
      snprintf(aux, max_length, "%-6d %-4s %-2d %-25s %-25s %-4d %-10s",
                (int)flight_id_1,
                (char*)aircraft_code_1,
                (int)conexion,
                (char*)scheduled_departure_1,
                (char*)scheduled_arrival_2,
                (int)asientosvacios,
                (char*)time_elapsed);


    t = strlen(aux)+1;
    t = MIN(t, max_length);
    strncpy((*result)[(*rows_result)], aux, t);
      
    if(conexion == 1) {
      snprintf(aux, LENGTH_ROWS, "Flight id(1): Aircraft code(1): Flight id(2): Aircraft code(2): Departure(1):        Arrival(1):          Departure(2):        Arrival(2): %-13d %-17s %-13d %-17s %-20s %-20s %-20s %-20s", 
                (int)flight_id_1,
                (char*)aircraft_code_1,
                (int)flight_id_2,
                (char*)aircraft_code_2,
                (char*)scheduled_departure_1,
                (char*)scheduled_arrival_1,
                (char*)scheduled_departure_2,
                (char*)scheduled_arrival_2);
    } else {
      snprintf(aux, LENGTH_ROWS, "Flight id:  Aircraft code:  Departure:                Arrival: %-11d %-15s %-25s %-25s",
                (int)flight_id_1,
                (char*)aircraft_code_1,
                (char*)scheduled_departure_1,
                (char*)scheduled_arrival_1);
    }
    
    strncpy((*msg)[(*rows_result)], aux, strlen(aux)+1);
    (*rows_result)++;
  }
  (*n_choices) = 0;
  while((*n_choices) < (*rows_result) && (*n_choices) < max_rows && (*n_choices) < TOTAL_ROWS) {
    t = strlen((*result)[(*n_choices)])+1;
    t = MIN(t, max_length);
    strncpy((*choices)[(*n_choices)], (*result)[(*n_choices)], t);
    (*n_choices)++;
  }

  if (*n_choices == 0) {
      strncpy((*choices)[0], "No se encontraron vuelos para esa ruta y fecha.", max_length);
      *n_choices = 1;
  }

  SQLCloseCursor(stmt);
}

