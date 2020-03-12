/**
 * @file constants.h
 * @author Diego Ortín Fernández
 * @date 5 March 2020
 * @brief Contains various constant values and enums that are useful for the entire project
 */

#ifndef PRACTICA1_CONSTANTS_H

#define DEBUG 2 ///< Sets the level of debugging logs. A value of 0 disables them.

#define CONFIG_PATH "/server/server.cfg"    ///< Path of the folder where the configuration file is located
///< relative to the main server directory

#define MAX_CONFIG_STR 512 ///< Maximum size of a configuration parameter string

#define MAX_BUFFER 1024
#define MAX_LINE 100

/**
 * Represents outcomes of the execution of a function
 */
typedef enum _STATUS {
	SUCCESS = 1, ///< The function executed without errors
	ERROR = 0 ///< The function encountered at least one error
} STATUS;

#define PRACTICA1_CONSTANTS_H
#endif //PRACTICA1_CONSTANTS_H
