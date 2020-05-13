/**
 * @file readconfig.h
 * @brief Module used for reading configuration files, storing the user-defined parameters in a dictionary and
 * adding/accessing parameters in it.
 * @author Diego Ortín Fernández
 * @date 5 March 2020
 */

#ifndef PRACTICA1_READCONFIG_H

#include "constants.h"
#include "uthash.h"

#define MAX_PARAM_NAME 30 ///< Biggest possible size for a configuration parameter name

/**
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @brief Each member of the enumeration represents one type of data of which a
 * parameter can be
 */
typedef enum config_partype {
    PARTYPE_INTEGER, ///< Type @c int
    PARTYPE_STRING ///< Type @c char*
} config_partype;

/**
 * @struct supported_param
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @brief Contains the metadata of a supported user-defined parameter
 */
struct supported_param {
    char name[MAX_PARAM_NAME]; ///< The string that identifies the parameter in the configuration file
    config_partype type; ///< The expected type for the parameter
};

// Supported parameters:
//====================================
/**
 * @enum USER_PARAMS
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @brief Contains the user-defined parameters that are supported by the Server
 */
enum USER_PARAMS {
    PARAMS_ADDRESS,
    PARAMS_PORT,
    PARAMS_WEBROOT,
    PARAMS_NTHREADS,
    PARAMS_QUEUE_SIZE,
    PARAMS_MIME_FILE
};

/**
 * @var USERPARAMS_META
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @brief Contains @ref supported_param structures, in the same order as the user-defined parameters specified in
 * @ref USER_PARAMS. Each structure represents the parameter in its same position.
 */
static const struct supported_param USERPARAMS_META[] = {
        {"ADDRESS",    PARTYPE_STRING},
        {"PORT",       PARTYPE_INTEGER},
        {"WEBROOT",    PARTYPE_STRING},
        {"NTHREADS",   PARTYPE_INTEGER},
        {"QUEUE_SIZE", PARTYPE_INTEGER},
        {"MIME_FILE",  PARTYPE_STRING}
};

#define USERPARAMS_NUM (sizeof(USERPARAMS_META) / sizeof(USERPARAMS_META[0])) ///< Number of supported parameters
//=====================================

/**
 * @union param_value
 * @brief This union contains the value of a parameter.
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @details Type information is required to know which member of
 * the union should be accessed.
 * @see config_param
 * @see config_partype
 */
union param_value {
    int integer; ///< Member which stores the data when the type is integer
    char string[MAX_CONFIG_STR]; ///< Member which stores the data when the type is string
};

/**
 * @struct config_param
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @brief This structure stores a key-value pair of the dictionary, together with its type information.
 */
struct config_param {
    char name[MAX_PARAM_NAME]; ///< Name of the parameter
    union param_value value; ///< Value of the parameter
    config_partype type; ///< Type of the parameter
    struct UT_hash_handle hh; ///< Handle to be used by @a uthash
};

//config_partype str_get_type(char *str);

/**
 * @brief Adds a new parameter to the dictionary with the given key and value
 * @author Diego Ortín Fernández
 * @date  11 March 2020
 * @note This is the low-level function for adding key-value pairs to the configuration dictionary.
 * If possible, use the higher level config_addparam_int() or config_addparam_str() versions.
 * @details A string with the name of the parameter serves as the key, a @ref param_value union serves
 * as the value, and the type argument specifies the type of the data contained inside the value parameter.
 * @see config_addparam_int
 * @see config_addparam_str
 * @pre @p value must contain data of the type specified by @p type
 * @param[in,out] configuration The configuration dictionary in which the parameter should be added
 * @param[in] name The name of the parameter
 * @param[in] value The value of the parameter
 * @param[in] type The type of the parameter
 * @return \ref STATUS.SUCCESS if the parameter could be added, \ref STATUS.ERROR if a parameter with
 * the same name already existed or if any error is encountered
 */
STATUS config_add_parameter(const struct config_param **configuration, char *name, union param_value *value,
                            config_partype type);

/**
 * @brief Adds a new @c int type parameter to the configuration dictionary
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @param[in,out] configuration The configuration dictionary in which the parameter should be added
 * @param[in] name The name of the parameter
 * @param[in] value The value of the parameter
 * @return @ref STATUS.SUCCESS if the parameter could be added, @ref STATUS.ERROR if a parameter with
 * the same name already existed or if any error is encountered
 */
