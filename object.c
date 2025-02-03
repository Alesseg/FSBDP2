#include "object.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>


/** object_create allocates memory for a new object
 *  and initializes its members
 */
Object* object_create(Id id) {
    Object *obj = NULL;

    /* Error control */
    if (id == NO_ID) return NULL;


    if(!(obj=(Object*)calloc(1, sizeof(Object)))) {
        return NULL;
    }

    /* Initialization of an empty object*/
    obj->location = NO_ID;

    return obj;
}

Status object_destroy(Object* object) {
    if(!object) {
        return ERROR;
    }

    free(object);
    object = NULL;

    return OK;
}


Status object_set_location(Object* object, Id id) {
    if(!object || id==NO_ID) {
        return ERROR;
    }

    object->location = id;

    return OK;
}

Id object_get_location(Object* object) {
    if(!object) {
        return NO_ID;
    }
    return object->location;
}


void object_print() {
    
    sprintf(str, "  |     *     |");

    return OK;
}