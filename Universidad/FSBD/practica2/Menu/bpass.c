/*
* Created by roberto on 3/5/21.
*/
#include "lbpass.h"
#include "odbc.h"
#include <stdio.h>

static void set_ncurses_error(int * n_choices, char *** choices, char * msg, int max_length) {
    *n_choices = 1;
    strncpy((*choices)[0], msg, max_length);
}

void    results_bpass(SQLHDBC dbc,
                       SQLHSTMT stmt_find_pending,
                       SQLHSTMT stmt_find_seat,
                       SQLHSTMT stmt_insert,
                       SQLHSTMT stmt_get_departure,
                       char * bookID,
                       int * n_choices, char *** choices,
                       int max_length,
                       int max_rows)
/**here you need to do your query and fill the choices array of strings
*
* @param bookID  form field bookId
* @param n_choices fill this with the number of results
* @param choices fill this with the actual results
* @param max_length output win maximum width
* @param max_rows output win maximum number of rows
*/

{
    SQLRETURN ret;
    int t = 0;
    char aux[1024]; /*he quitado el max_length de aquí*/
    int boarding_card_counter = 0; /* Para el boarding_no */

    /* Variables para Q1 (Find Pending) */
    char book_ref_param[7]; /* book_ref es char(6) */
    SQLCHAR passenger_name[128];
    SQLCHAR ticket_no[14]; /* ticket_no es char(13) */
    SQLINTEGER flight_id;
    SQLCHAR aircraft_code[4]; /* aircraft_code es char(3) */

    /* Variables para Q2 (Find Seat) */
    SQLCHAR seat_no[5]; /* seat_no es varchar(4) */

    /* Variables para Q4 (Get Departure) */
    SQLCHAR scheduled_departure[64];

    /**/
    *n_choices = 0;
    /**/

    /* Copiar el bookID (con padding si es necesario) */
    strncpy(book_ref_param, bookID, 6);
    book_ref_param[6] = '\0';


    /* --- INICIO DE LA TRANSACCIÓN --- */
    /* 1. Desactivar AutoCommit */
    ret = SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_OFF, 0);
    if (!SQL_SUCCEEDED(ret)) {
        set_ncurses_error(n_choices, choices, "Error: No se pudo iniciar la transaccion.", max_length);
        return;
    }

    /* --- EJECUTAR QUERY 1: Encontrar vuelos pendientes --- */
    
    /* 2. Bind Parameter (book_ref) */
    SQLBindParameter(stmt_find_pending, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 6, 0, book_ref_param, 0, NULL);
    
    /* 3. Execute */
    ret = SQLExecute(stmt_find_pending);
    if (!SQL_SUCCEEDED(ret)) {
        set_ncurses_error(n_choices, choices, "Error: No se encontraron vuelos para esa reserva.", max_length);
        odbc_extract_error("SQLExecute (find_pending)", stmt_find_pending, SQL_HANDLE_STMT);
        SQLTransact(SQL_NULL_HENV, dbc, SQL_ROLLBACK); /* Revertir transacción */
        SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0); /* Activar autocommit */
        SQLCloseCursor(stmt_find_pending);
        return;
    }

    /* 4. Bind Columns (para los resultados de Q1) */
    SQLBindCol(stmt_find_pending, 1, SQL_C_CHAR, passenger_name, sizeof(passenger_name), NULL);
    SQLBindCol(stmt_find_pending, 2, SQL_C_CHAR, ticket_no, sizeof(ticket_no), NULL);
    SQLBindCol(stmt_find_pending, 3, SQL_C_SLONG, &flight_id, 0, NULL);
    SQLBindCol(stmt_find_pending, 4, SQL_C_CHAR, aircraft_code, sizeof(aircraft_code), NULL);

    /* --- BUCLE PRINCIPAL: Iterar por cada vuelo pendiente --- */
    while (SQL_SUCCEEDED(ret = SQLFetch(stmt_find_pending)) && (*n_choices) < max_rows) {
        
        /* --- EJECUTAR QUERY 2: Encontrar asiento libre --- */
        
        /* 5. Bind Parameters (aircraft_code, flight_id) */
        SQLBindParameter(stmt_find_seat, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 3, 0, aircraft_code, 0, NULL);
        SQLBindParameter(stmt_find_seat, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &flight_id, 0, NULL);
        
        /* 6. Execute */
        ret = SQLExecute(stmt_find_seat);
        if (!SQL_SUCCEEDED(ret)) {
             set_ncurses_error(n_choices, choices, "Error: Al buscar asiento (Q2).", max_length);
             SQLTransact(SQL_NULL_HENV, dbc, SQL_ROLLBACK); /* Revertir */
             SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
             SQLCloseCursor(stmt_find_pending);
             SQLCloseCursor(stmt_find_seat);
             return;
        }

        /* 7. Bind Column (seat_no) */
        SQLBindCol(stmt_find_seat, 1, SQL_C_CHAR, seat_no, sizeof(seat_no), NULL);

        /* 8. Fetch (Solo esperamos 1 fila) */
        ret = SQLFetch(stmt_find_seat);
        if (!SQL_SUCCEEDED(ret)) {
            /* Esto significa que NO HAY ASIENTOS LIBRES */
            snprintf(aux, max_length, "Vuelo %d: SIN ASIENTOS DISPONIBLES.", (int)flight_id);
            t = strlen(aux)+1;
            strncpy((*choices)[*n_choices], aux, t);
            (*n_choices)++;
            SQLCloseCursor(stmt_find_seat);
            continue; /* Saltar al siguiente vuelo */
        }
        SQLCloseCursor(stmt_find_seat); /* Importante cerrar cursor de Q2 */


        /* --- EJECUTAR QUERY 3: Insertar tarjeta --- */
        
        boarding_card_counter++; /* Incrementar boarding_no */
        
        /* 9. Bind Parameters (ticket_no, flight_id, boarding_no, seat_no) */
        SQLBindParameter(stmt_insert, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_CHAR, 13, 0, ticket_no, 0, NULL);
        SQLBindParameter(stmt_insert, 2, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &flight_id, 0, NULL);
        SQLBindParameter(stmt_insert, 3, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &boarding_card_counter, 0, NULL);
        SQLBindParameter(stmt_insert, 4, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 4, 0, seat_no, 0, NULL);

        /* 10. Execute (El INSERT) */
        ret = SQLExecute(stmt_insert);
        if (!SQL_SUCCEEDED(ret)) {
            snprintf(aux, max_length, "Error: Al insertar tarjeta (Q3) para vuelo %d.", (int)flight_id);
            set_ncurses_error(n_choices, choices, aux, max_length);
            odbc_extract_error("SQLExecute (insert)", stmt_insert, SQL_HANDLE_STMT);
            SQLTransact(SQL_NULL_HENV, dbc, SQL_ROLLBACK); /* Revertir */
            SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
            SQLCloseCursor(stmt_find_pending);
            return;
        }
        SQLCloseCursor(stmt_insert); /* Cerrar cursor (aunque sea INSERT) */


        /* --- EJECUTAR QUERY 4: Obtener datos para la salida --- */
        
        /* 11. Bind Parameter (flight_id) */
        SQLBindParameter(stmt_get_departure, 1, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0, &flight_id, 0, NULL);
        
        /* 12. Execute */
        ret = SQLExecute(stmt_get_departure);
        SQLBindCol(stmt_get_departure, 1, SQL_C_CHAR, scheduled_departure, sizeof(scheduled_departure), NULL);
        SQLFetch(stmt_get_departure);
        SQLCloseCursor(stmt_get_departure);


        /* --- Formatear la línea de salida --- */
        /* (passenger_name, flight_id, scheduled_departure and seat_no) */
        /* Truncar passenger_name a 20 chars como pide la práctica */
        passenger_name[20] = '\0';
        
        snprintf(aux, max_length, "Emitida: %-20.20s | Vuelo: %-5d | Asiento: %-4s | Salida: %s",
                 (char*)passenger_name,
                 (int)flight_id,
                 (char*)seat_no,
                 (char*)scheduled_departure);
        
        t = strlen(aux)+1;
        strncpy((*choices)[*n_choices], aux, t);
        (*n_choices)++;

    } /* --- Fin del bucle principal (SQLFetch de Q1) --- */

    SQLCloseCursor(stmt_find_pending); /* Cerrar cursor de Q1 */

    if (*n_choices == 0) {
        set_ncurses_error(n_choices, choices, "No hay tarjetas de embarque pendientes para esa reserva.", max_length);
    }

    /* --- FIN DE LA TRANSACCIÓN --- */
    /* 13. Commit (Confirmar todos los INSERTs) */
    ret = SQLTransact(SQL_NULL_HENV, dbc, SQL_COMMIT);
    if (!SQL_SUCCEEDED(ret)) {
        set_ncurses_error(n_choices, choices, "Error: FALLO EN EL COMMIT FINAL.", max_length);
        SQLTransact(SQL_NULL_HENV, dbc, SQL_ROLLBACK); /* Intentar revertir por si acaso */
    }

    /* 14. Reactivar AutoCommit */
    SQLSetConnectAttr(dbc, SQL_ATTR_AUTOCOMMIT, (SQLPOINTER)SQL_AUTOCOMMIT_ON, 0);
}
