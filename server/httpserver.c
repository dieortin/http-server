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
 */

#include <stdio.h>
#include <zconf.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "httpserver.h"

int route(int socket, struct request *request, struct _srvutils *utils);

int resolution_get(int socket, struct request *request, struct _srvutils *utils);

int resolution_post(int socket, struct request *request, struct _srvutils *utils);

int resolution_options(int socket);

SERVERCMD processHTTPRequest(int socket, struct _srvutils *utils) {
    size_t prevbuflen = 0;

    char *buffer = calloc(MAX_HTTPREQ, sizeof(char));

    int ret = (int) read(socket, buffer, MAX_HTTPREQ);
    if (ret == -1) { // Error while reading from the socket
        utils->log(stderr, "Error while reading from socket %i: %s", socket, strerror(errno));
        respond(socket, INTERNAL_ERROR, "Internal server error", NULL, NULL, 0);
        return CONTINUE; // TODO: return STOP?
    }

    struct request *request = parseRequest(buffer, MAX_HTTPREQ, prevbuflen);

    int code = route(socket, request, utils);

    utils->log(stdout, "%s %s %i", request->method, request->path, code);

    free(buffer);
    freeRequest(request);

    return CONTINUE; /// Tell the server to continue accepting requests
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
    struct httpResHeaders *headers = create_header_struct();
    setDefaultHeaders(headers);

    // Stores the path used for the request, and might be the one required by the user or a different one if the server
    // decides so
    const char *used_path = NULL;

    if (strcmp(request->path, "/") == 0) { // If the browser gets a request for the server root
        used_path = INDEX_PATH; // Use the index path
    } else {
        used_path = request->path; // Use the path supplied by the request
    }

    // Calculate the size that the full path will have
    size_t fullpath_size = strlen(utils->webroot) + strlen(used_path) + 1;

    // Allocate enough space for the entire path and the null terminator
    char *fullpath = malloc(sizeof(char) * fullpath_size);

    // Concatenate both parts of the path to obtain the full path
    strcat(fullpath, utils->webroot);
    strcat(fullpath, used_path);

#if DEBUG >= 2
    utils->log(stdout, "Full path: %s", fullpath);
#endif

    int ret = send_file(socket, headers, fullpath, utils); // Attempt to serve the index file

    free(fullpath);
    headers_free(headers);
    return ret;
}

// TODO: Implement
int resolution_post(int socket, struct request *request, struct _srvutils *utils) {
    return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, NULL, 0);
}

int resolution_options(int socket) {
    struct httpResHeaders *headers = create_header_struct();
    setDefaultHeaders(headers);

    set_header(headers, HDR_ALLOW, ALLOWED_OPTIONS);

    respond(socket, NO_CONTENT, "No Content", headers, NULL, 0);
    headers_free(headers);

    return NO_CONTENT;
}