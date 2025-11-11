/*
 Created by roberto on 30/4/21.
 IT will not process properly UTF8
 export LC_ALL=C

 Function in this file assume a windows setup as:

 +-------------------------------+ <-- main_win
 |+-----------------------------+|
 ||          win_menu           ||
 |+-----------------------------+|
 |+--------++-------------------+|
 ||        ||                   ||
 ||        ||                   ||
 ||win_form||   win_out         ||
 ||        ||                   ||
 ||        ||                   ||
 |+--------++-------------------+|
 |+-----------------------------+|
 ||          win_messages       ||
 |+-----------------------------+|
 +-------------------------------+
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "loop.h"

/**/
#include "odbc.h"

/* Query 1 (Search): Completa, con 6 parámetros */
char *sql_search = "with 	tabla_1\n"
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
            "(scheduled_arrival - scheduled_departure) as time_elapsed,\n"
            "flight_id as flight_id_2,\n"
		        "scheduled_departure as scheduled_departure_2,\n"
		        "scheduled_arrival as scheduled_arrival_2,\n"
            "aircraft_code as aircraft_code_2\n"
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
            "(f2.scheduled_arrival - f1.scheduled_departure) as time_elapsed,\n"
            "f2.flight_id as flight_id_2,\n"
		        "f2.scheduled_departure as scheduled_departure_2,\n"
		        "f1.scheduled_arrival as scheduled_arrival_2,\n"
            "f2.aircraft_code as aircraft_code_2\n"
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
    "group by f1.flight_id,	f1.scheduled_departure, f2.scheduled_arrival, f2.flight_id, f2.scheduled_departure, f1.scheduled_arrival, f2.aircraft_code),\n"
    "result\n"
    "as\n"
            "(SELECT DISTINCT flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed, flight_id_2, scheduled_departure_2, scheduled_arrival_2, aircraft_code_2\n"
            "FROM flight_dir\n"
            "UNION\n"
            "SELECT DISTINCT flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed, flight_id_2, scheduled_departure_2, scheduled_arrival_2, aircraft_code_2\n"
            "FROM flight_sec)\n"
    "select distinct flight_id, aircraft_code, conexion, scheduled_departure, scheduled_arrival, asientosvacios, time_elapsed, flight_id_2, scheduled_departure_2, scheduled_arrival_2, aircraft_code_2\n"
    "from result\n"
    "order by time_elapsed;\n";

/* Query B.Pass 1: Encontrar vuelos pendientes */
char *sql_bpass_find_pending = "SELECT t.passenger_name,\n"
       "tf.ticket_no,\n"
       "tf.flight_id,\n"
       "f.aircraft_code\n"
"FROM   tickets AS t\n"
       "JOIN ticket_flights AS tf\n"
         "ON t.ticket_no = tf.ticket_no\n"
       "JOIN flights AS f\n"
         "ON tf.flight_id = f.flight_id\n"
       "LEFT JOIN boarding_passes AS bp\n"
              "ON tf.ticket_no = bp.ticket_no\n"
                 "AND tf.flight_id = bp.flight_id\n"
"WHERE  t.book_ref = ?\n"
       "AND bp.ticket_no IS NULL\n"
"ORDER  BY tf.ticket_no ASC;";

/* Query B.Pass 2: Encontrar asiento libre */
char *sql_bpass_find_seat = "SELECT s.seat_no\n"
"FROM   seats AS s\n"
"WHERE  s.aircraft_code = ?\n"
       "AND s.seat_no NOT IN (SELECT bp.seat_no\n"
                             "FROM   boarding_passes AS bp\n"
                             "WHERE  bp.flight_id = ?)\n"
"ORDER  BY s.seat_no asc\n"
"FETCH first 1 ROW only;";
/* Query B.Pass 3: Insertar tarjeta */
char *sql_bpass_insert = "INSERT INTO boarding_passes\n"
            "(ticket_no,\n"
             "flight_id,\n"
             "boarding_no,\n"
             "seat_no)\n"
"VALUES      (?,\n"
             "?,\n"
             "?,\n"
             "?);";

