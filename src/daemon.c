/*
 *
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 * Contributor(s): Jiri Hnidek <jiri.hnidek@tul.cz>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

//SOBRE EL SERVER
#include <sys/socket.h>
#include <netinet/in.h>

//SOBRE EL SERVER
static int running = 0;
static int delay = 1;
static int counter = 0;
static char *conf_file_name = NULL;
static char *pid_file_name = NULL;
static int pid_fd = -1;
static char *app_name = NULL;
static FILE *log_stream;

//SOBRE EL ARCHIVO DE CONFIG
#define CONFIG_FILE_DIR "/etc/webserver"
#define CONFIG_FILE_PATH "/etc/webserver/config.conf"
#define CONFIG_FILE_DEFAULT_PORT "PORT=8001"
#define CONFIG_FILE_DEFAULT_LOG_PATH "LOGFILE=/var/log/webserver.log"

struct stat s;

//HTTP headers
#define HTTP_CHUNK "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: keep-alive\r\n\r\n"
#define HTTP_OK "HTTP/1.1 200 OK\r\n\r\n"
#define HTTP_BAD_REQUEST "HTTP/1.1 400 BAD REQUEST\r\n\r\nError 400:\nBad request\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 NOT FOUND\r\n\r\nError 404:\nFile not found\n"
#define HTTP_INTERNAL "HTTP/1.1 500\r\nINTERNAL SERVER ERROR\r\n\r\n"
#define NO_HEADER "NO_HEADER\n\n"

#define PATH "home/criss/"

/**
 * \brief This function will daemonize this app
 */
static void daemonize()
{
    pid_t pid = 0;
    int fd;

    /* Fork off the parent process */
    pid = fork();

    /* An error occurred */
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* On success: The child process becomes session leader */
    if (setsid() < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Ignore signal sent from child to parent process */
    signal(SIGCHLD, SIG_IGN);

    /* Fork off for the second time*/
    pid = fork();

    /* An error occurred */
    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* Success: Let the parent terminate */
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    /* Set new file permissions */
    umask(0);

    /* Change the working directory to the root directory */
    /* or another appropriated directory */
    chdir("/");

    /* Close all open file descriptors */
    for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }

    /* Reopen stdin (fd = 0), stdout (fd = 1), stderr (fd = 2) */
    stdin = fopen("/dev/null", "r");
    stdout = fopen("/dev/null", "w+");
    stderr = fopen("/dev/null", "w+");

    /* Try to write PID of daemon to lockfile */
    if (pid_file_name != NULL)
    {
        char str[256];
        pid_fd = open(pid_file_name, O_RDWR | O_CREAT, 0640);
        if (pid_fd < 0)
        {
            /* Can't open lockfile */
            exit(EXIT_FAILURE);
        }
        if (lockf(pid_fd, F_TLOCK, 0) < 0)
        {
            /* Can't lock file */
            exit(EXIT_FAILURE);
        }
        /* Get current PID */
        sprintf(str, "%d\n", getpid());
        /* Write PID to lockfile */
        write(pid_fd, str, strlen(str));
    }
}

//METODOS DEL ARCHIVO COMIENZAN ACA
/*
 * En caso de que el directorio especificado exista
 * de lo contrario lo crea
 */
void verifyConfigDir()
{

    int err = stat(CONFIG_FILE_DIR, &s);

    if (-1 == err)
    {
        mkdir(CONFIG_FILE_DIR, 0700);
        printf("Directorio de configuracion creado.\n");
    }
}

/*
 * Funcion que verifica que el archivo de configuracion exista
 * en caso contrario lo crea
 */
void createConfigFile()
{

    verifyConfigDir();

    FILE *file = fopen(CONFIG_FILE_PATH, "w");

    if (file == NULL)
    {
        printf("No fue posible crear el archivo de configuración.\n Intente con sudo ./ejecutable\n");
        exit(EXIT_FAILURE);
    }

    else
    {
        fputs(CONFIG_FILE_DEFAULT_PORT, file);
        fputs("\n\n", file);
        fputs(CONFIG_FILE_DEFAULT_LOG_PATH, file);
        fclose(file);
    }

    printf("Archivo de configuración creado.\n");
}

