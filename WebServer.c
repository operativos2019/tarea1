#include <stdio.h>      // I/O
#include <sys/socket.h> //sockets
#include <unistd.h>     //required for access()
#include <netinet/in.h> //required for socket address management
#include <string.h>     //required for memset()
#include <stdlib.h>

#define HTTP_CHUNK "HTTP/1.1 200 OK\nTransfer-Encoding: chunked\n\n"
#define HTTP_OK "HTTP/1.1 200 OK\n\n"
#define HTTP_BAD_REQUEST "HTTP/1.1 400 BAD REQUEST\n\n"
#define HTTP_FORBIDDEN "HTTP/1.1 403 FORBIDDEN\n\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 NOT FOUND\n\n"
#define HTTP_TOO_MANY "HTTP/1.1 429 TOO MANY REQUESTS\n\n"
#define HTTP_INTERNAL "HTTP/1.1 500 INTERNAL SERVER ERROR\n\n"
#define HTTP_UNAVAILABLE "HTTP/1.1 503 SERVICE UNAVAILABLE\n\n"
#define NO_HEADER "NO_HEADER\n\n"

#define PATH "/home/criss/"

/**
 * Receives the socket number, the message body, the HTTP header (ex. HTTP_OK), boolean whether is by chunks, and the bytes read
 * Sends the message to the socket with HTTP 1.1 protocol. 
 * */
void sendResponse(int socket, const char *message, const char *header, int nread, int * exiting)
{   int actuallyWrote;
    char *response;
    if (header != NO_HEADER)
    {
        response = (char *)malloc(nread + strlen(header));
        memcpy(response, header, strlen(header));
        memcpy(response + strlen(header), message, nread);
        printf("Writing...\n");
        actuallyWrote = write(socket, response, strlen(header) + nread);
    }
    else
    {
        actuallyWrote = write(socket, message, nread);
    }
    if (actuallyWrote == -1){
        perror("Error writing");
        *exiting = 0;
    }
    free(response);
    
}

