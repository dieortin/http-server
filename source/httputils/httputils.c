/**
 * @file httputils.c
 * @brief Contains the implementation of the functions for processing and responding to HTTP requests
 * @author Diego Ortín Fernández & Mario Lopez
 * @date February 2020
 */

#include "httputils.h"
#include "constants.h"
#include "mimetable.h"

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>

#define CRLF_LEN strlen("\r\n") ///< Length of the string containing the response code (always three digit)

/**
 * @brief Attempts to get the content length of an HTTP request from its headers
 * @param[in] headers Structure containing the headers of the request
 * @param[in] num_headers Number of headers in the structure
 * @return The content length specified in the headers, or 0 if an error occurs or there was no length header
 */
unsigned long get_content_length(struct phr_header *headers, size_t num_headers) {
    if (!headers) return 0;

    for (int i = 0; i < num_headers; i++) {
        // Check if this header is Content-Length
        if (strncmp(headers[i].name, HDR_CONTENT_LENGTH, headers[i].name_len) == 0) {
            char *content_len_str = strndup(headers[i].value, headers[i].value_len); // Copy only the value section
            long content_length = strtol(content_len_str, NULL, 10);
            free(content_len_str);
            return content_length;
        }
    }

    return 0;
}

/**
 * @brief Returns a pointer to the querystring part of a path string
 * @param[in] path The complete path (including the querystring)
 * @param[in] path_len The length of the complete path
 * @return Pointer to the beginning of the querystring, or NULL if there's no querystring or an error occurs
 */
const char *get_querystring(const char *path, size_t path_len) {
    if (!path) return NULL;

    char *qspos = memchr(path, '?', path_len);
    if (!qspos) return NULL; // If the character wasn't found, there's no querystring

    return qspos;
}

parse_result
readParse(int socket, char *buf, size_t buf_size, int *minor_version, struct phr_header headers[], size_t *num_headers,
          size_t *method_len, size_t *path_len, const char **method, const char **path) {
    if (socket < 0 || !buf || buf_size <= 0 || num_headers < 0) {
        return PARSE_ERROR;
    }

    size_t buflen = 0, prevbuflen = 0;
    int pret;
    ssize_t rret;

    while (1) {
        while ((rret = read(socket, buf + buflen, buf_size - buflen)) == -1 && errno == EINTR);

        if (rret < 0) { // IO error
            return PARSE_IOERROR;
        }
        prevbuflen = buflen;
        buflen += rret;

        // Parse the request
        pret = phr_parse_request(buf, buflen, method, method_len, path, path_len, minor_version, headers, num_headers,
                                 prevbuflen);

        if (pret > 0) break;
        else if (pret == -1) return PARSE_ERROR;
        assert(pret == -2);
        if (buflen == sizeof(buf)) return PARSE_REQTOOLONG;
    }

    return PARSE_OK;
}

