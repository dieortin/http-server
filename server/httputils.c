/**
 * @file httputils.c
 * @brief Contains the implementation of the functions for processing and responding to HTTP requests
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
#include <sys/mman.h>

#define CRLF_LEN strlen("\r\n") ///< Length of the string containing the response code (always three digit)

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

int send_response_header(int socket, unsigned int code, const char *message, struct httpres_headers *headers) {
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

    // Size for the status line and headers
    size_t header_size = status_line_len + headers_getlen(headers) + CRLF_LEN;

    char *buffer = calloc(header_size + 1, sizeof(char)); // Allocate a buffer with the required memory
    if (!buffer) return -1;

    strcpy(buffer, status_line); // Copy the status line to the response buffer

    if (headers) {
        for (int i = 0; i < headers->num_headers; i++) {
            strcat(buffer + status_line_len, headers->headers[i]); // Add the header
            strcat(buffer, "\r\n"); // Add the header CRLF
        }
    }

    // Empty line before response body
    strcat(buffer, "\r\n");

#if DEBUG >= 3
    printf("Sending response header:\n%s\n", buffer);
#endif

    int ret = send(socket, buffer, header_size, 0);

    free(buffer);
    free(status_line);

    return ret;
}

int send_response_body(int socket, const char *body, long body_len) {
    if (!body || body_len <= 0) return -1;

#if DEBUG >= 3
    printf("Sending response body:\n%s\n", body);
#endif
    return send(socket, body, body_len + 1, 0);
}

int respond(int socket, unsigned int code, const char *message, struct httpres_headers *headers, const char *body,
            long body_len) {
#if DEBUG >= 1
    short int err = 0;
    long bytes_sent = 0;
#endif

    int ret = send_response_header(socket, code, message, headers); // Send the response header
#if DEBUG >= 1
    ret == -1 ? (err = 1) : (bytes_sent += ret);
#endif

    if (body) {
        ret = send_response_body(socket, body, body_len);
#if DEBUG >= 1
        ret == -1 ? (err = 1) : (bytes_sent += ret);
#endif
    }

#if DEBUG >= 1
    if (err) {
        perror("Error while sending response");
    } else {
        printf("Sent %li bytes\n", bytes_sent);
    }
#endif
    shutdown(socket, SHUT_WR);
    shutdown(socket, SHUT_RD);
    close(socket);

    return (int) code;
}

STATUS setDefaultHeaders(struct httpres_headers *headers) {
    if (!headers) return ERROR;

    struct tm tm;

    char timeStr[100];
    time_t now = time(0);
    gmtime_r(&now, &tm);
    strftime(timeStr, sizeof timeStr, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    //create date header
    set_header(headers, HDR_DATE, timeStr);
    //create server header
    set_header(headers, HDR_SERVER_ORIGIN, "httpServer");
    return SUCCESS;
}

// https://stackoverflow.com/a/4553076/3024970
int is_regular_file(const char *path) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(struct stat));
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode); // NOLINT(hicpp-signed-bitwise)
}

int is_directory(const char *path) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(struct stat));
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode); // NOLINT(hicpp-signed-bitwise)
}

off_t get_file_size(FILE *fd) {
    if (!fd) return -1;

    struct stat s;
    memset(&s, 0, sizeof(struct stat)); // Zero out the structure

    if (fstat(fileno(fd), &s) == -1) { // Obtain the stats for the file
        return -1; // Error while executing stat
    }

    return s.st_size; // Return the file size
}

int send_file(int socket, struct httpres_headers *headers, const char *path, struct _srvutils *utils) {
    if (!headers || !path) return ERROR;

    if (!is_regular_file(path)) { // If it's not a regular file (i.e. is a directory, pipe, link...)
        respond(socket, NOT_FOUND, "Not found", headers, NULL, 0);
        return NOT_FOUND;
    }
    FILE *f = fopen(path, "r");
    if (f) {
        long length = get_file_size(f);

        // Copy the entire file to the buffer
        char *buffer = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fileno(f), 0);

        // Add the file headers
        add_last_modified(path, headers);
        add_content_type(path, headers);
        add_content_length(length, headers);

        respond(socket, OK, "OK", headers, buffer, length);

        munmap(buffer, length); // Free the mapping

        fclose(f);
        return OK;
    } else {
        if (errno == ENOENT) {
            return respond(socket, NOT_FOUND, "Not found", NULL, NULL, 0);
        } else {
            return respond(socket, INTERNAL_ERROR, "Not found", NULL, NULL, 0);
        }
    }
}

/*from https://github.com/Menghongli/C-Web-Server/blob/master/get-mime-type.c*/
STATUS add_content_type(const char *filePath, struct httpres_headers *headers) {
    const char *content_name = NULL;
    content_name = get_mime_type(filePath);

    if (!content_name) return ERROR;

    return set_header(headers, HDR_CONTENT_TYPE, content_name);
}

STATUS add_last_modified(const char *filePath, struct httpres_headers *headers) {
    char t[100] = "";
    struct stat b;
    memset(&b, 0, sizeof(struct stat));

    stat(filePath, &b);

    struct tm tm;
    strftime(t, sizeof t, "%a, %d %b %Y %H:%M:%S %Z", localtime_r(&b.st_mtime, &tm));
    return set_header(headers, HDR_LAST_MODIFIED, t);
}


STATUS add_content_length(long length, struct httpres_headers *headers) {
    char len_str[10];
    sprintf(len_str, "%li", length);
    return set_header(headers, HDR_CONTENT_LENGTH, len_str);
}

const char *get_mime_type(const char *name) {
    if (!name) return NULL;

    char *ext = strrchr(name, '.');

    if (!ext) return NULL;
    ext++; // skip the '.';

    return mime_get_association(ext);
}

struct httpres_headers *create_header_struct() {
    struct httpres_headers *new = malloc(sizeof(struct httpres_headers));
    new->headers = NULL;
    new->num_headers = 0;
    return new;
}

STATUS set_header(struct httpres_headers *headers, const char *name, const char *value) {
    if (!headers || !name || !value) return ERROR;

    if (headers->num_headers == 0) { // If the array didn't exist, create it
        headers->headers = calloc(1, sizeof(char *));
    } else {
        headers->headers = realloc(headers->headers, (headers->num_headers + 1) * sizeof(char *));
    }

    // Calculate the needed size for allocating the header string plus the null terminator
    size_t needed_size = snprintf(NULL, 0, "%s: %s", name, value) + 1;

    // Allocate space for the string
    headers->headers[headers->num_headers] = malloc(sizeof(char) * needed_size);

    // Produce the header string
    snprintf(headers->headers[headers->num_headers], needed_size, "%s: %s", name, value);

    headers->num_headers++;
    return SUCCESS;
}

void headers_free(struct httpres_headers *headers) {
    if (!headers) return;

    for (int i = 0; i < headers->num_headers; i++) {
        free(headers->headers[i]);
    }
    if (headers->headers) free(headers->headers);

    free(headers);
}

int headers_getlen(struct httpres_headers *headers) {
    if (!headers) return 0;
    int counter = 0;
    for (int i = 0; i < headers->num_headers; i++) {
        counter += (int) strlen(headers->headers[i]);
        counter += CRLF_LEN; // also add the size of the CRLF header line terminator
    }

    return counter;
}