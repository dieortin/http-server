/**
 * @file constants.h
 * Contains various constant values and enums that are useful for the entire project
 */

#ifndef PRACTICA1_CONSTANTS_H
#define PRACTICA1_CONSTANTS_H

#define DEBUG 1

#define CONFIG_PATH "/server/server.cfg"

#define MAX_CONFIG_STR 512

#define MAX_BUFFER 1024
#define MAX_LINE 100

/**
 * Represents outcomes of the execution of a function
 */
typedef enum _STATUS {
	SUCCESS = 1, ///< The function executed without errors
	ERROR = 0 ///< The function encountered at least one error
} STATUS;

#endif //PRACTICA1_CONSTANTS_H