/* Query B.Pass 4: Obtener fecha de salida (para mostrar en la UI) */
char *sql_bpass_get_departure = "SELECT f.scheduled_departure\n"
"FROM   flights AS f\n"
"WHERE  f.flight_id = ?;";

/**/

static void trim_trailing_whitespace(char *str) {
    int i;
    if (str == NULL) {
        return;
    }
    i = strlen(str) - 1;
    while (i >= 0 && isspace(str[i])) {
        str[i] = '\0';
        i--;
    }
}

void loop(SQLHDBC dbc, _Windows *windows, _Menus *menus,
          _Forms *forms, _Panels *panels) {
    /** get keys pressed by user and process it. 
     * - If left/right arrow is pressed  move to
     * the next/previous menu item
     * - If up/down key is pressed check on focus
     * variable if focus == FOCUS_LEFT move to the
     * previous/next form item in the left window
     * (aka form_window). Otherwise do the same
     * in the win_out.
     * - If enter is pressed the action depend on the
     * item selected in the menu bar and the value of
     * "focus":
     *  a) focus = FOCUS_LEFT
     *     quit -> quit the application
     *     search -> execute  results_search
     *               and fill win_out
     *     bpass ->  execute  bpas_search
     *               and fill win_out
     *  b) focus == FOCUS_RIGHT
     *     show the highlighted line in
     *     win_out in the win_message window
     *  - tab key toggle focus between
     *  win_form and win_out windows
     */
    int focus = FOCUS_LEFT; /* focus is in win_form/win_out window */
    int ch=0; /* typed character */
    bool enterKey = FALSE; /* has enter been presswed ? */
    char buffer[1024]; /* auxiliary buffer to compose messages */
    ITEM *auxItem = (ITEM *) NULL; /* item selected in the menu */
    int choice = -1; /* index of the item selected in menu*/
    MENU *menu = NULL; /* pointer to menu in menu_win*/
    WINDOW *menu_win= NULL; /* pointer to menu_win */
    WINDOW *out_win= NULL; /* pointer to out_win */
    WINDOW *msg_win= NULL; /* pointer to msg_win */
    char * tmpStr1= NULL; /* used to read values typed in forms */
    char * tmpStr2= NULL; /* used to read values typed in forms */
    char * tmpStr3= NULL; /* used to read values typed in forms */
    char ** msg_search; /* used to store the message for the first consult */
    char ** result; /* used to store the resutl of the consult */
    int n_out_choices=0; /* number of printed lines in win_out window */
    int out_highlight = 0; /* line highlighted in win_out window */
    int rows_out_window = 0; /* size of win_out window */
    int start = 0; /* first row to be printed */
    int i = 0, j = 0; /* dummy variable for loops */

    /**/
    SQLHSTMT stmt_search, stmt_bpass_find_pending, stmt_bpass_find_seat, stmt_bpass_insert, stmt_bpass_get_departure;
    SQLRETURN ret;
    /**/

    (void) curs_set(1); /* show cursor */
    menu = menus->menu;
    menu_win = windows->menu_win;
    out_win = windows->out_win;
    msg_win = windows->msg_win;
    rows_out_window = windows->terminal_nrows - 2 * windows->height_menu_win - 1;

    /**/
    FILE *f;
    f = fopen("probando.txt", "w");
    fprintf(f, "Probando\n");
    /*Preparamos statements (solo una vez)*/

    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_search);
    ret = SQLPrepare(stmt_search, (SQLCHAR*) sql_search, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        write_msg(msg_win, "Error preparando sql_search", -1, -1, windows->msg_title);
        return;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_bpass_find_pending);
    ret = SQLPrepare(stmt_bpass_find_pending, (SQLCHAR*) sql_bpass_find_pending, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        write_msg(msg_win, "Error preparando sql_bpass_find_pending", -1, -1, windows->msg_title);
        return;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_bpass_find_seat);
    ret = SQLPrepare(stmt_bpass_find_seat, (SQLCHAR*) sql_bpass_find_seat, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        write_msg(msg_win, "Error preparando sql_bpass_find_seat", -1, -1, windows->msg_title);
        return;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_bpass_insert);
    ret = SQLPrepare(stmt_bpass_insert, (SQLCHAR*) sql_bpass_insert, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        write_msg(msg_win, "Error preparando sql_bpass_insert", -1, -1, windows->msg_title);
        return;
    }

    ret = SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt_bpass_get_departure);
    ret = SQLPrepare(stmt_bpass_get_departure, (SQLCHAR*) sql_bpass_get_departure, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        write_msg(msg_win, "Error preparando sql_bpass_get_departure", -1, -1, windows->msg_title);
        return;
    }

    /* Alloc memory for the consult 1 message */
    if(!(msg_search = (char **)malloc(sizeof(char *) * (TOTAL_ROWS)))) {
      return;
    }
    for(i = 0; i < TOTAL_ROWS ; i++) {
      if(!(msg_search[i] = (char *)malloc(sizeof(char) * (LENGTH_ROWS+1)))) {
        for(j = i; j >= 0 ;j--) {
          free(msg_search[j]);
        }
        free(msg_search);
      }
    }
    if(!(result = (char **)malloc(sizeof(char *) * (TOTAL_ROWS)))) {
      return;
    }
    for(i = 0; i < TOTAL_ROWS ; i++) {
      if(!(result[i] = (char *)malloc(sizeof(char) * (LENGTH_ROWS+1)))) {
        for(j = i; j >= 0 ;j--) {
          free(result[j]);
        }
        free(msg_search);
      }
    }
    fprintf(f, "While loop.c\n");

    /**/

    while ((bool) TRUE) {
        ch = getch(); /* get char typed by user */
        if ((bool)DEBUG) {
            (void)snprintf(buffer, 128, "key pressed %d %c (%d)",  ch, ch, item_index(auxItem));
            write_msg(msg_win, buffer, -1, -1, windows->msg_title);
        }
        switch (ch) {
            case KEY_LEFT:
            case 0x3C: /* < */
                focus = FOCUS_LEFT;
                /* auxiliary function provided by ncurses see
                 * https://docs.oracle.com/cd/E88353_01/html/E37849/menu-driver-3curses.html
                 */
                (void) menu_driver(menu, REQ_LEFT_ITEM);
                /* refresh window menu_win */
                (void) wrefresh(menu_win);
                /* get selected item from menu */
                auxItem = current_item(menu);
                /* draw the corresponding form in win_form */
                if (item_index(auxItem) == SEARCH)
                    (void) top_panel(panels->search_panel);
                else if (item_index(auxItem) == BPASS)
                    (void) top_panel(panels->bpass_panel);
                /* refresh window using the panel system.
                 * we need panels because search and bpass
                 * are attached to two ovelapping windows
                 * */
                (void) update_panels();
                (void) doupdate();
                break;
            case KEY_RIGHT:
            case 0x3E: /* > */
                focus = FOCUS_LEFT;
                (void) menu_driver(menu, REQ_RIGHT_ITEM);
                (void) wrefresh(menu_win);
                auxItem = current_item(menu);
                if (item_index(auxItem) == SEARCH)
                    (void) top_panel(panels->search_panel);
                else if (item_index(auxItem) == BPASS)
                    (void) top_panel(panels->bpass_panel);
                (void) update_panels();
                (void) doupdate();
                break;
            case KEY_UP:
            case 0x2B: /* + */
                /* form_driver is the equivalent to menu_driver for forms */
                if (item_index(auxItem) == SEARCH && focus == FOCUS_LEFT) {
                    (void) form_driver(forms->search_form, REQ_PREV_FIELD);
                    (void) form_driver(forms->search_form, REQ_END_LINE);
                    (void) wrefresh(windows->form_search_win);
                } else if (item_index(auxItem) == BPASS && focus == FOCUS_LEFT) {
                    (void) form_driver(forms->bpass_form, REQ_PREV_FIELD);
                    (void) form_driver(forms->bpass_form, REQ_END_LINE);
                    (void) wrefresh(windows->form_bpass_win);
                }  else if (focus == FOCUS_RIGHT){
                    out_highlight = MAX(out_highlight - 1, 0);
                    print_out(out_win, menus->out_win_choices, n_out_choices,
                              out_highlight, windows->out_title);
                }
                break;
            case KEY_DOWN:
            /* case 0x2D: */
                if (item_index(auxItem) == SEARCH && focus == FOCUS_LEFT) {
                    (void) form_driver(forms->search_form, REQ_NEXT_FIELD);
                    (void) form_driver(forms->search_form, REQ_END_LINE);
                    (void) wrefresh(windows->form_search_win);
                } else if (item_index(auxItem) == BPASS && focus == FOCUS_LEFT) {
                    (void) form_driver(forms->bpass_form, REQ_NEXT_FIELD);
                    (void) form_driver(forms->bpass_form, REQ_END_LINE);
                    (void) wrefresh(windows->form_bpass_win);
                } else if (focus == FOCUS_RIGHT){
                    out_highlight = MIN(out_highlight + 1, n_out_choices-1);
                    print_out(out_win, menus->out_win_choices, n_out_choices,
                              out_highlight, windows->out_title);
                }
                break;
            case KEY_STAB: /* tab key */
            case 9:
                /* toggle focus between win_form and win_out*/

                if (focus == FOCUS_RIGHT)
                     focus = FOCUS_LEFT;
                else
                     focus = FOCUS_RIGHT;

                (void) snprintf(buffer, 128, "focus in window %d", focus);
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);
                /* If win_form is selected place the cursor in the right place */
                if (item_index(auxItem) == SEARCH && focus == FOCUS_LEFT) {
                    (void)form_driver(forms->search_form, REQ_END_LINE);
                    (void)wrefresh(windows->form_search_win);
                }
                else if (item_index(auxItem) == BPASS && focus == FOCUS_LEFT) {
                    (void)form_driver(forms->bpass_form, REQ_END_LINE);
                    (void)wrefresh(windows->form_bpass_win);
                }
                break;
            case KEY_BACKSPACE: /* delete last key */
            case 127:
                if (item_index(auxItem) == SEARCH) {
                    (void)form_driver(forms->search_form, REQ_DEL_PREV);
                    (void)wrefresh(windows->form_search_win);
                } else if (item_index(auxItem) == BPASS) {
                    (void)form_driver(forms->bpass_form, REQ_DEL_PREV);
                    (void)wrefresh(windows->form_bpass_win);
                }
                break;
            case 10: /* enter has been pressed*/
                auxItem = current_item(menu);
                choice = item_index(auxItem);
                enterKey = (bool) TRUE; /* mark enter pressed */
                start = 0;
                break;
            /* Next page */
            case 0x2E: /* . */
              start = +windows->rows_out_win-2;
              (void)wclear(out_win); /* Clear window */
              (void)form_driver(forms->search_form, REQ_VALIDATION);
              /* Copy the rows from result starting from the start row*/
              i = 0;
              while(i < windows->rows_out_win-2 && i < TOTAL_ROWS) {
                j = strlen((result)[i + start])+1;
                j = MIN(j, windows->rows_out_win-2);
                strncpy((menus->out_win_choices)[i], (result)[i + start], j);
                i++;
              }
              print_out(out_win, menus->out_win_choices, n_out_choices, out_highlight, windows->out_title);
              break;
            /* Previous page*/
            case 0x2C: /* , */
              start = -windows->rows_out_win-2;
              if(start < 0) start = 0;
              (void)wclear(out_win);
              (void)form_driver(forms->search_form, REQ_VALIDATION);
              i = 0;
              while(i < windows->rows_out_win-2 && i < TOTAL_ROWS) {
                j = strlen((result)[i + start])+1;
                j = MIN(j, windows->rows_out_win-2);
                strncpy((menus->out_win_choices)[i], (result)[i + start], j);
                i++;
              }
              print_out(out_win, menus->out_win_choices, n_out_choices, out_highlight, windows->out_title);
              break;
            default: /* echo pressed key */
                auxItem = current_item(menu);
                if (item_index(auxItem) == SEARCH) {
                    (void)form_driver(forms->search_form, ch);
                    (void)wrefresh(windows->form_search_win);
                }
                else if (item_index(auxItem) == BPASS) {
                    (void)form_driver(forms->bpass_form, ch);
                    (void)wrefresh(windows->form_bpass_win);
                }
                break;
        }

        if (choice != -1 && enterKey) /* User did a choice process it */
        {
            if (choice == QUIT)
                break; /* quit */
            else if ((choice == SEARCH) && (focus == FOCUS_LEFT)) {
                out_highlight = 0;
                for(i=0; i< rows_out_window ; i++)
                    (menus->out_win_choices)[i][0] = '\0';
                (void)wclear(out_win);
                (void)form_driver(forms->search_form, REQ_VALIDATION);
                tmpStr1 = field_buffer((forms->search_form_items)[1], 0);
                tmpStr2 = field_buffer((forms->search_form_items)[3], 0);
                tmpStr3 = field_buffer((forms->search_form_items)[5], 0);
                trim_trailing_whitespace(tmpStr1);
                trim_trailing_whitespace(tmpStr2);
                trim_trailing_whitespace(tmpStr3);
                fprintf(f, "Entra en results_search\n");
                fclose(f);
                results_search(stmt_search, tmpStr1, tmpStr2, tmpStr3, &n_out_choices, & (menus->out_win_choices),
                               windows->cols_out_win-4, windows->rows_out_win-2, &msg_search, &result);
                f = fopen("probando.txt", "a");
                fprintf(f, "Sale de results_search\n");
                print_out(out_win, menus->out_win_choices, n_out_choices,
                          out_highlight, windows->out_title);
                if (!strcmp((menus->out_win_choices)[out_highlight], "No se encontraron vuelos para esa ruta y fecha.") || !strcmp((menus->out_win_choices)[out_highlight], "Error: No se encontraron vuelos.")) {
                  (void)snprintf(buffer, 128, "%s", (menus->out_win_choices)[out_highlight] );
                } else if((bool)DEBUG) {
                  (void)snprintf(buffer, 128, "Origen: %s  |  Destino: %s  |  Fecha: %s",  tmpStr1, tmpStr2, tmpStr3);
                }
                write_msg(msg_win, buffer, -1, -1, windows->msg_title);

            }
            else if ((choice == SEARCH) && (focus == FOCUS_RIGHT)) {
                /* Print the message */
                if(!strcmp((menus->out_win_choices)[out_highlight], "No se encontraron vuelos para esa ruta y fecha.") || !strcmp((menus->out_win_choices)[out_highlight], "Error: No se encontraron vuelos.")) {
                  (void)snprintf(buffer, 128, "%s", (menus->out_win_choices)[out_highlight] );
                } else {
                  (void)snprintf(buffer, LENGTH_ROWS, "%s", msg_search[out_highlight]);
                }
                write_msg(msg_win,buffer,
                          -1, -1, windows->msg_title);
            }
            else if ((choice == BPASS) && (focus == FOCUS_LEFT)) {
                out_highlight = 0;
                for(i=0; i< rows_out_window ; i++)
                    (menus->out_win_choices)[i][0] = '\0';
                (void) wclear(out_win);
                (void) form_driver(forms->bpass_form, REQ_VALIDATION);
                tmpStr1 = field_buffer((forms->bpass_form_items)[1], 0);
                trim_trailing_whitespace(tmpStr1);
                results_bpass(dbc, 
                              stmt_bpass_find_pending, 
                              stmt_bpass_find_seat, 
                              stmt_bpass_insert, 
                              stmt_bpass_get_departure,
                              tmpStr1, &n_out_choices, & (menus->out_win_choices),
                              windows->cols_out_win-4, windows->rows_out_win-2);
                print_out(out_win, menus->out_win_choices, n_out_choices,
                          out_highlight, windows->out_title);
            }
            else if ((choice == BPASS) && focus == (FOCUS_RIGHT)) {
                write_msg(msg_win, (menus->out_win_choices)[out_highlight],
                          -1, -1, windows->msg_title);
            }
        }
        choice = -1;
        enterKey = (bool) FALSE;
    }
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_search);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_bpass_find_pending);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_bpass_find_seat);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_bpass_insert);
    SQLFreeHandle(SQL_HANDLE_STMT, stmt_bpass_get_departure);
    /* Free message */
    for(i = 0; i < TOTAL_ROWS ; i++) {
      free(msg_search[i]);
    }
    free(msg_search);
    for(i = 0; i < TOTAL_ROWS ; i++) {
      free(result[i]);
    }
    free(result);
    fclose(f);
}
