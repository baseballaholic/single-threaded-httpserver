#include <stdio.h>
#include "bind.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "httpserver.h"

#define OK              "200 OK\r\n"
#define CREATED         "201 Created\r\n"
#define BAD_REQUEST     "400 Bad Request\r\n"
#define FORBIDDEN       "403 Forbidden\r\n"
#define FILE_NOT_FOUND  "404 Not Found\r\n"
#define INTERNAL_ERROR  "500 Internal Server Error\r\n"
#define NOT_IMPLEMENTED "501 Not Implemented\r\n"
#define PUTOK           "OK\n"
#define PUTCREATED      "Created\n"

void sendheader(int fd, char *response, char *status, int length, char *content) {
    sprintf(response, "HTTP/1.1 %sContent-Length: %d\r\n\r\n", status, length);
    //If there is a content body, add it to the response
    if (content != NULL) {
        strcat(response, content);
    }
    send(fd, response, strlen(response), 0);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
            "httpserver: wrong arguments: ./httpserver port_num\nusage: ./httpserver <port>\n");
        exit(EXIT_FAILURE);
    }
    uint16_t port = atoi(argv[argc - 1]);
    if (port < 1024) {
        fprintf(stderr, "httpserver: bind: Permission denied\n");
        exit(EXIT_FAILURE);
    }
    int socket = create_listen_socket(port);
    if (socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    while (1) {
        char response[BUFFER_SIZE];
        char content[BUFFER_SIZE];
        char buffer[BUFFER_SIZE];
        char header[BUFFER_SIZE];
        char headercopy[BUFFER_SIZE];
        char request[25];
        char httpversion[25];
        char filename[25];
        char *end_of_header;
        struct stat st;
        int contentlength;
        bool valid = true;
        int errors = 0; //Will become 1 if an error occurs

        //Start reading HTTP Request

        int fd = accept(socket, NULL, NULL);

        //Read the request into a buffer, and then we check if there was an error while doing so.
        //If there is an error, return.
        while (recv(fd, buffer, sizeof(char), 0) > 0) {
            if (fd < 0) {
                fprintf(stderr, "hi!\n");
                if (errno == EACCES) {
                    //If we were unable to open the file, send error code 403
                    errors = 1;
                    sendheader(fd, response, FORBIDDEN, 0, NULL);
                } else if (errno == ENOENT) {
                    //If we can't find the file, send the error 404
                    errors = 1;
                    sendheader(fd, response, FILE_NOT_FOUND, 0, NULL);
                } else {
                    //Some other error occurred, send the error 500
                    errors = 1;
                    sendheader(fd, response, INTERNAL_ERROR, 0, NULL);
                }
            } else if (errors == 0) {
                strcat(header, buffer);
                end_of_header = strstr(header, "\r\n\r\n");
                if (end_of_header != NULL) {
                    break;
                }
                memset(&buffer, 0, sizeof(buffer));
            }
        }
        //We need to check that the request was properly recieved
        //if(req < 0){
        //If we were not allowed to open the file
        //if(errno == EACCES){
        //errors = 1;
        //sendheader(fd, response, FORBIDDEN, 0, content);
        //}
        //If we couldn't locate the file
        //else if(errno == ENOENT){
        //errors = 1;
        //sendheader(fd, response, FILE_NOT_FOUND, 0, content);
        //}
        //A different error occurred
        //else{
        //errors = 1;
        //sendheader(fd, response, INTERNAL_ERROR, 0, content);
        //}
        //}

        //Split up the request and store the information
        if (strlen(header) > 0) {
            end_of_header = strstr(header, "\r\n");
            char tempend = *end_of_header;
            header[end_of_header - header] = '\0';
            sscanf(header, "%s %s %s", request, filename, httpversion);
            header[end_of_header - header] = tempend;

            //Now we need to check if the filename is valid
            valid = true;
            if (filename[0] != '/' || strlen(filename) == 1) {
                errors = 1;
                valid = false;
            }
            //We iterate through every character of the filename, valid will become false if a character is not valid.
            else if ((valid == true) && (errors == 0)) {
                char *tempname = filename;
                tempname++;
                for (uint8_t i = 0; i < strlen(tempname); i++) {
                    if (isalpha(tempname[i]) || isdigit(tempname[i]) || tempname[i] == '.'
                        || tempname[i] == '_') {
                        continue;
                    }
                    valid = false;
                }
            }
        } else {
            errors = 1;
        }
        //If it was not a valid filename, send the error
        if ((valid == false) && (errors == 0)) {
            errors = 1;
            sendheader(fd, response, BAD_REQUEST, 0, NULL);
        }

        //Check for correct version
        char *ver = strstr(header, "HTTP/1.1");

        if ((ver == NULL) && (errors == 0)) {
            errors = 1;
            sendheader(fd, response, BAD_REQUEST, 0, NULL);
        }

        bool reqtype = false;

        //We check if the request is one of the three valid request types, and if it is we set a flag to true
        if (((strcmp(request, "GET") == 0) || (strcmp(request, "HEAD") == 0)
                || (strcmp(request, "PUT") == 0))
            && (errors == 0)) {
            reqtype = true;
        }

        //Req will be false if the request type is not valid
        if (reqtype == false && (errors == 0)) {
            //Send an error that the request type is not vlaid
            errors = 1;
            sendheader(fd, response, NOT_IMPLEMENTED, 0, NULL);
        }

        memset(&buffer, 0, sizeof(buffer));
        memset(&content, 0, sizeof(content));
        if (((strcmp(request, "HEAD") == 0) || (strcmp(request, "GET") == 0)) && (errors == 0)) {
            char *tempname = filename;
            tempname++;
            int getfd = open(tempname, O_RDONLY);
            if (getfd < 0) {
                if (errno == EACCES) {
                    //If we were unable to open the file, send error code 403
                    errors = 1;
                    sendheader(fd, response, FORBIDDEN, 0, NULL);
                } else if (errno == ENOENT) {
                    //If we can't find the file, send the error 404
                    errors = 1;
                    sendheader(fd, response, FILE_NOT_FOUND, 0, NULL);
                } else {
                    //Some other error occurred, send the error 500
                    errors = 1;
                    sendheader(fd, response, INTERNAL_ERROR, 0, NULL);
                }
            } else {
                //Grab the size of the file
                stat(tempname, &st);
                int size = 0;
                size = st.st_size;
                bool dir = false;
                dir = S_ISDIR(st.st_mode);
                if (dir == true) {
                    errors = 1;
                    sendheader(fd, response, FORBIDDEN, 0, NULL);
                }
                //Check if we are sending a HEAD or GET request specifically
                if ((strcmp(request, "HEAD") == 0) && (errors == 0)) {
                    //Send an OK code without the file contents
                    sendheader(fd, response, OK, size, NULL);
                    close(getfd);
                }

                //If it's not a HEAD request then it's a GET request
                else if ((strcmp(request, "GET") == 0) && (errors == 0)) {
                    //Grab the contents of the file
                    while (read(getfd, buffer, sizeof(char))) {
                        strcat(content, buffer);
                        memset(&buffer, 0, sizeof(buffer));
                    }
                    close(getfd);

                    //Send an OK code with the file content
                    sendheader(fd, response, OK, size, content);
                }
            }
        }
        //Handling PUT request
        memset(&buffer, 0, sizeof(buffer));
        memset(&content, 0, sizeof(content));
        char *code = OK;
        int putfd = 0;
        if ((strcmp(request, "PUT") == 0) && (errors == 0)) {
            //get Content-Length
            char *ptrlen = strstr(header, "Content-Length:");
            if (ptrlen != NULL) {
                //Open the filename with read and write permissions for user and group
                char *tempname = filename;
                tempname++;
                putfd = open(tempname, O_TRUNC | O_WRONLY, 0664);
                if (putfd < 0) {
                    //If we were not allowed to open the file
                    if (errno == EACCES) {
                        errors = 1;
                        code = FORBIDDEN;
                    }
                    //If we couldn't locate the file
                    else if (errno == ENOENT) {
                        errors = 1;
                        code = FILE_NOT_FOUND;
                    }
                    //A different error occurred
                    else {
                        errors = 1;
                        code = INTERNAL_ERROR;
                    }
                    if (strcmp(code, FILE_NOT_FOUND) == 0) {
                        putfd = open(tempname, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                        code = CREATED;
                    }
                }

                //get content if content-length exists
                sscanf(ptrlen, "Content-Length: %d", &contentlength);
                int n = 0;
                for (int i = 0; i < contentlength; i++) {
                    n = read(fd, buffer, sizeof(char));
                    if (n <= 0) {
                        break;
                    } else if (n > 0) {
                        write(putfd, buffer, sizeof(char));
                        memset(&buffer, 0, sizeof(buffer));
                    }
                }
            }
            close(putfd);
            //Create the header with code 200 or 201 depending on if we created a file or not
            char *putcode = PUTOK;
            if (strcmp(code, CREATED) == 0) {
                putcode = PUTCREATED;
            }
            sendheader(fd, response, code, strlen(putcode), putcode);
        }
        //clear buffers
        memset(&header, 0, sizeof(header));
        memset(&headercopy, 0, sizeof(headercopy));
        memset(&content, 0, sizeof(content));
        memset(&response, 0, sizeof(response));

        //Close TCP Connection
        close(fd);
    }
}
