/*
* Created by roberto on 3/5/21.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "search.h"

void    results_search(char * from, char *to, char *date,
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
  SQLHENV env;
  SQLHDBC dbc;
  SQLHSTMT stmt;
  SQLRETURN ret; /* ODBC API return status */
  SQLCHAR flight_id[4096], code[4096], conexion[4096], dep[4096], arr[4096], asientos[4096], time_elapsed[4096];
  int t;
  char aux[1000];
  char query[]= "with 	tabla_1\n"
    "AS (SELECT flight_id,\n"
                "Count (flight_id) AS n_people\n"
        "FROM   boarding_passes\n"
        "GROUP  BY flight_id),\n"
      "totalPeople\n"
    "AS (SELECT f.flight_id,\n"
                "COALESCE(n_people, 0) AS numpeople\n"
        "FROM   flights f\n"
                "LEFT JOIN tabla_1 t\n"
                      "ON f.flight_id = t.flight_id),\n"
      "totalSeats\n"
    "AS (SELECT aircraft_code,\n"
                "Count (seat_no) AS numseats\n"
        "FROM   seats\n"
        "GROUP  BY aircraft_code),\n"
      "totalfree\n"
    "AS (SELECT flight_id,\n"
                "( numseats - numpeople ) AS asientosvacios\n"
        "FROM   totalPeople\n"
                "natural JOIN flights\n"
                "natural JOIN totalSeats),\n"
  "flight_dir \n"
  "as   (select distinct flight_id,\n"
            "aircraft_code,\n"
            "0 as conexion,\n"
            "scheduled_departure,\n"
            "scheduled_arrival,\n"
            "asientosvacios,\n"
            "(scheduled_arrival - scheduled_departure) as time_elapsed\n"
    "from	  flights\n"
            "natural join totalfree\n"
    "where   departure_airport = ?\n"
            "and arrival_airport = ?\n"
            "and (scheduled_arrival - scheduled_departure) < '24:00:00'\n"
            "and scheduled_departure::date = ?\n"
            "and asientosvacios > 0\n"
    "),\n"
        "flight_sec\n"
        "as   (select distinct f1.flight_id,\n"
            "f1.aircraft_code,\n"
            "1 as conexion,\n"
            "f1.scheduled_departure,\n"
            "f2.scheduled_arrival,\n"
            "min(asientosvacios) as asientosvacios,\n"
            "(f2.scheduled_arrival - f1.scheduled_departure) as time_elapsed\n"
    "from    flights f1,\n"
            "flights f2,\n"
            "totalfree tf\n"
    "where   f1.departure_airport = ?\n"
            "and f1.arrival_airport = f2.departure_airport\n"
            "and f2.arrival_airport = ?\n"
            "and f1.scheduled_arrival <= f2.scheduled_departure\n"
            "and (f2.scheduled_arrival - f1.scheduled_departure) < '24:00:00'\n"
            "and (f2.scheduled_arrival - f1.scheduled_departure) > '00:00:00'\n"
            "and f1.scheduled_departure::date = ?\n"
            "and 	(tf.flight_id = f1.flight_id\n"
              "or\n"
              "tf.flight_id = f2.flight_id)\n"
            "and asientosvacios > 0\n"
    "group by f1.flight_id,	f1.scheduled_departure, f2.scheduled_arrival),\n"
    "result\n"
    "as\n"
            "(SELECT DISTINCT flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n"
            "FROM flight_dir\n"
            "UNION\n"
            "SELECT DISTINCT flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n"
            "FROM flight_sec)\n"
    "select distinct flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n"
    "from result\n"
    "order by time_elapsed;\n";


  /* CONNECT */
  ret = odbc_connect(&env, &dbc);
  if (!SQL_SUCCEEDED(ret)) {
      return;
  }

  /* Allocate a statement handle */
  SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

  SQLPrepare(stmt, (SQLCHAR*) query, SQL_NTS);

  SQLBindParameter(stmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(from), 0, from, 0, NULL);
  SQLBindParameter(stmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(to), 0, to, 0, NULL);
  SQLBindParameter(stmt, 3, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(date), 0, date, 0, NULL);
  SQLBindParameter(stmt, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(from), 0, from, 0, NULL);
  SQLBindParameter(stmt, 5, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(to), 0, to, 0, NULL);
  SQLBindParameter(stmt, 6, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, strlen(date), 0, date, 0, NULL);

  if (!SQL_SUCCEEDED(SQLExecute(stmt))) {
    SQLCHAR sqlState[6], message[256];
    SQLINTEGER nativeError;
    SQLSMALLINT messageLen;
    SQLGetDiagRec(SQL_HANDLE_STMT, stmt, 1, sqlState, &nativeError, message, sizeof(message), &messageLen);
    printf("ODBC Error: %s (%s)\n", message, sqlState);
  }

  SQLBindCol(stmt, 1, SQL_C_CHAR, flight_id, sizeof(flight_id), NULL);
  SQLBindCol(stmt, 2, SQL_C_CHAR, code, sizeof(code), NULL);
  SQLBindCol(stmt, 3, SQL_C_CHAR, conexion, sizeof(conexion), NULL);
  SQLBindCol(stmt, 4, SQL_C_CHAR, dep, sizeof(dep), NULL);
  SQLBindCol(stmt, 5, SQL_C_CHAR, arr, sizeof(arr), NULL);
  SQLBindCol(stmt, 6, SQL_C_CHAR, asientos, sizeof(asientos), NULL);
  SQLBindCol(stmt, 7, SQL_C_CHAR, time_elapsed, sizeof(time_elapsed), NULL);

  
  /* Loop through the rows in the result-set */
  *n_choices = 0;
  while (SQL_SUCCEEDED(ret = SQLFetch(stmt)) && (*n_choices) < max_rows) {
    snprintf(aux, max_length,
             "%s\t%s\t%s\t%s\t%s\t%s\t%s",
             flight_id, code, conexion, dep, arr, asientos, time_elapsed);
    t = strlen(aux)+1;
    strncpy((*choices)[*n_choices], aux, t);
    (*n_choices)++;
  }

    SQLCloseCursor(stmt);

    /* free up statement handle */
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    /* DISCONNECT */
    ret = odbc_disconnect(env, dbc);
}
