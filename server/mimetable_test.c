//
// Created by diego on 4/4/20.
//

#include "mimetable.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main() {
    mime_add_association("jpeg", "image/jpeg");
    mime_add_association("mpg", "video/mpeg");
    mime_add_association("png", "image/png");
    mime_add_association("pdf", "application/pdf");
    mime_add_association("html", "text/html");

    assert(strcmp(mime_get_association("jpeg"), "image/jpeg") == 0);
    assert(strcmp(mime_get_association("mpg"), "video/mpeg") == 0);
    assert(strcmp(mime_get_association("png"), "image/png") == 0);
    assert(strcmp(mime_get_association("pdf"), "application/pdf") == 0);
    assert(strcmp(mime_get_association("html"), "text/html") == 0);

    return EXIT_SUCCESS;
}