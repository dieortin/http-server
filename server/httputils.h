
#ifndef PRACTICA1_HTTPUTILS_H
#define PRACTICA1_HTTPUTILS_H

#include <stdio.h>
#include "picohttpparser.h"

#define HTTP_VER "HTTP/1.1"
#define BUFFER_LEN 2048

#define GET "GET"
#define POST "POST"
#define OPTIONS "OPTIONS"

struct reqStruct {
    char *method, *path;
    int minor_version;
    struct phr_header headers[100];
    size_t num_headers;
};

int processHTTPRequest(int socket);

int respond(int socket, unsigned int code, char *message, char *body);

int httpreq_print(FILE *fd, struct reqStruct *request);

int resolution_get(int socket, struct reqStruct *request);

int resolution_post(int socket, struct reqStruct *request);

int resolution_options(int socket, struct reqStruct *request);

typedef enum _HTTP_SUCCESS {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
} HTTP_SUCCESS;
typedef enum _HTTP_CLIENT_ERROR {
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405
} HTTP_CLIENT_ERROR;

typedef enum _HTTP_SERVER_ERROR {
    INTERNAL_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    HTTP_VERSION_UNSUPPORTED = 505,
} HTTP_SERVER_ERROR;

typedef enum _HTTP_SERVER_ERROR {
    GET = "GET",
    NOT_IMPLEMENTED = 501,
    HTTP_VERSION_UNSUPPORTED = 505,
} HTTP_METHODS;

#endif //PRACTICA1_HTTPUTILS_H
