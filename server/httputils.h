
#ifndef PRACTICA1_HTTPUTILS_H
#define PRACTICA1_HTTPUTILS_H

#include <stdio.h>
#include "picohttpparser.h"
#include "server.h"

#define HTTP_VER "HTTP/1.1"

#define MAX_HTTPREQ (1024 * 8) ///< Maximum size of an HTTP request in any browser (Firefox in this case)

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

int respond(int socket, unsigned int code, char *message, struct httpResHeaders *headers, char *body, long body_len);

int httpreq_print(FILE *fd, struct request *request);

int resolution_get(int socket, struct request *request);

int resolution_post(int socket, struct request *request);

int resolution_options(int socket, struct request *request);

struct httpResHeaders *create_header_struct();

STATUS set_header(struct httpResHeaders *headers, const char *name, const char *value);

void headers_free(struct httpResHeaders *headers);

int headers_getlen(struct httpResHeaders *headers);

STATUS send_file(int socket, struct httpResHeaders *headers, const char *path);

STATUS add_last_modified(const char *filePath, struct httpResHeaders *headers);

const char *get_mime_type(const char *name);

STATUS add_content_type(const char *filePath, struct httpResHeaders *headers);

STATUS add_content_length(long length, struct httpResHeaders *headers);

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