parse_result parseRequest(int socket, struct request **request) {
    struct request *newreq = calloc(1, sizeof(struct request));
    if (!newreq) return PARSE_INTERNALERR; // If allocation failed return internal error

    // Zero out the structure
    memset(newreq, 0, sizeof(struct request));

    newreq->reqbuf = calloc(MAX_HTTPREQ, sizeof(char)); // Buffer to hold the request text
    if (!newreq->reqbuf) { // If allocation failed return internal error
        free(newreq);
        return PARSE_INTERNALERR;
    }

    // picohttpparser requires the number of headers to be set to the maximum one before parsing
    newreq->num_headers = sizeof(newreq->headers) / sizeof(newreq->headers[0]);


    // Temporal variables to store the method and path positions before copying them to the new structure
    const char *tmp_method, *tmp_fullpath;
    size_t tmp_method_len, tmp_fullpath_len, path_len, qs_len;


    parse_result pret = readParse(socket, newreq->reqbuf, MAX_HTTPREQ / sizeof(char), &newreq->minor_version,
                                  newreq->headers,
                                  &newreq->num_headers, &tmp_method_len, &tmp_fullpath_len, &tmp_method, &tmp_fullpath);
    if (pret != PARSE_OK) { // If parsing failed
        freeRequest(newreq); // Free the memory associated with the request
        return pret; // Return the error code
    }

    // Obtain the location of the querystring, and calculate the length of path and querystring
    const char *tmp_querystring = get_querystring(tmp_fullpath, tmp_fullpath_len);
    if (tmp_querystring) {
        // The length of the path is the total one minus that of the querystring
        path_len = tmp_querystring - tmp_fullpath;
        // The length of the querystring is the total one minus that of the path and minus the '?' character
        qs_len = tmp_fullpath_len - path_len - 1;

        newreq->querystring = calloc(qs_len + 1, sizeof(char)); // Allocate memory in the struct for the querystring
        if (!newreq->querystring) return PARSE_INTERNALERR;
        // Copy the querystring to the structure omitting the '?' character
        strncpy((char *) newreq->querystring, tmp_querystring + 1, qs_len);
    } else {
        path_len = tmp_fullpath_len; // There's no querystring, so the length of the path is that of the full path
        newreq->querystring = NULL; // Initialize the querystring field of the structure to NULL
    }


    // Allocate memory in the structure for method and path
    newreq->method = calloc(tmp_method_len + 1, sizeof(char));
    if (!newreq->method) return PARSE_INTERNALERR;
    newreq->path = calloc(path_len + 1, sizeof(char));
    if (!newreq->path) return PARSE_INTERNALERR;

    // Copy method and path to the structure
    strncpy((char *) newreq->method, tmp_method, tmp_method_len);
    strncpy((char *) newreq->path, tmp_fullpath, path_len);

    // If the request is POST, check its body length
    if (strcmp(newreq->method, POST) == 0) {
        newreq->body_len = get_content_length(newreq->headers, newreq->num_headers);
    } else { // Else, set it to zero
        newreq->body_len = 0;
    }

    if (newreq->body_len > 0) { // If the request has a body
        int last_value_len = newreq->headers[newreq->num_headers - 1].value_len;

        // Calculate the position of the body by moving past the last header value, and skipping the two consecutive
        // CRLF line endings
        const char *body_pos = newreq->headers[newreq->num_headers - 1].value + last_value_len + (CRLF_LEN * 2);

        // Duplicate the body and put it into the structure
        newreq->body = strndup(body_pos, newreq->body_len);
    } else {
        newreq->body = NULL; // If there's no body, set it to NULL
    }

    *request = newreq;

    return PARSE_OK;
}

STATUS freeRequest(struct request *request) {
    if (!request) return ERROR;

    if (request->reqbuf) free((char *) request->reqbuf);
    if (request->path) free((char *) request->path);
    if (request->method) free((char *) request->method);
    if (request->querystring) free((char *) request->querystring);
    if (request->body) free((char *) request->body);

    free(request);

    return SUCCESS;
}

int send_response_header(int socket, unsigned int code, const char *message, struct httpres_headers *headers) {
    char *status_line = NULL;
    size_t status_line_len = 0;

    // Response header
    if (message) {
        // First, calculate the size needed for the status line buffer
        status_line_len = (size_t) snprintf(NULL, 0, "%s %i %s\r\n", HTTP_VER, code, message);
        status_line = malloc(status_line_len + 1); // Allocate memory for the status line and null-terminator
        sprintf(status_line, "%s %i %s\r\n", HTTP_VER, code, message); // Print the status line
    } else {
        // First, calculate the size needed for the status line buffer
        status_line_len = (size_t) snprintf(NULL, 0, "%s %i\r\n", HTTP_VER, code);
        status_line = malloc(status_line_len + 1); // Allocate memory for the status line and null-terminator
        sprintf(status_line, "%s %i\r\n", HTTP_VER, code); // Print the status line
    }

    // Size for the status line and headers
    size_t header_size = status_line_len + headers_getlen(headers) + CRLF_LEN;

    char *buffer = calloc(header_size + 1, sizeof(char)); // Allocate a buffer with the required memory
    if (!buffer) return -1;

    strcpy(buffer, status_line); // Copy the status line to the response buffer

    if (headers) {
        for (int i = 0; i < headers->num_headers; i++) {
            strcat(buffer + status_line_len, headers->headers[i]); // Add the header
            strcat(buffer, "\r\n"); // Add the header CRLF
        }
    }

    // Empty line before response body
    strcat(buffer, "\r\n");

#if DEBUG >= 3
    printf("Sending response header:\n%s\n", buffer);
#endif

    int ret = send(socket, buffer, header_size, 0);

    free(buffer);
    free(status_line);

    return ret;
}

