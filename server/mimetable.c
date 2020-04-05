/**
 * @file mimetable.c
 * @author Diego Ortín Fernández
 * @date 5 April 2020
 * @brief Implementation of the functions for parsing a MIME file and retrieving its associations
 * @details This file contains the implementation for the MIME type hash table, which can be used to
 * retrieve the MIME type associated with certain file extension. It has been decided that having
 * different MIME tables for different servers doesn't make sense, so this hash table is global.
 * Therefor, all code using this module shares the same MIME type associations.
 *
 * The dictionary is implemented as a hash table for better performance, using the uthash library.
 * @see https://troydhanson.github.io/uthash/
 */

#include "uthash.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "mimetable.h"

#define MAX_EXTENSION 10
#define MAX_MIMETYPE 30

#define TAB_DELIM "\t"

struct mime_association {
    char extension[MAX_EXTENSION]; ///< File extension
    char type[MAX_MIMETYPE]; ///< Associated MIME type
    struct UT_hash_handle hh; ///< Handle to be used by @a uthash
};

struct mime_association *mime_table = NULL; ///< Global hash table holding the extension-type associations

STATUS mime_parse_line(char *line);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"

STATUS mime_add_association(char *extension, char *type) {
    if (!extension || !type) return ERROR;
    struct mime_association *mime_association = NULL;

    HASH_FIND_STR(mime_table, extension, mime_association); // Attempt to find an element with this key
    if (mime_association == NULL) { // If an element with this key wasn't found
        mime_association = (struct mime_association *) malloc(sizeof *mime_association); // Create a new one
        strcpy(mime_association->type, type);
        strcpy(mime_association->extension, extension);
        HASH_ADD_STR(mime_table, extension, mime_association); // Add the key for the hashtable
        return SUCCESS;
    } else { // If an element with this key already exists
        // TODO: Print to logs
        if (DEBUG >= 2) printf("Error: key %s already exists in the MIME table!\n", extension);
        return ERROR;
    }
}

#pragma clang diagnostic pop

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"

const char *mime_get_association(const char *extension) {
    if (!mime_table || !extension) return NULL;
    struct mime_association *association = NULL;

    HASH_FIND_STR(mime_table, extension, association);

    if (!association) return NULL; // If no association exists for this extension

    return association->type;
}

#pragma clang diagnostic pop

STATUS mime_add_from_file(const char *path) {
    if (!path) return ERROR;

    FILE *mimefd = fopen(path, "r");
    if (!mimefd) { // In case any error ocurred while opening the MIME file
        // TODO: Print to logs
#if DEBUG >= 1
        printf("Error while opening the MIME file: %s\n", strerror(errno));
#endif
        return ERROR;
    }

    // Enough size for extension, type, tab separator, newline and null-terminator
    char current_line[MAX_EXTENSION + MAX_MIMETYPE + 1 + 1 + 1];
    short int lines_parsed = 0;
    short int errors = 0;

    while (fgets(current_line, sizeof(current_line) / sizeof(current_line[0]), mimefd) != NULL) {
        if (mime_parse_line(current_line) == ERROR) {
            // TODO: Print to logs
#if DEBUG >= 1
            printf("Error while reading MIME file line: [%s]\n", current_line);
            errors++;
#endif
        } else {
#if DEBUG >= 2
            // TODO: Print to logs
            printf("Successfully parsed MIME association: [%s]\n", current_line);
#endif
            lines_parsed++;
        }
    }

    // TODO: Print to logs
    printf("%i MIME types loaded, %i errors\n", lines_parsed, errors);

    // If no lines were correctly parsed, it's better to return error, as the server won't correctly send
    // the Content-Type header
    if (lines_parsed == 0) return ERROR;

    return SUCCESS;
}

STATUS mime_parse_line(char *line) {
    if (!line) return ERROR;

    line[strlen(line) - 1] = '\0'; // Remove the newline character

    char *extension = strtok(line, TAB_DELIM); // Store the extension part
    char *type = strtok(NULL, TAB_DELIM); // Store the mime type part

    if (!extension || !type) return ERROR; // If any of the strtok calls failed, return error

    return mime_add_association(extension, type); // Return the result of adding the association
}