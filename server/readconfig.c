//
// Created by diego on 5/3/20.
//

#include "readconfig.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <zconf.h>
#include <assert.h>


#define PARAM_PORT "PORT"
#define PARAM_WEBROOT "WEBROOT"
#define PARAM_NTHREADS "NTHREADS"

int readConfig(char *filename, struct configuration *env) {
    if (!filename) return -1;

	char currentdir[MAX_BUFFER];
	getcwd(currentdir, MAX_BUFFER);
	assert(strlen(currentdir) + strlen(CONFIG_PATH) + strlen(filename) < MAX_BUFFER); // Ensure we don't overflow

    FILE *configFile = fopen(strcat(currentdir, filename), "r");
    if (!configFile) {
        printf("Error while opening the configuration file at %s: %s\n", filename, strerror(errno));
        exit(EXIT_FAILURE);
    }


    char current_line[MAX_LINE];

    while (fgets(current_line, MAX_LINE, configFile) != NULL) {
        char parName[MAX_LINE], parValue[MAX_LINE];

        sscanf(current_line, "%[^= ]=%s", parName, parValue);
        if (DEBUG) {
            printf("Read parameter with name '%s' and value '%s'\n", parName, parValue);
            if (strcmp(parName, PARAM_PORT) == 0) {
                long port = strtol(parValue, NULL, 10);
                if (port > INT_MAX) { // Check if the port value can be cast to integer safely
                    printf("Incorrect port value: doesn't fit in an integer\nTerminating.\n");
                    exit(EXIT_FAILURE);
                }
                env->port = (unsigned int) port;
            } else if (strcmp(parName, PARAM_WEBROOT) == 0) {
                if ((strlen(parValue) + 1) > MAX_BUFFER) {
                    printf("Parameter %s is too long.\nTerminating.\n", parName);
                    exit(EXIT_FAILURE);
                }
                strcpy(env->webroot, parValue);
            } else if (strcmp(parName, PARAM_NTHREADS) == 0) {
                long nthreads = strtol(parValue, NULL, 10);
                if (nthreads > INT_MAX) { // Check if the port value can be cast to integer safely
                    printf("Incorrect nthreads value: doesn't fit in an integer\nTerminating.\n");
                    exit(EXIT_FAILURE);
                }
                env->nthreads = (unsigned int) nthreads;
            } else {
                printf("Unrecognized parameter: '%s'\n", parName);
            }
        }
    }

    return EXIT_SUCCESS;
}