/*
 *  Funcion que retorna el puerto especificado en el archivo de configuracion
 *  si no lo encuentra retorna NULL
 */
int *getPortFromConfigFile()
{

    FILE *file;

    file = fopen(CONFIG_FILE_PATH, "r"); // read mode

    if (file == NULL)
    {
        createConfigFile();
        file = fopen(CONFIG_FILE_PATH, "r"); // read mode
    }

    if (file == NULL)
    {
        printf("No fue posible leer el archivo de configuración\n");
        exit(EXIT_FAILURE);
    }

    int *port = calloc(1, sizeof(int));

    while (!feof(file))
    {
        if (fscanf(file, "PORT=%d", port) == 1)
        {
            break;
        }
        fgetc(file);
    }

    if ((port != NULL) && (*port == '\0'))
    {
        printf("No se encontró el puerto en el archivo de configuración\n");
        return NULL;
    }

    fclose(file);

    return port;
}

/*
 *  Funcion que retorna la ruta al log file del webserver
 *  si no lo encuentra retorna NULL
 */
char *getLogPathFromConfigFile()
{

    FILE *file;

    file = fopen(CONFIG_FILE_PATH, "r");

    if (file == NULL)
    {
        printf("No fue posible leer el archivo de configuración\n");
        return NULL;
    }

    char *logFilePath = (char *)calloc(256, sizeof(char));

    while (!feof(file))
    {
        if (fscanf(file, "LOGFILE=%s", logFilePath) == 1)
        {
            break;
        }
        fgetc(file);
    }

    if ((logFilePath != NULL) && (logFilePath[0] == '\0'))
    {
        printf("No se encontró el logfile path en el archivo de configuración\n");
        return NULL;
    }

    fclose(file);

    return logFilePath;
}
//METODOS DEL ARCHIVO TERMINAN ACA

//METODOS DEL SERVER COMIENZAN ACA

void printError(const char *error, int size)
{
    for (int i = 0; i < size; i++)
    {
        if (error[i] != '\r')
        {
            printf("%c", error[i]);
        }
    }
    printf("\n");
}

/**
 * Receives the socket number, the message body, the HTTP header (ex. HTTP_OK), boolean whether is by chunks, and the bytes read
 * Sends the message to the socket with HTTP 1.1 protocol. 
 * 
 * Checks if the correct number of bytes were written and if they are not sends the signal to end the process
 * */
int sendResponse(int socket, const char *message, const char *header, int nread)
{
    //printError(message, nread);
    int actuallyWrote;
    char *response;
    if (header != NO_HEADER)
    {

        response = (char *)malloc(nread + strlen(header));
        memcpy(response, header, strlen(header));
        memcpy(response + strlen(header), message, nread);
        actuallyWrote = write(socket, response, strlen(header) + nread);
        free(response);
    }
    else
    {
        actuallyWrote = write(socket, message, nread);
    }
    if (actuallyWrote == -1)
    {
        fprintf(log_stream, "nread %d\n", nread);
        fprintf(log_stream, "MESSAGE:\n");
        for (int i = 0; i < nread; i++)
        {
            fprintf(log_stream, "%c", message[i]);
        }
        return 0;
        perror("error writing");
    }
    else
    {
        return 1;
    }
}
//METODOS DEL SERVER TERMINAN ACA

