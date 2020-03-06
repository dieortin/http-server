//
// Created by diego on 5/3/20.
//

#ifndef PRACTICA1_READCONFIG_H
#define PRACTICA1_READCONFIG_H

#include "constants.h"

struct configuration {
    unsigned int port;
    char webroot[MAX_CONFIG_STR];
    unsigned int nthreads;
};

int readConfig(char *filename, struct configuration *env);


#endif //PRACTICA1_READCONFIG_H
