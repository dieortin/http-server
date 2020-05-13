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
#include "../picohttpparser/picohttpparser.h"
#include "server.h"
#include "constants.h"

#define HTTP_VER "HTTP/1.1" ///< HTTP version used by the server

#define MAX_HTTPREQ (1024 * 8) ///< Maximum size of an HTTP request in any browser (Firefox in this case)
#define MAX_HEADERS 100 ///< Maximum number of HTTP headers supported

#define GET "GET" ///< String for the GET method
#define POST "POST" ///< String for the POST method
#define OPTIONS "OPTIONS" ///< String for the OPTIONS method
#define ALLOWED_OPTIONS "GET, POST, OPTIONS" ///< String representing the allowed HTTP methods

#define HDR_DATE "Date" ///< HTTP Date header name
#define HDR_SERVER_ORIGIN "Server" ///< HTTP Server header name
#define HDR_LAST_MODIFIED "Last-Modified" ///< HTTP Last-Modified header name
#define HDR_CONTENT_LENGTH "Content-Length" ///< HTTP Content-Length header name
#define HDR_CONTENT_TYPE "Content-Type" ///< HTTP Content-Type header name
#define HDR_ALLOW "Allow" ///< HTTP Allow header name

#define INDEX_PATH "/index.html" ///< Default path of the index file in a folder

/**
 * @brief Codes representing the result of the parsing operation
 */
typedef enum _parse_result {
    PARSE_OK, ///< The parsing operation completed successfully
    PARSE_ERROR, ///< There was an error while parsing
    PARSE_REQTOOLONG, ///< The request to parse is too long
    PARSE_IOERROR, ///< There was an error while reading the request
    PARSE_INTERNALERR ///< There was an internal error
} parse_result;

/**
 * @brief Various HTTP response codes
 * @see rfc2616
 */
typedef enum _HTTP_RESPONSE_CODE {
    OK = 200,
    //CREATED = 201,
            NO_CONTENT = 204,
    BAD_REQUEST = 400,
    //UNAUTHORIZED = 401,
            FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_ERROR = 500,
    //NOT_IMPLEMENTED = 501,
    //HTTP_VERSION_UNSUPPORTED = 505,
} HTTP_RESPONSE_CODE;

/**
 * @struct request
 * @brief Stores all the data related to an HTTP request
 */
struct request {
    char *reqbuf; ///< The request in full
    const char *method; ///< HTTP Method of the request
    const char *path; ///< Path of the request
    const char *querystring; ///< Querystring part of the path
    const char *body; ///< Body of the request
    unsigned long body_len; ///< Length of the request body
    int minor_version; ///< HTTP version of the request
    struct phr_header headers[MAX_HEADERS]; ///< Structure containing the request headers
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

/**
 * @brief Sends an HTTP response to the given socket
 * @param[out] socket Socket to send the response to
 * @param[in] code Reponse code for the HTTP response
 * @param[in] message Message for the response header line
 * @param[in] headers Structure containing the headers for the response (can be NULL)
 * @param[in] body Body of the request (can be NULL)
 * @param[in] body_len Length of the body
 * @return response code sent (\p code)
 */
HTTP_RESPONSE_CODE
respond(int socket, HTTP_RESPONSE_CODE code, const char *message, struct httpres_headers *headers, const char *body,
        unsigned long body_len);

/**
 * @brief Reads an HTTP request from the given socket, parses it, and creates a \ref request structure with the results
 * @param[in] socket The socket to read from
 * @param[out] request Pointer where the newly created \ref request structure will be stored
 * @return \ref parse_result.PARSE_OK if parsing went correctly, a different member of the enum otherwise depending
 * on the error
 */
parse_result parseRequest(int socket, struct request **request);

/**
 * @brief Frees all the memory associated with the request structure
 * @param[in] request The request structure to free
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS freeRequest(struct request *request);

/**
 * @brief Creates a new structure for storing HTTP response headers
 * @return New header structure, or NULL if an error happens
 */
struct httpres_headers *create_header_struct();

/**
 * @brief Adds a new header with the provided name and value to the header structure given
 * @param[out] headers Header structure where the new one must be added
 * @param[in] name String representing the name of the header
 * @param[in] value String representing the value of the header
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS set_header(struct httpres_headers *headers, const char *name, const char *value);

/**
 * @brief Frees all the memory associated with a header structure
 * @param[in] headers Header structure to free
 */
void headers_free(struct httpres_headers *headers);

/**
 * @brief Returns the combined length of the headers contained in the structure, taking into
 * account the CRLF line terminators
 * @param[in] headers Header structure whose length must be calculated
 * @return Combined length of the headers
 */
int headers_getlen(struct httpres_headers *headers);

/**
 * @brief Checks if the provided path is a regular file
 * @param[in] path The path to be checked
 * @return 1 if the path is a regular file, 0 otherwise
 */
int is_regular_file(const char *path);

/**
 * @brief Checks if the provided path is a directory
 * @param[in] path The path to be checked
 * @return 1 if the path is a directory, 0 otherwise
 */
int is_directory(const char *path);

/**
 * @brief Sends the file at the given path to the provided socket as an HTTP response with the appropiate headers
 * @param[out] socket The socket to which the response must be sent
 * @param[in] headers Structure containing the headers for the response
 * @param[in] path Path where the file to be sent resides
 * @return code of the HTTP response sent to the socket
 */
HTTP_RESPONSE_CODE send_file(int socket, struct httpres_headers *headers, const char *path);

/**
 * @brief Executes the script in the request path using the provided command, passing arguments to it via stdin
 * @details This function runs the provided command with the path as its first parameter. It then writes the
 * querystring and the POST parameters to its standard input. Finally, it reads the result of the script execution,
 * and sends it to the socket as an HTTP response.
 * @author Diego Ortín Fernández
 * @param[out] socket The socket to which the response must be sent
 * @param[in] headers Structure containing the headers for the response
 * @param[in] request Request from which the data must be obtained
 * @param[in] utils Structure containing utilities (used for logging)
 * @param[in] exec_cmd Command to be used
 * @param[in[ fullpath Absolute path of the executable file in the system
 * @return 0 if an error occurs, 1 otherwise
 */
int run_executable(int socket, struct httpres_headers *headers, struct request *request, struct _srvutils *utils,
                   const char *exec_cmd, const char *fullpath);

/**
 * @brief Sets the Last-Modified header to the last modified date of the provided date
 * @author Mario López
 * @param[in] filePath Path of the file
 * @param[out] headers Structure where the header must be set
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS add_last_modified(const char *filePath, struct httpres_headers *headers);

/**
 * @brief Obtains the MIME type of a file name
 * @param[in] name The name of the file
 * @return String representing the MIME type of the filename, or NULL if it has no extension or an error happens
 */
const char *get_mime_type(const char *name);

/**
 * @brief Sets the Content-Type header to the MIME type of the provided file
 * @param[in] filePath Path of the file
 * @param[out] headers Structure where the header must be set
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS add_content_type(const char *filePath, struct httpres_headers *headers);

/**
 * @brief Sets the Content-Length header to the provided one
 * @author Mario López
 * @param[in] length Value to set
 * @param[out] headers Structure where the header must be set
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS add_content_length(long length, struct httpres_headers *headers);

/**
 * @brief Sets the date and server signature headers
 * @param[out] headers Structure where the headers must be set
 * @return \ref STATUS.SUCCESS if everything went well, \ref STATUS.ERROR otherwise
 */
STATUS setDefaultHeaders(struct httpres_headers *headers);


#endif //PRACTICA1_HTTPUTILS_H
