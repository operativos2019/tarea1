#include <stdio.h> // I/O
#include <sys/socket.h> //sockets
#include <unistd.h> //required for close()
#include <netinet/in.h> //required for socket address management
#include <string.h> //required for memset()
#include <stdlib.h>



int main(){
    int port = 8080; //localhost port

    struct sockaddr_in socketAddress; 

    memset((char *) &socketAddress, '\0', sizeof socketAddress.sin_zero);
    //fill out the socket address structure
    socketAddress.sin_family = AF_INET; //sets up the socket family to AF_INET
    socketAddress.sin_addr.s_addr = INADDR_ANY; //sets up the address to this machine's IP address 
    socketAddress.sin_port = htons(port); //specifies port for clients to connect to this server

    int fileDescriptor, errno; //socket file descriptor and error number
    errno = (fileDescriptor = socket(AF_INET, SOCK_STREAM,0)); //Creates the fileDescriptor with an autoselected protocol
    if (errno <= -1){
        perror("Error while creating file descriptor");
        return -1;
    }

    socklen_t address_len = sizeof socketAddress; //kernel space required to copy de socket address
    errno = bind(fileDescriptor, (struct sockaddr *) &socketAddress, address_len); //System call bind to name the socket 
    if (errno <= -1){
        perror("Error while binding the socket");
        return -1;
    }

    
    int backlog = 30; //number of queued operations allowed
    errno = listen(fileDescriptor, backlog); //Creates the listener on the socket

    if (errno <= -1){
        perror("Error while creating the listener");
        return -1;
    }

    int nextSocket;
    int exiting = 0;
    //http response header
    char * httpHeader = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 18\n\nHello world!";
    while (1){
        printf("Serving...\n");
        //Accepts the next item in the queue
        errno = (nextSocket =  accept (fileDescriptor, (struct sockaddr *) &socketAddress, &address_len));
        if (errno <= -1){
            perror("Error while accepting next item on the queue");
            exiting = -1;
            break;
        }
        char buffer [50000] = {0};
        //reads the file
        printf("Reading...\n");
        read( nextSocket, buffer, 50000);
        //writes the file
        printf("Writing...\n");
        write(nextSocket, httpHeader, strlen(httpHeader));
        //closes the socket
        printf("Closing...\n");
        close(nextSocket);
    }
    return exiting;
}