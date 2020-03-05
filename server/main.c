#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <limits.h>
#include <assert.h>

#include <errno.h>
#include <zconf.h>

#include "httputils.h"

#define PORT 8081
#define DEBUG 1

#define PARAM_PORT "PORT"
#define PARAM_WEBROOT "WEBROOT"
#define PARAM_NTHREADS "NTHREADS"

#define MAX_BUFFER 1024
#define MAX_LINE 100

#define CONFIG_PATH "/server/server.cfg"

struct configuration {
	int port;
	char webroot[MAX_BUFFER];
	int nthreads;
};

struct configuration env;

int readConfig(char *filename);

int main() {
	printf("INFO: Server started\n");

	char currentdir[MAX_BUFFER];
	getcwd(currentdir, MAX_BUFFER);

	// FIXME: Does this overflow when currentdir + CONFIG_PATH > MAX_BUFFER?
	readConfig(strcat(currentdir, CONFIG_PATH));

	int socket_descriptor, new_socket, valread;
	int opt = 1;

	struct sockaddr_in address;
	int addrlen = sizeof(address);

	// Create socket
	if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	if ((setsockopt(socket_descriptor, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof opt)) == 0) {
		perror("Options failed");
		exit(EXIT_FAILURE);
	}

	memset(&address, 0, sizeof address);
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);


	if (bind(socket_descriptor, (struct sockaddr *) &address, sizeof(address)) < 0) {
		perror("Bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_descriptor, 3) < 0) {
		perror("Listen failed");
		exit(EXIT_FAILURE);
	}

	printf("INFO: Server listening on port %i\n", PORT);

	while(1) {
		if ((new_socket = accept(socket_descriptor, (struct sockaddr *) &address, (socklen_t * ) & addrlen)) == 0) {
			perror(("Accept failed"));
			exit(EXIT_FAILURE);
		} else {
			processHTTPRequest(new_socket);
		}
	}



}

int readConfig(char *filename) {
	if (!filename) return -1;

	FILE *configFile = fopen(filename, "r");
	if (!configFile) {
		printf("Error while opening the configuration file at %s: %s\n", filename, strerror(errno));
		exit(EXIT_FAILURE);
	}


	char current_line[MAX_LINE];

	while(fgets(current_line, MAX_LINE, configFile) != NULL) {
		char parName[MAX_LINE], parValue[MAX_LINE];

		sscanf(current_line, "%[^= ]=%s", parName, parValue);
		if (DEBUG) {
			printf("Read parameter with name '%s' and value '%s'\n", parName, parValue);
			if (strcmp(parName, PARAM_PORT) == 0) {
				long port = strtol(parValue, NULL,  10);
				if(port > INT_MAX) { // Check if the port value can be cast to integer safely
					printf("Incorrect port value: doesn't fit in an integer\nTerminating.\n");
					exit(EXIT_FAILURE);
				}
				env.port = (int)port;
			} else if (strcmp(parName, PARAM_WEBROOT) == 0) {
				if((strlen(parValue) + 1) > MAX_BUFFER) {
					printf("Parameter %s is too long.\nTerminating.\n", parName);
					exit(EXIT_FAILURE);
				}
				strcpy(env.webroot, parValue);
			} else if (strcmp(parName, PARAM_NTHREADS) == 0) {
				if((strlen(parValue) + 1) > MAX_BUFFER) {
					printf("Parameter %s is too long.\nTerminating.\n", parName);
					exit(EXIT_FAILURE);
				}
				strcpy(env.nthreads, parValue);
			} else {
				printf("Unrecognized parameter: '%s'\n", parName);
			}
		}
	}

	return EXIT_SUCCESS;
}

