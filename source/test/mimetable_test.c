/**
 * @file mimetable_test.c
 * @author Diego Ortín Fernández
 * @date 5 April 2020
 * @brief File that loads a test MIME association file, and tests both adding and retrieving associations.
 */

#include "mimetable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_DIR 400

int main(int argc, char *argv[]) {
    if (argc < 2) { // We should have the command and the server configuration path
        fprintf(stderr, "Configuration file argument missing\n");
        fprintf(stdout, "Usage: mimetable_test <mime.tsv path>\n");
        exit(EXIT_FAILURE);
    }
    char *mimefile = argv[1];

    printf("Adding MIME types from file %s\n", mimefile);
    assert(mime_add_from_file(mimefile) != ERROR);

    // Correct association tests
    assert(strcmp(mime_get_association("jpg"), "image/jpeg") == 0);
    assert(strcmp(mime_get_association("mpg"), "video/mpeg") == 0);
    assert(strcmp(mime_get_association("png"), "image/png") == 0);
    assert(strcmp(mime_get_association("pdf"), "application/pdf") == 0);
    assert(strcmp(mime_get_association("html"), "text/html") == 0);

    // Missing association tests
    assert(mime_get_association("ttt") == NULL);
    assert(mime_get_association("nope") == NULL);
    assert(mime_get_association("h12ml") == NULL);
    assert(mime_get_association("jpggg") == NULL);

    return EXIT_SUCCESS;
}