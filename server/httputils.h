/**
 * @file httputils.h
 * @brief Module containing functions and data types useful for the operation of an HTTP server.
 * @details It offers methods for parsing, processing and responding to HTTP requests, with full header support.
 * @author Diego Ortín Fernández & Mario Lopez
 * @date February 2020
 */

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
#define ALLOWED_OPTIONS "GET, POST, OPTIONS"

#define HDR_DATE "Date"
#define HDR_SERVER_ORIGIN "Server"
#define HDR_LAST_MODIFIED "Last-Modified"
#define HDR_CONTENT_LENGTH "Content-Length"
#define HDR_CONTENT_TYPE "Content-Type"
#define HDR_ALLOW "Allow"

#define INDEX_PATH "/index.html"

/**
 * @struct request
 * @brief Stores all the data related to an HTTP request
 */
struct request {
    const char *method; ///< HTTP Method of the request
    const char *path; ///< Path of the request
    const char *querystring; ///< Querystring part of the path
    int minor_version; ///< HTTP version of the request
    struct phr_header headers[100]; ///< Structure containing the request headers
    size_t num_headers; ///< Number of headers in the request
};

/**
 * @struct httpres_headers
 * @brief Stores the headers that must be sent with an HTTP response
 */
struct httpres_headers {
    int num_headers; ///< Number of headers in the structure
    char **headers; ///< Array of strings containing the full headers
};


int respond(int socket, unsigned int code, const char *message, struct httpres_headers *headers, const char *body,
            unsigned long body_len);

struct request *parseRequest(const char *buf, int buflen, size_t prevbuflen);

STATUS freeRequest(struct request *request);

struct httpres_headers *create_header_struct();

STATUS set_header(struct httpres_headers *headers, const char *name, const char *value);

void headers_free(struct httpres_headers *headers);

int headers_getlen(struct httpres_headers *headers);

int is_regular_file(const char *path);

int is_directory(const char *path);

int send_file(int socket, struct httpres_headers *headers, const char *path, struct _srvutils *utils);

int
run_executable(int socket, struct httpres_headers *headers, const char *querystring, struct _srvutils *utils,
               const char *exec_cmd, const char *path);

STATUS add_last_modified(const char *filePath, struct httpres_headers *headers);

const char *get_mime_type(const char *name);

STATUS add_content_type(const char *filePath, struct httpres_headers *headers);

STATUS add_content_length(long length, struct httpres_headers *headers);

STATUS setDefaultHeaders(struct httpres_headers *headers);

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
