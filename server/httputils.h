
#ifndef PRACTICA1_HTTPUTILS_H
#define PRACTICA1_HTTPUTILS_H

#include <stdio.h>
#include "picohttpparser.h"
#include "server.h"

#define HTTP_VER "HTTP/1.1"
#define BUFFER_LEN 2048

#define GET "GET"
#define POST "POST"
#define OPTIONS "OPTIONS"

#define HDR_DATE "Date"
#define HDR_SERVER_ORIGIN "Server"
#define HDR_LAST_MODIFIED "Last-Modified"
#define HDR_CONTENT_LENGTH "Content-Length"
#define HDR_CONTENT_TYPE "Content-Type"

#define HEADER_EXTRA_SPACE 3 ///< Space required for ": " and the null termination character in the headers


struct request {
    const char *method, *path;
    int minor_version;
    struct phr_header headers[100];
    size_t num_headers;
};

struct httpResHeaders {
    int num_headers;
    char **headers;
};

SERVERCMD processHTTPRequest(int socket, void (*log)(const char *fmt, ...));

int respond(int socket, unsigned int code, char *message, struct httpResHeaders *headers, char *body);

int httpreq_print(FILE *fd, struct request *request);

int resolution_get(int socket, struct request *request);

int resolution_post(int socket, struct request *request);

int resolution_options(int socket, struct request *request);

STATUS set_header(struct httpResHeaders *headers, char *name, char *value);

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


#endif //PRACTICA1_HTTPUTILS_H