int send_response_body(int socket, const char *body, unsigned long body_len) {
    if (!body || body_len <= 0) return -1;

#if DEBUG >= 3
    printf("Sending response body:\n%s\n", body);
#endif
    return send(socket, body, body_len/* + 1*/, 0);
}

HTTP_RESPONSE_CODE
respond(int socket, HTTP_RESPONSE_CODE code, const char *message, struct httpres_headers *headers, const char *body,
        unsigned long body_len) {
#if DEBUG >= 1
    short int err = 0;
    long bytes_sent = 0;
#endif

    int ret = send_response_header(socket, code, message, headers); // Send the response header
#if DEBUG >= 1
    ret == -1 ? (err = 1) : (bytes_sent += ret);
#endif

    if (body) {
        ret = send_response_body(socket, body, body_len);
#if DEBUG >= 1
        ret == -1 ? (err = 1) : (bytes_sent += ret);
#endif
    }

#if DEBUG >= 1
    if (err) {
        perror("Error while sending response");
    } else {
        printf("Sent %li bytes\n", bytes_sent);
    }
#endif
    shutdown(socket, SHUT_WR);
    shutdown(socket, SHUT_RD);
    close(socket);

    return code;
}

STATUS setDefaultHeaders(struct httpres_headers *headers) {
    if (!headers) return ERROR;

    struct tm tm;

    char timeStr[100];
    time_t now = time(0);
    gmtime_r(&now, &tm);
    strftime(timeStr, sizeof timeStr, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    //create date header
    set_header(headers, HDR_DATE, timeStr);
    //create server header
    set_header(headers, HDR_SERVER_ORIGIN, "httpServer"); // FIXME: USE SERVER SIGNATURE FROM CONFIG FILE
    return SUCCESS;
}

// https://stackoverflow.com/a/4553076/3024970
int is_regular_file(const char *path) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(struct stat));
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode); // NOLINT(hicpp-signed-bitwise)
}

int is_directory(const char *path) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(struct stat));
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode); // NOLINT(hicpp-signed-bitwise)
}

/**
 * @brief Returns the size of the provided file
 * @param[in] fd File descriptor whose size must be found out
 * @return Size of the file, or -1 if an error occurs
 */
off_t get_file_size(FILE *fd) {
    if (!fd) return -1;

    struct stat s;
    memset(&s, 0, sizeof(struct stat)); // Zero out the structure

    if (fstat(fileno(fd), &s) == -1) { // Obtain the stats for the file
        return -1; // Error while executing stat
    }

    return s.st_size; // Return the file size
}

/**
 * @brief Spawns a child process executing the provided command, and routes its stdin and stdout to pipes
 * @details This function is similar to popen(), but supports bidirectional communication with the spawned
 * process. It creates pipes in both directions, so that the caller can later write to the spawned process'
 * stdin, and read from its stdout. Heavily inspired in code by <psimerda@redhat.com>
 * @author Diego Ortín Fernández
 * @see https://github.com/crossdistro/netresolve/blob/master/backends/exec.c#L46
 * @param[in]  command The argv of the command to be executed
 * @param[out] pid Variable to store the process ID of the spawned process
 * @param[out] infd Variable to store the descriptor number for writing to the process' stdin
 * @param[out] outfd Variable to store the descriptor number for reading from the process' stdout
 * @return 0 if something goes wrong, 1 otherwise
 */
