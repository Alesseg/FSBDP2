/**
 * @brief It defines the object interpreter interface
 *
 * @file object.h
 * @author Alejandro Seguido Grinnon
 * @version 0
 * @date 1-02-2025
 * @copyright GNU Public License
 */

#ifndef OBJECT_H
#define OBJECT_H

#include "types.h"

typedef struct _Object {
    Id location;
} Object;

/**
 * @brief It creates a new object, allocating memory and initializing its members
 * @author Alejandro Seguido Griñón
 *
 * @param id the identification number for the new object
 * @return a new object, initialized
 */
Object* object_create(Id id);

/**
 * @brief It destroys an object, freeing the allocated memory
 * @author Alejandro Seguido Griñón
 *
 * @param object a pointer to the object that must be destroyed
 * @return OK, if everything goes well or ERROR if there was some mistake
 */
Status object_destroy(Object* object);

/**
 * @brief It sets the location of the object
 * @author Alejandro Seguido Griñón
 *
 * @param object a pointer to the object
 * @param id the identification number for the new object location
 * @return OK, if everything goes well or ERROR if there was some mistake
 */
Status object_set_location(Object* object, Id id);

/**
 * @brief It gets the location of the object
 * @author Alejandro Seguido Grinnon
 *
 * @param object a pointer to the object
 * @return a id number of the space where the object is
 */
Id object_get_location(Object* object);

/**
 * @brief It prints the object
 * @author Alejandro Seguido Grinnon
 *
 * 
 * @param none
 * @return No return
 */
void object_print();

#endif