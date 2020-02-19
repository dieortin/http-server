
#include "httputils.h"

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define SPACE_SEPARATOR " "
#define CR_SEPARATOR "\r\n"
#define DOUBLE_CR_SEPARATOR "\r\n\r\n"

int countlines(const char *buffer, int bufsiz) {
	if (!buffer) return -1;

	int count = 1;

	for (int i = 0; i < bufsiz; i++) {
		if (buffer[i] == '\n') count++;
		if (buffer[i] == '\0') break;
	}

	return count;
}

int parseRequestData(const char *buffer, int bufsiz, struct httpreq_data *request) {
	if (!buffer || !request) return -1;

	int linenum = countlines(buffer, bufsiz);
	char **lines = calloc(linenum, sizeof(char *));
	int current_line_num = 0;

	char *buffer_copy = strdup(buffer);
	char *current_line = NULL, *last = NULL;

	for (current_line = strtok_r(buffer_copy, "\n", &last);
	     current_line;
	     current_line = strtok_r(NULL, "\n", &last)) {
		printf("Current line: %s\n", current_line);
		lines[current_line_num] = calloc(strlen(current_line) + 1, sizeof(char));
		strcpy(lines[current_line_num++], current_line);
	}

	return 0;
}

/*
int parseRequestData(const char *buffer, struct httpreq_data *request) {
	if (!buffer || !request) return -1;

	char all_headers[MAX_BODY];
	char *last = NULL, *current_header, *headerlast = NULL;
	char *request_copy = strdup(buffer);

	strcpy(request->method, strtok_r(request_copy, SPACE_SEPARATOR, &last));
	strcpy(request->url, strtok_r(NULL, SPACE_SEPARATOR, &last));
	strcpy(request->httpver, strtok_r(NULL, CR_SEPARATOR, &last));

	strcpy(all_headers, strtok_r(NULL, "\r\n\r\n", &last));

	printf("All headers: \n%s\n", all_headers);

	for (current_header = strtok_r(all_headers, CR_SEPARATOR, &headerlast);
	     current_header;
	     current_header = strtok_r(NULL, CR_SEPARATOR, &headerlast)) {
		printf("Current header: %s\n", current_header);
	}

	strcpy(request->body, "body would go here\r\n");

	httpreq_print(stdout, request);

	return 0;
}
*/

int processHTTPRequest(int socket) {
	struct httpreq_data request;
	// Zero out the structure
	memset(&request, 0, sizeof request);

	char buffer[BUFFER_LEN];
	memset(buffer, 0, sizeof buffer);


	read(socket, buffer, BUFFER_LEN);
	parseRequestData(buffer, BUFFER_LEN, &request);
	printf("-------BEGIN-----------\n%s\n-------END------\n", buffer);
	/*printf("------BEGIN----------------\n");
	for (int i = 0; i < strlen(buffer); i++) {
		if (buffer[i] == '\r') {
			printf("\\r");
		} else if (buffer[i] == '\n') {
			printf("\\n");
		} else {
			printf("%c", buffer[i]);
		}
	}*/
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
