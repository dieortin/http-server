#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "httputils.h"
#include "constants.h"

int parseRequest(const char *buf, int buflen, size_t prevbuflen, struct reqStruct *request) {

    return phr_parse_request(buf, buflen, &request->method, &request->method_len, &request->path, &request->path_len,
                             &request->minor_version, request->headers, &request->num_headers, prevbuflen);
}

void processHTTPRequest(int socket) {
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

    respond(socket, 404, "Not found", "Sorry, the requested resource was not found at this server");
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
            "httpreq_data:{\n\tmethod='%.*s' \n\tpath='%.*s' \n\tminor_version='%d'\n\tnum_headers='%zu'\n}\n",
            (int) request->method_len, request->method,
            (int) request->path_len, request->path, request->minor_version, request->num_headers);
    for (i = 0; i < request->num_headers; ++i) {
        fprintf(fd,
                "headers:{\n\theader name='%.*s' \n\theader value='%.*s'\n}\n",
                (int) request->headers->name_len, request->headers->name,
                (int) request->headers->value_len, request->headers->value);
    }
    return 0;
}