int main()
{
    int counter = 0;
    int messageSize = 100000;
    char *hexChunk = (char *)malloc(5);
    sprintf(hexChunk, "%x", messageSize);
    int exiting = 1;
    char *httpHeader = HTTP_OK;
    int port = 8000; //localhost port
    char *message;   //body message
    int nread;
    int i;

    struct sockaddr_in socketAddress;

    memset((char *)&socketAddress, '\0', sizeof socketAddress.sin_zero);
    //fill out the socket address structure
    socketAddress.sin_family = AF_INET;         //sets up the socket family to AF_INET
    socketAddress.sin_addr.s_addr = INADDR_ANY; //sets up the address to this machine's IP address
    socketAddress.sin_port = htons(port);       //specifies port for clients to connect to this server

    int fileDescriptor, errno;                                  //socket file descriptor and error number
    errno = (fileDescriptor = socket(AF_INET, SOCK_STREAM, 0)); //Creates the fileDescriptor with an autoselected protocol
    if (errno <= -1)
    {
        perror("Error while creating file descriptor");
        exiting = 0;
    }

    //Fixes the binding issue when the server is interrupted
    errno = setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    if (errno == -1){
            perror("setsockopt failed");
            exiting = 0;
    }

    socklen_t address_len = sizeof socketAddress;                                 //kernel space required to copy de socket address
    errno = bind(fileDescriptor, (struct sockaddr *)&socketAddress, address_len); //System call bind to name the socket
    if (errno <= -1)
    {
        perror("Error while binding the socket");
        exiting = 0;
    }

   

    int backlog = 1000000;                     //number of queued operations allowed
    errno = listen(fileDescriptor, backlog); //Creates the listener on the socket

    if (errno <= -1)
    {
        perror("Error while creating the listener");
        exiting = 0;
    }

    if (exiting == 0)
    {

        printf("Try again later\n");
        return -1;
    }
    int nextSocket;
    //http response header
    while (1)
    {
        exiting = 1;
        int * pExiting = &exiting;
        printf("Waiting...\n");
        //Accepts the next item in the queue
        errno = (nextSocket = accept(fileDescriptor, (struct sockaddr *)&socketAddress, &address_len));
        if (errno <= -1)
        {
            perror("Error while accepting next item on the queue");
            exiting = -1;
            break;
        }
        else
        {

            char request[10000] = {0};

            read(nextSocket, request, 10000); //reads the file
            if (request == "\0" || request == NULL)
            {
                continue;
            }
            printf("Reading the request...\n");

            /*********************  Getting the request ****************/
            const char *begin = "GET /";
            const char *end = " HTTP";
            char *requestString = NULL;
            char *start, *finish;
            char *requestBody = NULL;

            //generates the Get request string
            if (start = strstr(request, begin))
            {
                start += strlen(begin);
                if (finish = strstr(start, end))
                {
                    requestString = (char *)malloc(finish - start + 1);
                    memcpy(requestString, start, finish - start);
                    requestString[finish - start] = '\0';
                }
            }

            if (requestString == NULL || requestString == "\0")
            {
                perror("Error while reading request (2)");
                printf("Bad request");
                continue;
            }else{
            
                requestBody = (char *) malloc( sizeof(requestString)* strlen(requestString) + strlen(PATH)*sizeof(PATH));
                sprintf(requestBody, "%s%s", PATH, requestString);
            }
            
            printf("Reading the file %s\n", requestBody);

            //*****check if file can be found****//

            printf("Checking the file integrity");
            if (access(requestBody, R_OK) == -1)
            {
                printf("... File is not ok\n");
                perror("File does not exists or permissions are not granted");
                httpHeader = HTTP_NOT_FOUND;
            }
            else
            {
                printf("... File is ok\n");

                FILE *f = fopen(requestBody, "rb"); //open the file in binary mode
                fseek(f, 0, SEEK_END);

                printf("Checking file size...");
                int fsize = ftell(f); //binary file size
                fseek(f, 0, 0);
                counter += fsize;

                printf(" %d\n", fsize);
                /* The content can be sent in 1 piece*/
                if (messageSize > fsize)
                {
                    message = (char *)malloc(fsize);
                    nread = fread(message, 1, sizeof(message) * fsize, f);
                    printf("Number read: %d\n", nread);
                    /*Sets up the header*/

                    httpHeader = HTTP_OK;
                    sendResponse(nextSocket, message, httpHeader, nread, pExiting);
                    free(requestBody);
                    free(message);
                }
                else
                {
                    /**Chunked content */
                    i = 1;
                    httpHeader = HTTP_CHUNK;
                    while (i * messageSize < fsize && exiting != 0)
                    {
                        
                        long int hexLen = 5 * sizeof(hexChunk);
                        int bodyLen = messageSize;
                        long int totalSize = hexLen + bodyLen + 4; //2 \r\n per message
                        message = (char *)malloc(totalSize);
                        memcpy(message, hexChunk, hexLen); //the hex
                        memcpy(message + hexLen, "\r\n", 2);
                        nread = fread(message + hexLen + 2, 1, bodyLen, f); //the chunk
                        memcpy(message + hexLen + 2 + bodyLen, "\r\n", 2);
                        i++;
                        sendResponse(nextSocket, message, httpHeader, totalSize, pExiting);
                        httpHeader = NO_HEADER;
                        free(message);
                    }
                    if (exiting == 1){
                        httpHeader = NO_HEADER;
                        int bodyLen = fsize - (i - 1) * messageSize;
                        sprintf(hexChunk, "%x", bodyLen);
                        long int hexLen = 5 * sizeof(hexChunk);
                        long int totalSize = hexLen + bodyLen + 4; //2 \r\n per message
                        message = (char *)malloc(totalSize);
                        memcpy(message, hexChunk, hexLen); //the hex
                        memcpy(message + hexLen, "\r\n", 2);
                        nread = fread(message + hexLen + 2, 1, bodyLen, f); //the chunk
                        memcpy(message + hexLen + 2 + bodyLen, "\r\n", 2);
                        sendResponse(nextSocket, message, httpHeader, totalSize, pExiting);
                        

                        free(message);
                        if (exiting == 1){                        
                            //finish the chunking
                            char *endMessage = "0\r\n\r\n";
                            write(nextSocket, endMessage, 6);
                        }else{
                            perror("Error last request");
                            close(nextSocket);
                        }
                    }else
                    {
                        perror("Error second to last request");
                        close(nextSocket);
                    }
                    

                    free(requestBody);
                    
                }
            }
            close(nextSocket);

            
        }
    }
    return exiting;
}