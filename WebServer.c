#include <stdio.h>      // I/O
#include <sys/socket.h> //sockets
#include <unistd.h>     //required for close()
#include <netinet/in.h> //required for socket address management
#include <string.h>     //required for memset()
#include <stdlib.h>

int main()
{
    int port = 8000; //localhost port

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
        return -1;
    }

    socklen_t address_len = sizeof socketAddress;                                 //kernel space required to copy de socket address
    errno = bind(fileDescriptor, (struct sockaddr *)&socketAddress, address_len); //System call bind to name the socket
    if (errno <= -1)
    {
        perror("Error while binding the socket");
        return -1;
    }

    int backlog = 30;                        //number of queued operations allowed
    errno = listen(fileDescriptor, backlog); //Creates the listener on the socket

    if (errno <= -1)
    {
        perror("Error while creating the listener");
        return -1;
    }

    int nextSocket;
    int exiting = 0;
    //http response header
    char *httpHeader = "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
    while (1)
    {
        printf("Serving...\n");
        //Accepts the next item in the queue
        errno = (nextSocket = accept(fileDescriptor, (struct sockaddr *)&socketAddress, &address_len));
        if (errno <= -1)
        {
            perror("Error while accepting next item on the queue");
            exiting = -1;
            break;
        }
        char request[10000] = {0};
        char response[400000] = {0};

        printf("Reading the request...\n");
        read(nextSocket, request, 10000); //reads the file
        //printf("%s\n", request);          //Get the request

        //split the GET request
        const char *begin = "GET /";
        const char *end = " HTTP";
        char *requestBody = NULL;
        char *start, *finish;
        if (start = strstr(request, begin))
        {
            start += strlen(begin);
            if (finish = strstr(start, end))
            {
                requestBody = (char *)malloc(finish - start + 1);
                memcpy(requestBody, start, finish - start);
                requestBody[finish - start] = '\0';
            }
        }
        
        printf("Reading the file 1\n");
        
        FILE *f = fopen("sample.html", "rb");
        fseek(f, 0, SEEK_END);
        int fsize = ftell(f);
        fseek(f, 0, 0);

        printf ("%d\n", fsize);

        char * message = (char * ) malloc(fsize);
        fread(message, fsize, 1, f);

        sprintf(response, "%s%s", httpHeader, message);

        //writes the file
        printf("Writing... \n");
        write(nextSocket, response, strlen(response));

        //closes the socket
        close(nextSocket);
        break;
    }
    return exiting;
}