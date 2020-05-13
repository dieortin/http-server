/**
 * @file httpserver.c
 * @brief Functions that implement this specific HTTP server
 * @details This module implements an HTTP server using the methods and data structures provided by the httputils.h
 * module. It has a main request processing function that conforms to the prototype required by the #Server module, in
 * order to be used together with it.
 * It also has a router function, that calls the appropriate function depending on the method of the HTTP request.
 * @see httputils.h
 * @see server.h
 * @author Diego Ortín and Mario López
 * @date February 2020
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <glob.h>

#include "httpserver.h"


enum EXECUTABLE {
    PYTHON,
    PHP,
    NON_EXECUTABLE
};

char *executable_cmd[] = {"python", "php"};

int route(int socket, struct request *request, struct _srvutils *utils);

int resolution_get(int socket, struct request *request, struct _srvutils *utils);

int resolution_post(int socket, struct request *request, struct _srvutils *utils);

int resolution_options(int socket);

enum EXECUTABLE executable_type(const char *path);

SERVERCMD processHTTPRequest(int socket, struct _srvutils *utils) {
    int routecode;

    struct request *request = NULL;
    parse_result pres = parseRequest(socket, &request);
    switch (pres) {
        case PARSE_OK:
            routecode = route(socket, request, utils);
            if (request->querystring) {
                utils->log(stdout, "%s %s%s %i", request->method, request->path, request->querystring, routecode);
            } else {
                utils->log(stdout, "%s %s %i", request->method, request->path, routecode);
            }
            freeRequest(request);

            return CONTINUE; /// Tell the server to continue accepting requests
        case PARSE_ERROR:
            respond(socket, BAD_REQUEST, "Bad request", NULL, NULL, 0);
            utils->log(stdout, "%s %i", "Bad request", BAD_REQUEST);
            return CONTINUE;
        case PARSE_REQTOOLONG:
            respond(socket, BAD_REQUEST, "Request too long", NULL, NULL, 0);
            utils->log(stdout, "%s %i", "Request too long", BAD_REQUEST);
            return CONTINUE;
        case PARSE_IOERROR:
            utils->log(stderr, "Error while reading from socket %i: %s", socket, strerror(errno));
            respond(socket, INTERNAL_ERROR, "Internal server error", NULL, NULL, 0);
            utils->log(stdout, "%s %i", "Internal error", INTERNAL_ERROR);
            return CONTINUE; // TODO: stop?
        default:
            utils->log(stderr, "Error while parsing request");
            respond(socket, INTERNAL_ERROR, "Internal server error", NULL, NULL, 0);
            utils->log(stdout, "%s %i", "Internal error", INTERNAL_ERROR);
            return CONTINUE; // TODO: stop?
    }
}

int route(int socket, struct request *request, struct _srvutils *utils) {
    if (strcmp(request->method, GET) == 0) {
        return resolution_get(socket, request, utils);
    } else if (strcmp(request->method, POST) == 0) {
        return resolution_post(socket, request, utils);
    } else if (strcmp(request->method, OPTIONS) == 0) {
        return resolution_options(socket);
    } else {
        return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, NULL, 0);
    }
}

int resolution_get(int socket, struct request *request, struct _srvutils *utils) {
    //create header structure
    struct httpres_headers *headers = create_header_struct();
    setDefaultHeaders(headers);

    // Calculate the size that the full path will have
    size_t fullpath_size = strlen(utils->webroot) + strlen(request->path) + 1;

    // Allocate enough space for the entire path and the null terminator
    char *fullpath = calloc(sizeof(char), fullpath_size);

    // Concatenate both parts of the path to obtain the full path
    strcat(fullpath, utils->webroot);
    strcat(fullpath, request->path);

#if DEBUG >= 2
    utils->log(stdout, "Full path: %s", fullpath);
#endif

    if (is_directory(fullpath)) { // If it's a directory, attempt to serve an index.html
        size_t newfullpath_size = fullpath_size + strlen(INDEX_PATH);
        fullpath = realloc(fullpath, newfullpath_size); // Reallocate with size for the index
        strncat(fullpath, INDEX_PATH, newfullpath_size); // Concatenate the index.html path at the end
    }

    enum EXECUTABLE type = executable_type(fullpath); // Check if the file is one of the executable extensions

    int ret;
    if (type != NON_EXECUTABLE) { // If the file is of one of the executable types
        ret = run_executable(socket, headers, request, utils, executable_cmd[type], fullpath);
    } else {
        ret = send_file(socket, headers, fullpath); // Attempt to serve the index file
    }

    free(fullpath);
    headers_free(headers);
    return ret;
}

int resolution_post(int socket, struct request *request, struct _srvutils *utils) {
    //create header structure
    struct httpres_headers *headers = create_header_struct();
    setDefaultHeaders(headers);

    // Calculate the size that the full path will have
    size_t fullpath_size = strlen(utils->webroot) + strlen(request->path) + 1;

    // Allocate enough space for the entire path and the null terminator
    char *fullpath = calloc(sizeof(char), fullpath_size);

    // Concatenate both parts of the path to obtain the full path
    strcat(fullpath, utils->webroot);
    strcat(fullpath, request->path);

#if DEBUG >= 2
    utils->log(stdout, "Full path: %s", fullpath);
#endif

    int ret;

    if (is_directory(fullpath)) { // If it's a directory, return a forbidden code
        ret = respond(socket, FORBIDDEN, "Can't POST there", headers, NULL, 0);
        goto end;
    }

    enum EXECUTABLE type = executable_type(fullpath); // Check if the file is one of the executable extensions

    if (type != NON_EXECUTABLE) { // If the file is of one of the executable types
        ret = run_executable(socket, headers, request, utils, executable_cmd[type], fullpath);
    } else { // If it's not an executable extension, return a forbidden code
        ret = respond(socket, FORBIDDEN, "Can't POST there", headers, NULL, 0);
    }

    end:
    free(fullpath);
    headers_free(headers);
    return ret;
}

int resolution_options(int socket) {
    struct httpres_headers *headers = create_header_struct();
    setDefaultHeaders(headers);

    set_header(headers, HDR_ALLOW, ALLOWED_OPTIONS);

    respond(socket, NO_CONTENT, "No Content", headers, NULL, 0);
    headers_free(headers);

    return NO_CONTENT;
}

enum EXECUTABLE executable_type(const char *path) {
    if (!path) return -1;

    char *ext = strrchr(path, '.'); // Find the extension
    if (!ext) return -1; // If there's no extension
    ext++; // skip the '.'

    // Return the executable type, or -1 if it's not an executable file
    if (strcmp(ext, "py") == 0) {
        return PYTHON;
    } else if (strcmp(ext, "php") == 0) {
        return PHP;
    } else {
        return NON_EXECUTABLE;
    }

}