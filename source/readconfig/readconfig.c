/**
 * @file readconfig.c
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @brief Implementation of the functions for reading configuration files
 *
 * This file contains the implementations for the functions that allow adding new key-value pairs to
 * the configuration dictionary, as well as utility functions used inside them. It also contains the
 * main function responsible for parsing a configuration file and adding the appropiate options to the
 * dictionary.
 *
 * The dictionary is implemented as a hash table for better performance, using the uthash library.
 * @see https://troydhanson.github.io/uthash/
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <assert.h>

#include "readconfig.h"

#define PARTYPE_MAX 10 ///< Maximum size of the name of a parameter type

// Private functions
//======================================================

/**
 * @brief Converts a string to an integer
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @details This function implements the same functionality as @ref strtol(), but converts
 * the string to an @c int instead of to a @c long. If the number to be converted doesn't fit
 * in an integer, @c errno will be set appropiately to indicate the range error.
 * @param[in] s String which will be converted to @c int
 * @param[out] endptr Pointer which will be set to the first non-numerical value in the provided string
 * @param[in] base Base to use in the conversion
 * @return The integer resulting from the conversion, or the maximum/minimum value that fits
 * in an integer if the number doesn't fit in an int.
 */
int strtoi(const char *s, char **endptr, int base);

/**
 * @brief Returns a string representation of a #config_partype
 * @author Diego Ortín Fernández
 * @date 10 March 2020
 * @post The user is responsible for freeing the allocated memory
 * @param[in] type The type to represent as a string
 * @return String representation for the type
 */
char *type_to_str(config_partype type);

/**
 * @brief Infers the type of the data represented by a string
 * @details This function assumes that any string whose characters are all
 * digits represents a @ref config_partype.PARTYPE_INTEGER value. Otherwise,
 * it represents a @ref config_partype.PARTYPE_STRING value.
 * @warning The implementation of this function makes it impossible to have
 * strings with all their characters being digits.
 * @author Diego Ortín Fernández
 * @date 10 March 2020
 * @param[in] str The string whose type must be inferred
 * @return The @ref config_partype of the provided string
 */
/*config_partype str_get_type(char *str) {
	for (int i = 0; i < strlen(str); i++) {
		if (isdigit(str[i]) == 0) { /// If any character isn't a number
			return PARTYPE_STRING; /// Type must be string
		}
	}
	return PARTYPE_INTEGER; /// Else, it must be an integer
}*/

/**
 * @brief Finds the metadata for the option corresponding to the string
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @param[in] str The string whose corresponding option must be found
 * @return Pointer to the structure with the option's metadata
 */
const struct supported_param *get_matching_option(const char *str) {
    if (!str) return NULL;

    for (int i = 0; i < USERPARAMS_NUM; i++) { // Check if any options match the string
        // Return the corresponding option metadata
        if (strcmp(USERPARAMS_META[i].name, str) == 0) return &USERPARAMS_META[i];
    }

    return NULL; // If it didn't match anything, return NULL
}

/**
 * @brief Parses a configuration file line, attempting to add the parameter stored
 * in it to the configuration dictionary.
 * @details This function's purpose is to be used by parseConfig() to parse the
 * configuration file.
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @param configuration Pointer to a pointer to the configuration dictionary
 * @param str String where the line to parse is stored
 */
void parseLine(const struct config_param **configuration, const char *str) {
    char parName[MAX_LINE], parValue[MAX_LINE];

    sscanf(str, "%[^= ]=%s", parName, parValue); // Separate the line into key and value

    const struct supported_param *paramData = NULL;
    if ((paramData = get_matching_option(parName)) != NULL) { // If the option is supported
        STATUS ret;
        if (paramData->type == PARTYPE_INTEGER) { // If the parameter's type is integer
            int parValueInt = strtoi(parValue, NULL, 10); // Convert the value string to int
            ret = config_addparam_int(configuration, parName,
                                      parValueInt); // Add the parameter to the configuration
#if DEBUG >= 2
            if (ret == SUCCESS) {
                printf("Added new parameter '%s' with value '%i'\n", parName, parValueInt);
            } else {
                printf("Couldn't add new parameter '%s' with value '%i'\n", parName, parValueInt);
            }
#endif
        } else if (paramData->type == PARTYPE_STRING) { // If the parameter's type is string
            ret = config_addparam_str(configuration, parName, parValue); // Add the parameter to the configuration
#if DEBUG >= 2
            if (ret == SUCCESS) {
                printf("Added new parameter '%s' with value %s\n", parName, parValue);
            } else {
                printf("Couldn't add new parameter '%s' with value %s\n", parName, parValue);
            }
#endif
        } else {
#if DEBUG >= 2
            printf("Parameter '%s' has value '%s' of unknown type\n", parName, parValue);
#endif
        }
    } else {
#if DEBUG >= 1
        printf("Found unsupported parameter with name '%s'\n", parName);
#endif
    }
}


// Public functions
//======================================================

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-multiway-paths-covered"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"

