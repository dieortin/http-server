//
// Created by diego on 4/4/20.
//

#include "mimetable.h"
#include "uthash.h"
#include "constants.h"
#include <string.h>
#include <stdio.h>

#define MAX_EXTENSION 10
#define MAX_MIMETYPE 20

struct mime_association {
    char extension[MAX_EXTENSION]; ///< File extension
    char type[MAX_MIMETYPE]; ///< Associated MIME type
    struct UT_hash_handle hh; ///< Handle to be used by @a uthash
};

struct mime_association *mime_table = NULL; ///< Global hash table holding the extension-type associations

STATUS mime_add_association(char *name, char *value) {
    if (!name || !value) return ERROR;
    struct mime_association *mime_association = NULL;

    HASH_FIND_STR(mime_table, name, mime_association); // Attempt to find an element with this key
    if (mime_association == NULL) { // If an element with this key wasn't found
        mime_association = (struct mime_association *) malloc(sizeof *mime_association); // Create a new one
        strcpy(mime_association->type, value);
        strcpy(mime_association->extension, name);
        HASH_ADD_STR(mime_table, extension, mime_association); // Add the key for the hashtable
        return SUCCESS;
    } else { // If an element with this key already exists
        // TODO: Print to logs
        if (DEBUG >= 2) printf("Error: key %s already exists in the MIME table!\n", name);
        return ERROR;
    }
}

const char *mime_get_association(const char *extension) {
    if (!mime_table || !extension) return NULL;
    struct mime_association *association = NULL;

    HASH_FIND_STR(mime_table, extension, association);

    if (!association) return NULL; // If no association exists for this extension

    return association->type;
}