STATUS config_addparam_int(const struct config_param **configuration, char *name, int value);

/**
 * @brief Adds a new @c char* type parameter to the configuration dictionary
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @param[in,out] configuration The configuration dictionary in which the parameter should be added
 * @param[in] name The name of the parameter
 * @param[in] value The value of the parameter
 * @return @ref STATUS.SUCCESS if the parameter could be added, @ref STATUS.ERROR if a parameter with
 * the same name already existed or if any error is encountered
 */
STATUS config_addparam_str(const struct config_param **configuration, char *name, char *value);

/**
 * @brief Obtains the value of the @c int type parameter associated with the name @p name
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @param[in] configuration The configuration dictionary in which the parameter should be searched for
 * @param[in] name The name of the parameter
 * @param[out] out Pointer to the @c int variable where the value must be stored
 * @return 0 if no errors ocurred, -1 if any parameters were wrong, -2 if no key-value pair was found
 * with that name, and -3 if the type of the key-value pair isn't @ref PARTYPE_INTEGER
 */
int config_getparam_int_n(const struct config_param **configuration, const char *name, int *out);

/**
 * @brief Obtains the value of the @c int type parameter associated with the supported option @p option
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @param[in] configuration The configuration dictionary in which the parameter should be searched for
 * @param[in] option The supported user parameter whose value should be retrieved
 * @param[out] out Pointer to the @c char* variable where the value must be stored
 * @return 0 if no errors ocurred, -1 if any parameters were wrong, -2 if no key-value pair was found
 * with that name, and -3 if the type of the key-value pair isn't @ref PARTYPE_INTEGER
 */
int config_getparam_int(const struct config_param **configuration, enum USER_PARAMS option, int *out);

/**
 * @brief Obtains the value of the @c char* type parameter associated with the name @p name
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @param[in] configuration The configuration dictionary in which the parameter should be searched for
 * @param[in] name The name of the parameter
 * @param[out] out Pointer to the @c char* variable where the value must be stored
 * @return 0 if no errors ocurred, -1 if any parameters were wrong, -2 if no key-value pair was found
 * with that name, and -3 if the type of the key-value pair isn't @ref PARTYPE_STRING
 */
int config_getparam_str_n(const struct config_param **configuration, const char *name, char **out);

/**
 * @brief Obtains the value of the @c char* type parameter associated with the supported option @p option
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @param[in] configuration The configuration dictionary in which the parameter should be searched for
 * @param[in] option The supported user parameter whose value should be retrieved
 * @param[out] out Pointer to the @c char* variable where the value must be stored
 * @return 0 if no errors ocurred, -1 if any parameters were wrong, -2 if no key-value pair was found
 * with that name, and -3 if the type of the key-value pair isn't @ref PARTYPE_STRING
 */
int config_getparam_str(const struct config_param **configuration, enum USER_PARAMS option, char **out);

/**
 * @brief Obtains the value of the parameter associated with the name @p name
 * @author Diego Ortín Fernández
 * @date 11 March 2020
 * @param[in] configuration The configuration dictionary in which the parameter should be searched for
 * @param[in] name The name associated with the key-value pair to be retrieved
 * @param[out] out Pointer that will be set to point to the found parameter
 * @return 0 if the operation executed successfully, -1 if any parameters were wrong, -2 if no key-value pair
 * was found for that name
 */
int config_getparam(const struct config_param **configuration, const char *name, struct config_param **out);

/**
 * @brief Parses the configuration file with name @p filename in path @ref CONFIG_PATH of the server directory,
 * and attempts to add its parameters to the provided configuration dictionary.
 * @param[in] filename The name of the configuration file
 * @param[out] configuration The dictionary in which the configuration options must be added
 * @return -1 if any error occurs, 0 otherwise
 */
int parseConfig(char *filename, const struct config_param **configuration);

/**
 * @brief Returns a string literal describing the readconfig error associated
 * with the error code provided
 * @author Diego Ortín Fernández
 * @date 12 March 2020
 * @param[in] err The error code
 * @return String literal describing the error
 */
char *readconfig_perror(int err);

#define PRACTICA1_READCONFIG_H
#endif //PRACTICA1_READCONFIG_H