STATUS config_add_parameter(const struct config_param **configuration, char *name, union param_value *value,
                            config_partype type) {
    if (!name || !value) return ERROR;
    struct config_param *parameter;

    HASH_FIND_STR(*configuration, name, parameter); // Attempt to find an element with this key
    if (parameter == NULL) { // If an element with this key wasn't found
        parameter = (struct config_param *) malloc(sizeof *parameter); // Create a new one
        parameter->value = *value;
        parameter->type = type;
        strcpy(parameter->name, name);
        HASH_ADD_STR(*configuration, name, parameter); // Add the key for the hashtable
        return SUCCESS;
    } else { // If an element with this key already exists
        // TODO: Print to logs
        if (DEBUG >= 2) printf("Error: key %s already exists in the options dictionary!\n", name);
        return ERROR;
    }
}

#pragma clang diagnostic pop

STATUS config_addparam_int(const struct config_param **configuration, char *name, int value) {
    if (!name || !value) return ERROR;
    union param_value val;
    val.integer = value;

    config_add_parameter(configuration, name, &val, PARTYPE_INTEGER);

    return SUCCESS;
}

STATUS config_addparam_str(const struct config_param **configuration, char *name, char *value) {
    if (!name || !value) return ERROR;
    union param_value val;
    strcpy(val.string, value);

    config_add_parameter(configuration, name, &val, PARTYPE_STRING);

    return SUCCESS;
}

int config_getparam(const struct config_param **configuration, const char *name, struct config_param **out) {
    if (!configuration || !name) return -1; // If parameters are null, return -1
    struct config_param *parameter = NULL;

    HASH_FIND_STR(*configuration, name, parameter); // NOLINT(hicpp-multiway-paths-covered,hicpp-signed-bitwise)

    if (!parameter) return -2;
    *out = parameter;
    return 0; // If everything went well, return 0
}

int config_getparam_int_n(const struct config_param **configuration, const char *name, int *out) {
    if (!name) return -1; // If arguments are null, return -1
    struct config_param *parameter; // Pointer to store the reference to the found parameter
    int ret; // Variable to store the return code from config_getparam()
    if ((ret = config_getparam(configuration, name, &parameter)) != 0) { // If config_getparam() failed
        return ret; // Return the reason for the failure
    }

    if (parameter->type != PARTYPE_INTEGER) {
        char *typestr = type_to_str(parameter->type);
        printf("Tried to get parameter %s as integer, but its type is %s\n", name, typestr);
        free(typestr);
        return -3; // If type is wrong, return -3
        // TODO: Better error printing
    } else {
        *out = parameter->value.integer;
        return 0; // If everything went well, return 0
    }
}

int config_getparam_int(const struct config_param **configuration, enum USER_PARAMS option, int *out) {
    const char *name = USERPARAMS_META[option].name;
    return config_getparam_int_n(configuration, name, out);
}

int config_getparam_str_n(const struct config_param **configuration, const char *name, char **out) {
    if (!name) return -1; // If arguments are null, return -1
    struct config_param *parameter; // Pointer to store the reference to the found parameter
    int ret; // Variable to store the return code from config_getparam()
    if ((ret = config_getparam(configuration, name, &parameter)) != 0) { // If config_getparam() failed
        return ret; // Return the reason for the failure
    }

    if (parameter->type != PARTYPE_STRING) {
        char *typestr = type_to_str(parameter->type);
        printf("Tried to get parameter %s as string, but its type is %s\n", name, typestr);
        // TODO: Better error printing
        free(typestr);
        return -3; // If type is wrong, return -3
    } else {
        *out = parameter->value.string; // Make the output point to the string
        return 0; // If everything went well, return 0
    }
}

int config_getparam_str(const struct config_param **configuration, enum USER_PARAMS option, char **out) {
    const char *name = USERPARAMS_META[option].name;
    return config_getparam_str_n(configuration, name, out);
}

int parseConfig(char *filename, const struct config_param **configuration) {
    if (!filename) return -1;

    FILE *configFile = fopen(filename, "r"); // Open the configuration file
    if (!configFile) { // If the file couldn't be opened
        printf("Error while opening the configuration file at %s: %s\n", filename, strerror(errno));
        return -1;
    }

    char current_line[MAX_LINE];
    while (fgets(current_line, MAX_LINE, configFile) != NULL) {
        parseLine(configuration, current_line);
    }

    return 0;
}

int strtoi(const char *s, char **endptr, int base) {
#if INT_MAX == LONG_MAX && INT_MIN == LONG_MIN // If long and int are identical in this system
    return (int) strtol(s, endptr, base);   // It's safe to simply cast to integer
#else /// Else, we need to check wether casting is safe
    long num = strtol(s, endptr, base); // Convert the string to a long
    if (num > INT_MAX) { // If the number is bigger than maximum integer
        errno = ERANGE; // Set the errno to range error
        return INT_MAX; // Return the maximum integer
    } else if (num < INT_MIN) { // If the number is smaller than minimum integer
        errno = ERANGE; // Set the errno to range error
        return INT_MIN; // Return the minimum integer
    } else { // If the number fits in an integer
        return (int) num; // We can safely cast it and return it
    }
#endif
}

char *type_to_str(config_partype type) {
    char *str = malloc(sizeof(char) * PARTYPE_MAX);
    switch (type) {
        case PARTYPE_INTEGER:
            strcpy(str, "integer");
            break;
        case PARTYPE_STRING:
            strcpy(str, "string");
            break;
        default:
            strcpy(str, "unknown");
            break;
    }
    return str;
}

char *readconfig_perror(int err) {
    switch (err) {
        case -1:
            return "Bad arguments";
        case -2:
            return "Not found";
        case -3:
            return "Wrong type";
        default:
            return "Unknown error";
    }
}
