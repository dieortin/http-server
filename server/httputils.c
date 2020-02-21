
#include "httputils.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


int parseRequest(const char *buf, int buflen, size_t prevbuflen, struct reqStruct *request) {

    return phr_parse_request(buf, buflen, &request->method, &request->method_len, &request->path, &request->path_len,
                             &request->minor_version, request->headers, &request->num_headers, prevbuflen);
}

int processHTTPRequest(int socket) {
    struct httpreq_data request;
    // Zero out the structure
    memset(&request, 0, sizeof request);

	char buffer[BUFFER_LEN];
	memset(buffer, 0, sizeof buffer);


	read(socket, buffer, BUFFER_LEN);

	printf("-------BEGIN-----------\n%s\n-------END------\n", buffer);

    printf("------END----------------\n");
	respond(socket, 404, "Not found", "Sorry, the requested resource was not found at this server");
	return 0;
}

int respond(int socket, int code, char *message, char *body) {
	char buffer[BUFFER_LEN];

	// Response header
	sprintf(buffer, "%s %i %s\r\n", HTTP_VER, code, message);

	// Empty line before response body
	sprintf(buffer + strlen(buffer), "\r\n");

	// Response body
	sprintf(buffer + strlen(buffer), "%s\r\n", body);

	send(socket, buffer, strlen(buffer), 0);

	printf("--------------------\nResponse sent:\n%s\n", buffer);
	close(socket);

	return 0;
}

int httpreq_print(FILE *fd, struct httpreq_data *request) {
	if (!fd || !request) return -1;

	fprintf(fd, "httpreq_data:{\n\tmethod='%s'\n\turl='%s'\n\thttpver='%s'\n\tbody='%s'\n}\n", request->method,
	        request->url, request->httpver, request->body);
	return 0;
}
