/**
 * @file httputils.c
 * @brief Contains the implementation of the functions for requests and responds
 * @author Diego Ortín Fernández & Mario Lopez
 * @date February 2020
 */
#include "httputils.h"
#include "constants.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

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


    phr_parse_request(buf, buflen, &tmp_method, &tmp_method_len, &tmp_path, &tmp_path_len, &req->minor_version,
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


SERVERCMD processHTTPRequest(int socket, void (*log)(const char *fmt, ...)) {
    size_t prevbuflen = 0;

    char buffer[BUFFER_LEN];
    memset(buffer, 0, sizeof buffer);


    read(socket, buffer, BUFFER_LEN);

//    printf("-------BEGIN-----------\n%s\n-------END------\n", buffer);
//
//    printf("------END----------------\n");

    struct request *request = parseRequest(buffer, BUFFER_LEN, prevbuflen);
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
        return respond(socket, METHOD_NOT_ALLOWED, "Not supported", NULL, "Sorry, bad request");
    }
}

int respond(int socket, unsigned int code, char *message, struct httpResHeaders *headers, char *body) {
    char buffer[BUFFER_LEN];
    int i;

    // Response header
    if (message) {
        sprintf(buffer, "%s %i %s\r\n", HTTP_VER, code, message);
    } else {
        sprintf(buffer, "%s %i\r\n", HTTP_VER, code);
    }

    if (headers) {
        for (i = 0; i < headers->num_headers; i++) {
            sprintf(buffer + strlen(buffer), "%s\r\n", headers->headers[i]);
        }
    }

    // Empty line before response body
    sprintf(buffer + strlen(buffer), "\r\n");

    // Response body
    if (body) {
        sprintf(buffer + strlen(buffer), "%s\r\n", body);
    } else {
        sprintf(buffer + strlen(buffer), "\r\n");
    }

    send(socket, buffer, strlen(buffer), 0);

    if (DEBUG) {
        printf("--------------------\nResponse sent:\n%s\n", buffer);
    }
    close(socket);

    return 0;
}

int httpreq_print(FILE *fd, struct request *request) {
    int i;
    if (!fd || !request) return -1;

    fprintf(fd,
            "httpreq_data:{\n\tmethod='%s' \n\tpath='%s' \n\tminor_version='%d'\n\tnum_headers='%zu'\n}\n",
            request->method,
            request->path, request->minor_version, request->num_headers);
    for (i = 0; i < request->num_headers; ++i) {
        fprintf(fd,
                "headers:{\n\theader name='%.*s' \n\theader value='%.*s'\n}\n",
                (int) request->headers->name_len, request->headers->name,
                (int) request->headers->value_len, request->headers->value);
    }
    return 0;
}

int resolution_get(int socket, struct request *request) {
    char webpath[300];
    char cwd[200];
    memset(webpath, 0, 300 * sizeof(char));
    memset(cwd, 0, 200 * sizeof(char));
    getcwd(cwd, 200);
    char *buffer = NULL;
    long length;
    //create header structure
    struct httpResHeaders headers;
    memset(&headers, 0, sizeof headers);
    //create http date
    char timeStr[1000];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(timeStr, sizeof timeStr, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    strcpy(webpath, cwd);
    strcat(webpath, "/www");
    strcat(webpath, request->path);
    //printf("%s", webpath);

    set_header(&headers, HDR_DATE, timeStr);

    //si el archivo existe
    // TODO: Check if it's a directory or a file
    FILE *f = fopen(webpath, "r");
    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = calloc(length + 1, sizeof(char));

        fread(buffer, sizeof(char), length, f);    //se introduce en el buffer el archivo
        respond(socket, OK, "OK", &headers, buffer);
        fclose(f);
        return OK;
    } else {
        if (errno == ENOENT) {
            respond(socket, NOT_FOUND, "Not found", NULL, "Sorry, the requested resource was not found at this server");
            return NOT_FOUND;
        } else {
            respond(socket, INTERNAL_ERROR, "Not found", NULL, "Sorry, the requested resource can't be accessed");
            return INTERNAL_ERROR;
        }
    }
}

int resolution_post(int socket, struct request *request) {
    return 0;
}

int resolution_options(int socket, struct request *request) {
    return 0;
}

STATUS set_header(struct httpResHeaders *headers, char *name, char *value) {
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