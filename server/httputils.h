
#ifndef PRACTICA1_HTTPUTILS_H
#define PRACTICA1_HTTPUTILS_H

#include <stdio.h>
#include "picohttpparser.h"

#define HTTP_VER "HTTP/1.1"
#define BUFFER_LEN 2048

struct reqStruct {
    const char *method, *path;
    int minor_version;
    struct phr_header headers[100];
    size_t method_len, path_len, num_headers;
};

int processHTTPRequest(int socket);

int respond(int socket, int code, char *message, char *body);

int httpreq_print(FILE *fd, struct reqStruct *request);

#endif //PRACTICA1_HTTPUTILS_H
