# Assignment 1: HTTP Server

This is an HTTP Server which processes incoming HTTP commands from clients.

### Functionality

The server creates, listens, and accepts connections on a socket that listens onto a port. There are three types of HTTP operations, GET, PUT, and HEAD.

##### Usage

./httpserver <port>

### Program Design

There is one helper function to go along with this code, "sendheader".

sendheader constructs the message that is written to the client. This displays the HTTP version, status code, and content if applicable.

The main function after checking edge cases hops into a while loop which will run while true.
This is how the HTTP Server stays online.
The header is read into a buffer, and it's checked for errors. After we read in the header we scan for the HTTP version, filename, and request type.
After checking the validity of the information, and finding out which request type it is, we start with our response.

First we check if its a GET or HEAD request, and because they are similar we can treat them almost the same. After grabbing the contents of the file we send the appropriate header as a response.

If it's a PUT request, we get the content length of the file so we know how much we need to read. We check if the file we are creating already exists, and if not we create the file. The program writes the contents into our new file and t hen sends the appropriate header. 

Lastly, we reset the memory of all the buffers used and close the socket.

### Error Codes
Status-Code Status-Phrase         Message-Body            Usage
200         OK                    OK\n                    When a method is Successful
201         Created               Created\n               When a URI’s file is created
400         Bad Request           Bad Request\n           When a request is ill-formatted
403         Forbidden             Forbidden\n             When the server cannot access the URI’s file
404         Not Found             Not Found\n             When the URI’s file does not exist
500         Internal Server Error Internal Server Error\n When an unexpected issue prevents processing
501         Not Implemented       Not Implemented\n       When a request includes an unimplemented Method

