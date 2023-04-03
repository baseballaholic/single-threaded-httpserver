#ifndef _HTTPSERVER_H_INCLUDE_
#define _HTTPSERVER_H_INCLUDE_

#define BUFFER_SIZE 4096

void sendheader(int fd, char *response, char *status, int length, char *content);

#endif