/* Main function */
int main(int argc, char **argv, char **envp)
{
    int counter = 0;
    char *logFilePath;

    logFilePath = NULL;
    int port;
    running = 1;

    int messageSize = 1000000;
    char *hexChunk = (char *)malloc(5);
    sprintf(hexChunk, "%x", messageSize);
    int exiting = 1;
    char *httpHeader = HTTP_OK; //placeholder
    char *message;              //body message
    int nread;
    int i;

    struct sockaddr_in socketAddress;

    memset((char *)&socketAddress, '\0', sizeof socketAddress.sin_zero);
    //fill out the socket address structure
    port = *(getPortFromConfigFile());
    socketAddress.sin_family = AF_INET;         //sets up the socket family to AF_INET
    socketAddress.sin_addr.s_addr = INADDR_ANY; //sets up the address to this machine's IP address
    socketAddress.sin_port = htons(port);       //specifies port for clients to connect to this server

    int fileDescriptor, errorNo;                                  //socket file descriptor and error number
    errorNo = (fileDescriptor = socket(AF_INET, SOCK_STREAM, 0)); //Creates the fileDescriptor with an autoselected protocol
    if (errorNo <= -1)
    {
        perror("Error while creating file descriptor");
        exiting = 0;
    }

    //Fixes the binding issue when the server is interrupted
    errorNo = setsockopt(fileDescriptor, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
    if (errorNo == -1)
    {
        perror("setsockopt failed");
        exiting = 0;
    }

    socklen_t address_len = sizeof socketAddress;                                   //kernel space required to copy de socket address
    errorNo = bind(fileDescriptor, (struct sockaddr *)&socketAddress, address_len); //System call bind to name the socket
    if (errorNo <= -1)
    {
        perror("Error while binding the socket");
        exiting = 0;
    }
    int backlog = 100000000;                   //number of queued operations allowed
    errorNo = listen(fileDescriptor, backlog); //Creates the listener on the socket

    if (errorNo <= -1)
    {
        perror("Error while creating the listener");
        exiting = 0;
    }

    if (exiting == 0)
    {

        printf("Try again later\n");
        return -1;
    }
    int nextSocket; //socket that will be used in each acceptance of a request

    logFilePath = getLogPathFromConfigFile();

    /* Try to open log file to this daemon */
    if (logFilePath != NULL)
    {
        log_stream = fopen(logFilePath, "a+");
        if (log_stream == NULL)
        {
            syslog(LOG_ERR, "Can not open log file: %s, error: %s",
                   logFilePath, strerror(errno));
            log_stream = stdout;
        }
    }
    else
    {
        log_stream = stdout;
    }

    /* Never ending loop of server */
    while (running == 1)
    {

        /* Debug print */

        int ret = fprintf(log_stream, "Debug: %d\n", counter++);
        if (ret < 0)
        {
            syslog(LOG_ERR, "Can not write to log stream: %s, error: %s",
                   (log_stream == stdout) ? "stdout" : logFilePath, strerror(errno));
            break;
        }
        ret = fflush(log_stream);
        if (ret != 0)
        {
            syslog(LOG_ERR, "Can not fflush() log stream: %s, error: %s",
                   (log_stream == stdout) ? "stdout" : logFilePath, strerror(errno));
            break;
        }

        exiting = 1;
        fprintf(log_stream, "Waiting... %d\n", counter);
        //Accepts the next item in the queue
        errorNo = (nextSocket = accept(fileDescriptor, (struct sockaddr *)&socketAddress, &address_len));
        if (errorNo == -1)
        {
            perror("Error while accepting next item on the queue");
            exiting = -1;
        }

        else //accepted correctly
        {

            sprintf(hexChunk, "%x", messageSize);
            char request[10000] = {0};

            read(nextSocket, request, 10000); //reads the file
            //No request sent
            if (request == "\0" || request == NULL)
            {
                httpHeader = HTTP_BAD_REQUEST;
                sendResponse(nextSocket, httpHeader, NO_HEADER, strlen(httpHeader));
            }
            else
            {
                /*********************  Getting the request ****************/
                fprintf(log_stream, "Reading the request... %d\n", counter++);

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
                    perror("Error while reading request");
                    httpHeader = HTTP_BAD_REQUEST;
                    sendResponse(nextSocket, httpHeader, NO_HEADER, strlen(httpHeader));
                }
                else
                {

                    requestBody = (char *)malloc(sizeof(requestString) * strlen(requestString) + strlen(PATH) * sizeof(PATH));
                    sprintf(requestBody, "%s%s", PATH, requestString);
                    //***check if file can be found****//
                    if (access(requestBody, R_OK) == -1)
                    {
                        free(requestBody);
                        perror("File does not exists or permissions are not granted");
                        httpHeader = HTTP_NOT_FOUND;
                        sendResponse(nextSocket, httpHeader, NO_HEADER, strlen(httpHeader));
                    }
                    else //file is good
                    {
                        FILE *f = fopen(requestBody, "rb"); //open the file in binary mode
                        fseek(f, 0, SEEK_END);
                        int fsize = ftell(f); //binary file size
                        fseek(f, 0, 0);

                        /* The content can be sent in 1 piece*/
                        if (messageSize >= fsize)
                        {
                            message = (char *)malloc(fsize);
                            nread = fread(message, 1, sizeof(message) * fsize, f);
                            /*Sets up the header*/
                            httpHeader = HTTP_OK;
                            exiting = sendResponse(nextSocket, message, httpHeader, nread);
                            free(message);     
                        }
                        else
                        {

                            /**Chunked content */
                            i = 1;
                            httpHeader = HTTP_CHUNK;
                            write(nextSocket, httpHeader, strlen(httpHeader));
                            httpHeader = NO_HEADER;
                            while (i * messageSize < fsize)
                            {
                                //each rn sizes 3
                                long unsigned int hexLen = strlen(hexChunk);
                                long unsigned int firstRN = hexLen + 2;
                                fprintf(log_stream, "Chunk size hex: %s\n", hexChunk);
                                message = (char *)malloc(firstRN + messageSize + 2);
                                memcpy(message, hexChunk, hexLen); 
                                memcpy(message + hexLen, "\r\n", 2);
                                nread = fread(message + firstRN, 1, messageSize, f);
                                if (nread < messageSize)
                                {
                                    fprintf(log_stream, "Chunk bytes read are less than expected: %d\n", nread);
                                }
                                if (nread == -1)
                                {
                                    fprintf(log_stream, "Error while reading chunk\n");
                                }
                                long unsigned int bodyLen = firstRN + nread;
                                long unsigned int totalSize = bodyLen + 2;
                                memcpy(message + bodyLen, "\r\n", 2);
                                i++;
                                exiting = sendResponse(nextSocket, message, NO_HEADER, totalSize);
                                free(message);
                                if (exiting == 0)
                                {
                                    fprintf(log_stream, "Exiting == 0 in chunks\n");
                                    break;
                                }
                            }
                            if (exiting == 1)
                            {
                                long unsigned int lastChunkSize = fsize - (i - 1) * messageSize;
                                sprintf(hexChunk, "%lx", lastChunkSize);
                                fprintf(log_stream, "Last chunk size hex: %s", hexChunk);
                                long unsigned int hexLen = strlen(hexChunk);
                                long unsigned int firstRN = hexLen + 2;
                                message = (char *)malloc(firstRN + lastChunkSize + 2);
                                memcpy(message, hexChunk, hexLen); //the hex
                                memcpy(message + hexLen, "\r\n", 2);
                                nread = fread(message + firstRN, 1, lastChunkSize, f); //the chunk
                                if (nread < lastChunkSize)
                                {
                                    fprintf(log_stream, "Last chunk bytes read are less than expected: %d", nread);
                                }
                                if (nread == -1)
                                {
                                    fprintf(log_stream, "Error while reading chunk\n");
                                }
                                long unsigned int bodyLen = firstRN + nread;
                                long unsigned int totalSize = bodyLen + 2;
                                memcpy(message + bodyLen, "\r\n", 2);
                                exiting = sendResponse(nextSocket, message, NO_HEADER, totalSize);
                                free(message);
                                if (exiting == 1)
                                {
                                    write(nextSocket, "0\r\n\r\n", 5);
                                    
                                }else{
                                    fprintf(log_stream, "Exiting == 0 in last chunk\n");
                                }
                            }
                            else
                            {

                                fprintf(log_stream, "Error in chunk: %d", i);
                            }
                        }
                        fclose(f);
                        free(requestBody);
                    }
                }
            }
            close(nextSocket);

            
        }

        /* Real server should use select() or poll() for waiting at
		 * asynchronous event. Note: sleep() is interrupted, when
		 * signal is received. */
        sleep(delay);
    }

    return EXIT_SUCCESS;
}
