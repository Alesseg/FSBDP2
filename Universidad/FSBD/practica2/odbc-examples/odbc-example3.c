#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include "odbc.h"

/*
 * example 3 with a queries build on-the-fly, the bad way
 */

int main(void) {
    SQLHENV env;
    SQLHDBC dbc;
    SQLHSTMT stmt;
    SQLRETURN ret; /* ODBC API return status */
    SQLCHAR flight_id[4096], conexion[4096], dep[4096], arr[4096], asientos[4096], time_elapsed[4096];
    char origin[4], destination[4];

    /* CONNECT */
    ret = odbc_connect(&env, &dbc);
    if (!SQL_SUCCEEDED(ret)) {
        return EXIT_FAILURE;
    }

    /* Allocate a statement handle */
    SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

    printf("From: ");
    fflush(stdout);
    if (scanf("%s", origin) != 1) {
      return EXIT_FAILURE;
    }
    printf("To: ");
    fflush(stdout);
    if (scanf("%s", destination) != 1) {
        return EXIT_FAILURE;
    }
    char query[4096];
      sprintf(query, "with 	tabla_1\n\
      AS (SELECT flight_id,\n\
                  Count (flight_id) AS n_people\n\
          FROM   boarding_passes\n\
          GROUP  BY flight_id),\n\
        totalPeople\n\
      AS (SELECT f.flight_id,\n\
                  COALESCE(n_people, 0) AS numpeople\n\
          FROM   flights f\n\
                  LEFT JOIN tabla_1 t\n\
                        ON f.flight_id = t.flight_id),\n\
        totalSeats\n\
      AS (SELECT aircraft_code,\n\
                  Count (seat_no) AS numseats\n\
          FROM   seats\n\
          GROUP  BY aircraft_code),\n\
        totalfree\n\
      AS (SELECT flight_id,\n\
                  ( numseats - numpeople ) AS asientosvacios\n\
          FROM   totalPeople\n\
                  natural JOIN flights\n\
                  natural JOIN totalSeats),\n\
    flight_dir \n\
    as   (select distinct flight_id,\n\
              0 as conexion,\n\
              scheduled_departure,\n\
              scheduled_arrival,\n\
              asientosvacios,\n\
              (scheduled_arrival - scheduled_departure) as time_elapsed\n\
      from	  flights\n\
              natural join totalfree\n\
      where   departure_airport = '%s'\n\
              and arrival_airport = '%s'\n\
              and (scheduled_arrival - scheduled_departure) < '24:00:00'\n\
              and scheduled_departure::date = '2017-07-16'\n\
              and asientosvacios > 0\n\
      ),\n\
          flight_sec\n\
          as   (select distinct f1.flight_id,\n\
              1 as conexion,\n\
              f1.scheduled_departure,\n\
              f2.scheduled_arrival,\n\
              min(asientosvacios) as asientosvacios,\n\
              (f2.scheduled_arrival - f1.scheduled_departure) as time_elapsed\n\
      from    flights f1,\n\
              flights f2,\n\
              totalfree tf\n\
      where   f1.departure_airport = '%s'\n\
              and f1.arrival_airport = f2.departure_airport\n\
              and f2.arrival_airport = '%s'\n\
              and (f2.scheduled_arrival - f1.scheduled_departure) < '24:00:00'\n\
              and (f2.scheduled_arrival - f1.scheduled_departure) > '00:00:00'\n\
              and f1.scheduled_departure::date = '2017-07-16'\n\
              and 	(tf.flight_id = f1.flight_id\n\
                or\n\
                tf.flight_id = f2.flight_id)\n\
              and asientosvacios > 0\n\
      group by f1.flight_id,	f1.scheduled_departure, f2.scheduled_arrival),\n\
      result\n\
      as\n\
              (SELECT DISTINCT flight_id, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n\
              FROM flight_dir\n\
              UNION\n\
              SELECT DISTINCT flight_id, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n\
              FROM flight_sec)\n\
      select distinct flight_id, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed\n\
      from result\n\
      order by time_elapsed;\n", origin, destination, origin, destination);

    printf("\nEjecutando consulta:\n%s\n", query);

    ret = SQLExecDirect(stmt, (SQLCHAR *)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret))
    {
        printf("Error ejecutando la consulta (SQLExecDirect).\n");
        odbc_show_error(SQL_HANDLE_STMT, stmt);
        SQLCloseCursor(stmt);
    }

    SQLBindCol(stmt, 1, SQL_C_CHAR, flight_id, sizeof(flight_id), NULL);
    SQLBindCol(stmt, 2, SQL_C_CHAR, conexion, sizeof(conexion), NULL);
    SQLBindCol(stmt, 3, SQL_C_CHAR, dep, sizeof(dep), NULL);
    SQLBindCol(stmt, 4, SQL_C_CHAR, arr, sizeof(arr), NULL);
    SQLBindCol(stmt, 5, SQL_C_CHAR, asientos, sizeof(asientos), NULL);
    SQLBindCol(stmt, 6, SQL_C_CHAR, time_elapsed, sizeof(time_elapsed), NULL);

    /* Loop through the rows in the result-set */
    printf(" Flight_id | Conexion | Departure | Arrival | Asientos | Time elapsed\n");
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt))) {
      printf(" %s\t| %s\t| %s\t| %s\t| %s\t| %s\n", flight_id, conexion, dep, arr, asientos, time_elapsed);
    }
    printf("\n");

    SQLCloseCursor(stmt);
    
    /* free up statement handle */
    SQLFreeHandle(SQL_HANDLE_STMT, stmt);

    /* DISCONNECT */
    ret = odbc_disconnect(env, dbc);
    if (!SQL_SUCCEEDED(ret)) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