int popen2(char *const command[], int *pid, int *infd, int *outfd) {
    int pipe_in[2], pipe_out[2];

    if (!pid || !infd || !outfd) return 0;

    if (pipe(pipe_in) == -1) goto pipe1_error;
    if (pipe(pipe_out) == -1) goto pipe2_error;
    if ((*pid = fork()) == -1) goto fork_error;

    if (*pid) { // If we're inside the parent process
        *infd = pipe_in[1]; // Set the input descriptor to the write end of pipe_in
        *outfd = pipe_out[0]; // Set the output descriptor to the read end of pipe_out

        close(pipe_in[0]); // Close the read end of pipe_in, as we're not using it
        close(pipe_out[1]); // Close the write end of pipe_out, as we're not using it

        return 1;
    } else { // If we're inside the child process
        dup2(pipe_in[0], STDIN_FILENO); // Replace the process' stdin by the read end of pipe_in
        dup2(pipe_out[1], STDOUT_FILENO); // Replace the process' stdout by the write end of pipe_in

        // Close all the pipes
        close(pipe_in[0]);
        close(pipe_in[1]);
        close(pipe_out[0]);
        close(pipe_out[1]);

        execvp(*command, command); // Execute the provided command

        // If an error occurs
        fprintf(stderr, "Error executing %s: %s", *command, strerror(errno));
        abort();
    }

    fork_error:
    close(pipe_out[1]);
    close(pipe_out[0]);
    pipe2_error:
    close(pipe_in[1]);
    close(pipe_in[0]);
    pipe1_error:
    return 0;
}

int run_executable(int socket, struct httpres_headers *headers, struct request *request, struct _srvutils *utils,
                   const char *exec_cmd, const char *fullpath) {
    if (!fullpath || !exec_cmd || !utils || !request || !headers) return 0;

    //char *command = NULL;

    const char *command[3];
    command[0] = exec_cmd;
    command[1] = fullpath;
    command[2] = NULL;

    // Calculate required size for the command string and its null terminator
    //size_t command_size = snprintf(NULL, 0, "%s %s", exec_cmd, path) + 1;
    //command = malloc(sizeof(char) * command_size); // Allocate space for the command string
    //snprintf(command, command_size, "%s %s", exec_cmd, path); // Print the command string


    pid_t pid;
    int infd, outfd;

    if (popen2((char *const *) command, &pid, &infd, &outfd) == 0) return 0;
    //free(command);
    //if (!fd) return 0;

    // If the request had a querystring, write it to the script's stdin
    if (request->querystring) {
        write(infd, request->querystring, strlen(request->querystring));
        write(infd, "\r\n", 2);
    }

    // If the request has a body, write it to the script's stdin
    if (request->body) {
        write(infd, request->body, request->body_len);
        write(infd, "\r\n", 2);
    }

    close(infd);

    char result[MAX_BUFFER + 1]; // Buffer to hold the result of the execution

    unsigned long n_read = read(outfd, result, sizeof(result) / sizeof(result[0])); // Read from the pipe

    if (n_read > 0) { // If reading from the pipe goes well
#if DEBUG >= 2
        utils->log(stdout, "Command output: \n%s", result);
#endif
        return respond(socket, OK, "OK", headers, result, n_read);
    } else {
        return respond(socket, INTERNAL_ERROR, "Execution error", headers, NULL, 0);
    }
}

