//
// Created by diego on 4/4/20.
//

#ifndef PRACTICA1_MIMETABLE_H
#define PRACTICA1_MIMETABLE_H

#include "constants.h"

STATUS mime_add_association(char *name, char *value);

const char *mime_get_association(const char *extension);

#endif //PRACTICA1_MIMETABLE_H
