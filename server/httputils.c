/**
 * @file httputils.c
 * @brief Contains the implementation of the functions for requests and responds
 * @author Diego Ortín Fernández & Mario Lopez
 * @date February 2020
 */
#include "httputils.h"
#include "constants.h"
#include "mimetable.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

#define MIMETYPE "mime.tsv"

STATUS setDefaultHeaders(struct httpResHeaders *headers);

#define CRLF_LEN strlen("\r\n") ///< Length of the string containing the response code (always three digit)

int route(int socket, struct request *request);

//int parseRequest(const char *buf, int buflen, size_t prevbuflen, struct request *request) {
//    int p;
//    const char *method, *path;
//    size_t method_len, path_len;
//    p = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
//                          &request->minor_version, request->headers, &request->num_headers, prevbuflen);
//    request->method = calloc(method_len + 1, sizeof(char));
//    request->path = calloc(path_len + 1, sizeof(char));
//    strncpy(request->method, method, method_len);
//    strncpy(request->path, path, path_len);
//
//    return p;
//}

struct request *parseRequest(const char *buf, int buflen, size_t prevbuflen) {
    if (!buf) return NULL;

    struct request *req = calloc(1, sizeof(struct request));
    if (!req) return NULL;
    // Zero out the structure
    memset(req, 0, sizeof(struct request));


    // Temporal variables to store the method and path positions before copying them to the new structure
    const char *tmp_method, *tmp_path;
    size_t tmp_method_len, tmp_path_len;


    phr_parse_request(buf, (size_t) buflen, &tmp_method, &tmp_method_len, &tmp_path, &tmp_path_len, &req->minor_version,
                      req->headers, &req->num_headers, prevbuflen);

    req->method = calloc(tmp_method_len + 1, sizeof(char));
    req->path = calloc(tmp_path_len + 1, sizeof(char));

    strncpy((char *) req->method, tmp_method, tmp_method_len);
    strncpy((char *) req->path, tmp_path, tmp_path_len);

    return req;
}

STATUS freeRequest(struct request *request) {
    if (!request) return ERROR;

    free((char *) request->path);
    free((char *) request->method);
    free(request);

    return SUCCESS;
}


SERVERCMD processHTTPRequest(int socket, void (*log)(const char *, ...)) {
    size_t prevbuflen = 0;

    char *buffer = calloc(MAX_HTTPREQ, sizeof(char));

    int ret = (int) read(socket, buffer, MAX_HTTPREQ);
    if (ret == -1) { // Error while reading from the socket
        log("Error while reading from socket %i: %s", socket, strerror(errno));
        respond(socket, INTERNAL_ERROR, "Internal server error", NULL, NULL, 0);
        return CONTINUE; // TODO: return STOP?
    }


//    printf("-------BEGIN-----------\n%s\n-------END------\n", buffer);
//
//    printf("------END----------------\n");

    struct request *request = parseRequest(buffer, MAX_HTTPREQ, prevbuflen);
    //httpreq_print(stdout, request);

    int code = route(socket, request);

    log("%s %s %i", request->method, request->path, code);

    freeRequest(request);

    return CONTINUE; /// Tell the server to continue accepting requests
}

int route(int socket, struct request *request) {
    if (strcmp(request->method, GET) == 0) {
        return resolution_get(socket, request);
    } else if (strcmp(request->method, POST) == 0) {
        return resolution_post(socket, request);
    } else if (strcmp(request->method, OPTIONS) == 0) {
        return resolution_options(socket, request);
    } else {
        return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, NULL, 0);
    }
}

int respond(int socket, unsigned int code, char *message, struct httpResHeaders *headers, char *body, long body_len) {
    char *status_line = NULL;
    size_t status_line_len = 0;

    // Response header
    if (message) {
        // First, calculate the size needed for the status line buffer
        status_line_len = (size_t) snprintf(NULL, 0, "%s %i %s\r\n", HTTP_VER, code, message);
        status_line = malloc(status_line_len + 1); // Allocate memory for the status line and null-terminator
        sprintf(status_line, "%s %i %s\r\n", HTTP_VER, code, message); // Print the status line
    } else {
        // First, calculate the size needed for the status line buffer
        status_line_len = (size_t) snprintf(NULL, 0, "%s %i\r\n", HTTP_VER, code);
        status_line = malloc(status_line_len + 1); // Allocate memory for the status line and null-terminator
        sprintf(status_line, "%s %i\r\n", HTTP_VER, code); // Print the status line
    }

    // Size for the status line, headers, body and null terminator
    size_t response_size = status_line_len + headers_getlen(headers) + CRLF_LEN + body_len + 1;

    char *buffer = calloc(response_size, sizeof(char)); // Allocate a buffer with the required memory

    strcpy(buffer, status_line); // Copy the status line to the response buffer

    if (headers) {
        for (int i = 0; i < headers->num_headers; i++) {
            strcat(buffer + status_line_len, headers->headers[i]); // Add the header
            strcat(buffer, "\r\n"); // Add the header CRLF
        }
    }

    // Empty line before response body
    strcat(buffer, "\r\n");

    //printf("Length is %li\n", body_len);

    if (body)
        memcpy(buffer + strlen(buffer), body,
               (size_t) body_len); // Copy the body using memcpy (we're not always working with strings)

//    // Response body
//    if (body) {
//        strcat(buffer, body);
//        strcat(buffer, "\r\n");
//    }
//    strcat(buffer, "\r\n");

    int ret = (int) send(socket, buffer, response_size, 0);
    if (ret < 0) {
        perror("Error while sending response");
    } else {
        //printf("Sent %i bytes\n", ret);
    }

    if (DEBUG >= 2) {
        printf("--------------------\nResponse sent:\n%s\n", buffer);
    }
    close(socket);

    free(status_line);

    return 0;
}