HTTP_RESPONSE_CODE send_file(int socket, struct httpres_headers *headers, const char *path) {
    if (!headers || !path) {
        return respond(socket, INTERNAL_ERROR, "Internal error", NULL, NULL, 0);
    }

    if (!is_regular_file(path)) { // If it's not a regular file (i.e. is a directory, pipe, link...)
        return respond(socket, NOT_FOUND, "Not found", headers, NULL, 0);
    }

    FILE *f = fopen(path, "r");
    if (f) {
        long length = get_file_size(f);

        // Copy the entire file to the buffer
        char *buffer = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fileno(f), 0);
        if (buffer == MAP_FAILED) {
            return respond(socket, INTERNAL_ERROR, "Internal error", headers, NULL, 0);
        }

        // Add the file headers
        add_last_modified(path, headers);
        add_content_type(path, headers);
        add_content_length(length, headers);

        respond(socket, OK, "OK", headers, buffer, length);

        munmap(buffer, length); // Free the mapping

        fclose(f);
        return OK;
    } else {
        if (errno == ENOENT) {
            return respond(socket, NOT_FOUND, "Not found", NULL, NULL, 0);
        } else {
            return respond(socket, INTERNAL_ERROR, "Not found", NULL, NULL, 0);
        }
    }
}

STATUS add_content_type(const char *filePath, struct httpres_headers *headers) {
    const char *content_name = NULL;
    content_name = get_mime_type(filePath);

    if (!content_name) return ERROR;

    return set_header(headers, HDR_CONTENT_TYPE, content_name);
}

STATUS add_last_modified(const char *filePath, struct httpres_headers *headers) {
    char t[100] = "";
    struct stat b;
    memset(&b, 0, sizeof(struct stat));

    stat(filePath, &b);

    struct tm tm;
    strftime(t, sizeof t, "%a, %d %b %Y %H:%M:%S %Z", localtime_r(&b.st_mtime, &tm));
    return set_header(headers, HDR_LAST_MODIFIED, t);
}


STATUS add_content_length(long length, struct httpres_headers *headers) {
    char len_str[10];
    sprintf(len_str, "%li", length);
    return set_header(headers, HDR_CONTENT_LENGTH, len_str);
}

const char *get_mime_type(const char *name) {
    if (!name) return NULL;

    char *ext = strrchr(name, '.');

    if (!ext) return NULL;
    ext++; // skip the '.';

    return mime_get_association(ext);
}

struct httpres_headers *create_header_struct() {
    struct httpres_headers *new = malloc(sizeof(struct httpres_headers));
    if (!new) return NULL; // Check for malloc error
    new->headers = NULL;
    new->num_headers = 0;
    return new;
}

STATUS set_header(struct httpres_headers *headers, const char *name, const char *value) {
    if (!headers || !name || !value) return ERROR;

    if (headers->num_headers == 0) { // If the array didn't exist, create it
        headers->headers = calloc(1, sizeof(char *));
    } else {
        headers->headers = realloc(headers->headers, (headers->num_headers + 1) * sizeof(char *));
    }

    // Calculate the needed size for allocating the header string plus the null terminator
    size_t needed_size = snprintf(NULL, 0, "%s: %s", name, value) + 1;

    // Allocate space for the string
    headers->headers[headers->num_headers] = malloc(sizeof(char) * needed_size);

    // Produce the header string
    snprintf(headers->headers[headers->num_headers], needed_size, "%s: %s", name, value);

    headers->num_headers++;
    return SUCCESS;
}

void headers_free(struct httpres_headers *headers) {
    if (!headers) return;

    if (headers->headers) {
        for (int i = 0; i < headers->num_headers; i++) {
            if (headers->headers[i]) free(headers->headers[i]);
        }
        free(headers->headers);
    }

    free(headers);
}

int headers_getlen(struct httpres_headers *headers) {
    if (!headers) return 0;
    int counter = 0;
    for (int i = 0; i < headers->num_headers; i++) {
        counter += (int) strlen(headers->headers[i]);
        counter += CRLF_LEN; // also add the size of the CRLF header line terminator
    }

    return counter;
}