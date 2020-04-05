/**
 * @file mimetable.h
 * @author Diego Ortín Fernández
 * @date 5 April 2020
 * @brief Dictionary containing associations between a file extension and its corresponding MIME type.
 * @details This module's aim is to provide a way to retrieve the MIME type associated with certain file
 * extension, in a performant way. It can parse a file containing tab separated pairs, and add it to the
 * MIME table. Then, the associated type of any extension can be retrieved with another function.
 */

#ifndef PRACTICA1_MIMETABLE_H
#define PRACTICA1_MIMETABLE_H

#include "constants.h"

/**
 * @brief Parses a file containing extension-MIMEtype associations, and adds them to the MIME table
 * @author Diego Ortín Fernández
 * @date 5 April 2020
 * @pre File located in @p path must contain one association per line, with the extension first, separated from the
 * MIME type by a tab character: <extension>\\t<mimetype>\\n
 * @param path[in] The path of the file to parse
 * @return @ref STATUS.SUCCESS if at least one association was added successfully, @ref STATUS.ERROR if any error
 * ocurred or if no associations were added successfully.
 */
STATUS mime_add_from_file(const char *path);

/**
 * @brief Retrieves the MIME type associated with the provided extension
 * @author Diego Ortín Fernández
 * @date 4 April 2020
 * @pre The association for the @p extension must exist in the table
 * @param extension[in] The extension whose corresponding MIME type must be retrieved
 * @return pointer to the string containing the MIME type
 */
const char *mime_get_association(const char *extension);

#endif //PRACTICA1_MIMETABLE_H