//int httpreq_print(FILE *fd, struct request *request) {
//    int i;
//    if (!fd || !request) return -1;
//
//    fprintf(fd,
//            "httpreq_data:{\n\tmethod='%s' \n\tpath='%s' \n\tminor_version='%d'\n\tnum_headers='%zu'\n}\n",
//            request->method,
//            request->path, request->minor_version, request->num_headers);
//    for (i = 0; i < request->num_headers; ++i) {
//        fprintf(fd,
//                "headers:{\n\theader name='%.*s' \n\theader value='%.*s'\n}\n",
//                (int) request->headers->name_len, request->headers->name,
//                (int) request->headers->value_len, request->headers->value);
//    }
//    return 0;
//}

int resolution_get(int socket, struct request *request) {
    //create header structure
    struct httpResHeaders *headers = create_header_struct();


    setDefaultHeaders(headers);

    return send_file(socket, headers, request->path);
}

STATUS setDefaultHeaders(struct httpResHeaders *headers) {
    if (!headers) return ERROR;

    char timeStr[100];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(timeStr, sizeof timeStr, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    //create date header
    set_header(headers, HDR_DATE, timeStr);
    //create server header
    set_header(headers, HDR_SERVER_ORIGIN, "httpServer");
    return SUCCESS;
}

STATUS set_header(struct httpResHeaders *headers, const char *name, const char *value) {
    if (!headers || !name || !value) return ERROR;

    if (headers->num_headers == 0) {
        headers->headers = calloc(1, sizeof(char *));
    } else {
        headers->headers = realloc(headers->headers, (headers->num_headers + 1) * sizeof(char *));
    }

    headers->headers[headers->num_headers] = calloc(strlen(name) + strlen(value) + HEADER_EXTRA_SPACE, sizeof(char));
    sprintf(headers->headers[headers->num_headers], "%s: %s", name, value);

    headers->num_headers++;
    return SUCCESS;
}


STATUS send_file(int socket, struct httpResHeaders *headers, const char *path) {
    if (!headers || !path) return ERROR;

    char *buffer = NULL;
    long length;
    char webpath[300];
    char cwd[200];
    memset(webpath, 0, 300 * sizeof(char));
    memset(cwd, 0, 200 * sizeof(char));
    getcwd(cwd, 200);

    strcpy(webpath, cwd);
    strcat(webpath, "/www");
    strcat(webpath, path);

    int returnCode;

    // TODO: Check if it's a directory or a file
    FILE *f = fopen(webpath, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = calloc((size_t) (length + 1), sizeof(char));

        fread(buffer, sizeof(char), (size_t) length, f);    //se introduce en el buffer el archivo
        if (ferror(f)) {
            printf("Read error\n"); // TODO: Remove
        }

        char length_str[20];
        memset(length_str, 0, 20);
        sprintf(length_str, "%li", length);
        set_header(headers, HDR_CONTENT_LENGTH, length_str);
        //set_header(&headers, HDR_CONTENT_TYPE, "image/jpeg");

        //crear headers de archivo
        add_last_modified(path, headers);
        add_content_type(path, headers);
        add_content_length(length, headers);

        respond(socket, OK, "OK", headers, buffer, length);
        fclose(f);
        returnCode = OK;
    } else {
        if (errno == ENOENT) {
            respond(socket, NOT_FOUND, "Not found", NULL, NULL, 0);
            returnCode = NOT_FOUND;
        } else {
            respond(socket, INTERNAL_ERROR, "Not found", NULL, NULL, 0);
            returnCode = INTERNAL_ERROR;
        }
    }

    headers_free(headers);

    return returnCode;
}

// TODO: Implement
int resolution_post(int socket, struct request *request) {
    return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, NULL, 0);
}

// TODO: Implement
int resolution_options(int socket, struct request *request) {
    return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, NULL, 0);
}

/*from https://github.com/Menghongli/C-Web-Server/blob/master/get-mime-type.c*/
STATUS add_content_type(const char *filePath, struct httpResHeaders *headers) {
    const char *content_name = NULL;
    content_name = get_mime_type(filePath);

    if (!content_name) return ERROR;

    return set_header(headers, HDR_CONTENT_TYPE, content_name);
}

STATUS add_last_modified(const char *filePath, struct httpResHeaders *headers) {
    char t[100] = "";
    struct stat b;
    stat(filePath, &b);
    strftime(t, 100, "%a, %d %b %Y %H:%M:%S %Z", localtime(&b.st_mtime));
    return set_header(headers, HDR_LAST_MODIFIED, t);
}


STATUS add_content_length(long length, struct httpResHeaders *headers) {
    char len_str[10];
    sprintf(len_str, "%li", length);
    return set_header(headers, HDR_CONTENT_LENGTH, len_str);
}

const char *get_mime_type(const char *name) {
    char *ext = strrchr(name, '.');
    ext++; // skip the '.';

    return mime_get_association(ext);
}

struct httpResHeaders *create_header_struct() {
    struct httpResHeaders *new = malloc(sizeof(struct httpResHeaders));
    new->headers = NULL;
    new->num_headers = 0;
    return new;
}

void headers_free(struct httpResHeaders *headers) {
    for (int i = 0; i < headers->num_headers; i++) {
        free(headers->headers[i]);
    }
    free(headers->headers);
}

int headers_getlen(struct httpResHeaders *headers) {
    if (!headers) return 0;
    int counter = 0;
    for (int i = 0; i < headers->num_headers; i++) {
        counter += (int) strlen(headers->headers[i]);
        counter += CRLF_LEN; // also add the size of the CRLF header line terminator
    }

    return counter;
}