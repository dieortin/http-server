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

int parseRequest(const char *buf, int buflen, size_t prevbuflen, struct reqStruct *request) {
    int p;
    const char *method, *path;
    size_t method_len, path_len;
    p = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                          &request->minor_version, request->headers, &request->num_headers, prevbuflen);
    request->method = calloc(method_len + 1, sizeof(char));
    request->path = calloc(path_len + 1, sizeof(char));
    strncpy(request->method, method, method_len);
    strncpy(request->path, path, path_len);

    return p;
}

SERVERCMD processHTTPRequest(int socket) {
    struct reqStruct request;
    size_t prevbuflen = 0;
    // Zero out the structure
    memset(&request, 0, sizeof request);

    char buffer[BUFFER_LEN];
    memset(buffer, 0, sizeof buffer);


    read(socket, buffer, BUFFER_LEN);

    printf("-------BEGIN-----------\n%s\n-------END------\n", buffer);

    printf("------END----------------\n");

    parseRequest(buffer, BUFFER_LEN, prevbuflen, &request);
    httpreq_print(stdout, &request);

    if (strcmp(request.method, GET) == 0) {
        resolution_get(socket, &request);
    } else if (strcmp(request.method, POST) == 0) {
        resolution_post(socket, &request);
    } else if (strcmp(request.method, OPTIONS) == 0) {
        resolution_options(socket, &request);
    } else {
        //respond(socket, METHOD_NOT_ALLOWED, "Not supported", "Sorry, bad request");
    }
    free(request.method);
    free(request.path);
    return CONTINUE; /// Tell the server to continue accepting requests
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

int httpreq_print(FILE *fd, struct reqStruct *request) {
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

STATUS resolution_get(int socket, struct reqStruct *request) {
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
    printf("%s", webpath);

    set_header(&headers, Date, timeStr);

    //si el archivo existe
    if (access(webpath, F_OK) == 0) {
       FILE *f = fopen(webpath, "r");
        if (!f) {
            //respond(socket, INTERNAL_ERROR, "can't open", "Sorry, the requested resource could not be accessed");
            return ERROR;
        }

        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = calloc(length + 1, sizeof(char));
        if (!buffer) {
            //respond(socket, INTERNAL_ERROR, "can't open", "Sorry, the requested resource could not be accessed");
            return ERROR;
        }
        fread(buffer, sizeof(char), length, f);    //se introduce en el buffer el archivo
        respond(socket, OK, "Solved", &headers, buffer);
        fclose(f);
        return SUCCESS;

    } else if (errno == ENOENT) {  //si el archivo no existe
        //respond(socket, NOT_FOUND, "Not found", "Sorry, the requested resource was not found at this server");
        return SUCCESS;
    } else {   //otro error
        //respond(socket, INTERNAL_ERROR, "Not found", "Sorry, the requested resource can't be accessed");
        return ERROR;
    }
}

int resolution_post(int socket, struct reqStruct *request) {
    return 0;
}

int resolution_options(int socket, struct reqStruct *request) {
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