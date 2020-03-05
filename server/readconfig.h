//
// Created by diego on 5/3/20.
//

#ifndef PRACTICA1_READCONFIG_H
#define PRACTICA1_READCONFIG_H

#include "constants.h"

struct configuration {
    int port;
    char webroot[MAX_CONFIG_STR];
    int nthreads;
};

int readConfig(char *filename, struct configuration *env);


#endif //PRACTICA1_READCONFIG_H