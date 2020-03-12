
#include "httputils.h"
#include "constants.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

int parseRequest(const char *buf, int buflen, size_t prevbuflen, struct reqStruct *request) {
    int p;
    const char *method, *path;
    size_t method_len, path_len;
    p = phr_parse_request(buf, buflen, &method, &method_len, &path, &path_len,
                          &request->minor_version, request->headers, &request->num_headers, prevbuflen);
    request->method = malloc(method_len + 1);
    request->path = malloc(path_len + 1);
    strncpy(request->method, method, method_len);
    strncpy(request->path, path, path_len);
    return p;
}

int processHTTPRequest(int socket) {
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
        respond(socket, METHOD_NOT_ALLOWED, "Not supported", "Sorry, bad request");
    }


    return 0;
}

int respond(int socket, unsigned int code, char *message, char *body) {
    char buffer[BUFFER_LEN];

    // Response header
    if (message) {
	    sprintf(buffer, "%s %i %s\r\n", HTTP_VER, code, message);
    } else {
    	sprintf(buffer, "%s %i\r\n", HTTP_VER, code);
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

int resolution_get(int socket, struct reqStruct *request) {
    char webpath[300];
    char cwd[200];
    getcwd(cwd, 200);
    char *buffer = 0;
    long length;
    FILE *f = fopen(webpath, "rb");

    strcpy(webpath, cwd);
    strcat(webpath, "/www");
    strcat(webpath, request->path);
    printf("%s", webpath);

    ///si el archivo existe
    if (access(webpath, F_OK) == 0) {

        if (f) {
            fseek(f, 0, SEEK_END);
            length = ftell(f);
            fseek(f, 0, SEEK_SET);
            buffer = malloc(length);
            if (buffer) {
                fread(buffer, 1, length, f);    ///se introduce en el buffer el archivo
            }
            fclose(f);
        }

        respond(socket, OK, "Solved", buffer);
    } else if (errno == ENOENT) {  ///si el archivo no existe
        respond(socket, NOT_FOUND, "Not found", "Sorry, the requested resource was not found at this server");
    } else {   ///otro error
        respond(socket, INTERNAL_ERROR, "Not found", "Sorry, the requested resource can't be accessed");
    }
    return 0;
}

int resolution_post(int socket, struct reqStruct *request) {
    return 0;
}

int resolution_options(int socket, struct reqStruct *request) {
    return 0;
}