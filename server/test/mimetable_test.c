/**
 * @file mimetable_test.c
 * @author Diego Ortín Fernández
 * @date 5 April 2020
 * @brief File that loads a test MIME association file, and tests both adding and retrieving associations.
 */

#include "../mimetable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_MIMEFILE_LOCATION "/server/test/mimetest.tsv"
#define MAX_DIR 400

int main() {
    char dir[MAX_DIR];
    getcwd(dir, sizeof dir);
    strcat(dir, TEST_MIMEFILE_LOCATION);

    assert(mime_add_from_file(dir) != ERROR);

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