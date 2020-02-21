
#ifndef PRACTICA1_HTTPUTILS_H
#define PRACTICA1_HTTPUTILS_H

#include <stdio.h>
#include "picohttpparser.h"

#define HTTP_VER "HTTP/1.1"
#define BUFFER_LEN 2048

// Como máximo 7 caracteres de método (ej. OPTIONS) y caracter de fin de cadena
#define MAX_METHOD 8

#define MAX_HTTPVER 10

// Arbitrarios
#define MAX_URL 150
#define MAX_HOST 50
#define MAX_PATH 50
#define MAX_QUERYSTRING 50
#define MAX_BODY 1024

struct httpreq_data {
	char method[MAX_METHOD];

	char url[MAX_URL];

	char host[MAX_HOST];
    char path[MAX_PATH];
    char querystring[MAX_QUERYSTRING];

    char httpver[MAX_HTTPVER];
    char **headers;
    int num_headers;

    char body[MAX_BODY];
};

struct reqStruct {
    const char *method, *path;
    int minor_version;
    struct phr_header headers[100];
    size_t method_len, path_len, num_headers;
};

int processHTTPRequest(int socket);

int respond(int socket, int code, char *message, char *body);

int httpreq_print(FILE *fd, struct httpreq_data *request);

#endif //PRACTICA1_HTTPUTILS_H
