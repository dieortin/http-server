/**
 * @file constants.h
 * @author Diego Ortín Fernández
 * @date 5 March 2020
 * @brief Contains various constant values and enums that are useful for the entire project
 */

#ifndef PRACTICA1_CONSTANTS_H

#define DEBUG 0 ///< Sets the level of debugging logs. A value of 0 disables them.

#define CONFIG_PATH "../../" ///< Path of the root folder of the project where the configuration files reside
///< relative to the "bin" directory where the project is built by default

#define MAX_CONFIG_STR 512 ///< Maximum size of a configuration parameter string

#define MAX_BUFFER 1024 ///< Default size for temporary buffers
#define MAX_LINE 100 ///< Maximum length of a line in the configuration file

/**
 * Represents outcomes of the execution of a function
 */
typedef enum _STATUS {
    SUCCESS = 1, ///< The function executed without errors
    ERROR = 0 ///< The function encountered at least one error
} STATUS;

#define PRACTICA1_CONSTANTS_H
#endif //PRACTICA1_CONSTANTS_H
