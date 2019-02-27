#include <stdio.h>      // I/O
#include <sys/socket.h> //sockets
#include <unistd.h>     //required for access()
#include <netinet/in.h> //required for socket address management
#include <string.h>     //required for memset()
#include <stdlib.h>


#define HTTP_OK "HTTP/1.1 200 OK\n"
#define HTTP_BAD_REQUEST "HTTP/1.1 400 BAD REQUEST\n"
#define HTTP_FORBIDDEN "HTTP/1.1 403 FORBIDDEN\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 NOT FOUND\n"
#define HTTP_TOO_MANY "HTTP/1.1 429 TOO MANY REQUESTS\n"
#define HTTP_INTERNAL "HTTP/1.1 500 INTERNAL SERVER ERROR\n"
#define HTTP_UNAVAILABLE "HTTP/1.1 503 SERVICE UNAVAILABLE\n"

int main()
{
    int exiting = 1;
    char *httpHeader;  
    int port = 8080; //localhost port


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

    socklen_t address_len = sizeof socketAddress;                                 //kernel space required to copy de socket address
    errno = bind(fileDescriptor, (struct sockaddr *)&socketAddress, address_len); //System call bind to name the socket
    if (errno <= -1)
    {
        perror("Error while binding the socket");
        exiting = 0;

    }

    int backlog = 10000;                      //number of queued operations allowed
    errno = listen(fileDescriptor, backlog); //Creates the listener on the socket

    if (errno <= -1)
    {
        perror("Error while creating the listener");
        exiting = 0;
    }

    if (exiting == 0){

        printf("Try again later\n");
        return -1;
    }
    int nextSocket;
    //http response header
    while (1)
    {
        printf("Init...\n");
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
            char *requestBody = NULL;
            char *start, *finish;

            //generates the Get request string
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

            if (requestBody == NULL || requestBody == "\0")
            {
                perror("Error while reading request (2)");
                printf("Bad request");
                continue;
            }

            printf("Reading the file %s\n", requestBody);

            //*****check if file can be found****

            printf("Checking the file integrity");
            if (access(requestBody, R_OK) == -1)
            {
                printf ("... File is not ok\n");
                perror("File does not exists");
            }
            else
            {
                printf ("... File is ok\n");
                FILE *f = fopen(requestBody, "rb"); //open the file in binary mode
                fseek(f, 0, SEEK_END);

                printf ("Checking file size...");
                int fsize = ftell(f); //binary file size
                fseek(f, 0, 0);

                printf("%d\n", fsize);

                /*Creates the body message*/
                char * message = (char *)malloc(fsize);
                int nread = fread(message, 1, sizeof(message)*fsize, f);

                printf("Number read: %d\n", nread);
                /*for (int i = 0; i < nread; ++i){
                    printf("%d", message[i]);
                }
                printf("\n");*/

                /*Sets up the header*/
                httpHeader = HTTP_OK;
                char *header = (char *) malloc (strlen(httpHeader)+1);
                sprintf(header, "%s%s", httpHeader, "\n"); //creates the http header

                /*Sets up the response message*/

                char *response = (char *) malloc(nread + strlen(header));
                memcpy(response, header, strlen(header));
                memcpy(response + strlen(header), message, nread);
                //sprintf(response, "%s%s", header, message); //generates the http message body

                //printf("Response: %s", response);
                //writes the file
            
                printf("Writing...\n");
                write(nextSocket, response, strlen(header)+ nread);
                free(requestBody);
                free(response);

            }

            //closes the socket
            
            close(nextSocket);
        }
    }
    return exiting;
}