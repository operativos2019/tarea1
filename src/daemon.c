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
#define CONFIG_FILE_DIR   "/etc/webserver"
#define CONFIG_FILE_PATH  "/etc/webserver/config.conf"
#define CONFIG_FILE_DEFAULT_PORT "PORT=8001"
#define CONFIG_FILE_DEFAULT_LOG_PATH "LOGFILE=/var/log/webserver.log"  

struct stat s; 


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
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Ignore signal sent from child to parent process */
	signal(SIGCHLD, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	for (fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--) {
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
		pid_fd = open(pid_file_name, O_RDWR|O_CREAT, 0640);
		if (pid_fd < 0) {
			/* Can't open lockfile */
			exit(EXIT_FAILURE);
		}
		if (lockf(pid_fd, F_TLOCK, 0) < 0) {
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
void verifyConfigDir() {

   int err = stat(CONFIG_FILE_DIR, &s);

   if(-1 == err) {
      mkdir(CONFIG_FILE_DIR, 0700);
      printf("Directorio de configuracion creado.\n");
   }

}

/*
 * Funcion que verifica que el archivo de configuracion exista
 * en caso contrario lo crea
 */
void createConfigFile() {

   verifyConfigDir();
   
   FILE* file = fopen(CONFIG_FILE_PATH, "w");
   
   if(file == NULL) {
      printf("No fue posible crear el archivo de configuración.\n Intente con sudo ./ejecutable\n");
      exit(EXIT_FAILURE);
   }
   
   else {
      fputs(CONFIG_FILE_DEFAULT_PORT , file);
      fputs("\n\n" , file);
      fputs(CONFIG_FILE_DEFAULT_LOG_PATH, file);
      fclose(file);
   }

   printf("Archivo de configuración creado.\n");
      
}

/*
 *  Funcion que retorna el puerto especificado en el archivo de configuracion
 *  si no lo encuentra retorna NULL
 */
int* getPortFromConfigFile() {

   FILE* file;
 
   file = fopen(CONFIG_FILE_PATH, "r"); // read mode

   if(file == NULL) {
      createConfigFile();
      file = fopen(CONFIG_FILE_PATH, "r"); // read mode
   }

   if(file == NULL) {
      printf("No fue posible leer el archivo de configuración\n");
      exit(EXIT_FAILURE);
   }

   int* port = calloc(1, sizeof(int));

   while(!feof(file)) {
      if(fscanf(file,"PORT=%d", port) == 1) {
         break;
      }
      fgetc(file);
   }

   if((port != NULL) && (*port == '\0')) {
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
char* getLogPathFromConfigFile() {
   
   FILE* file;

   file = fopen(CONFIG_FILE_PATH, "r");

   if(file == NULL) {
      printf("No fue posible leer el archivo de configuración\n");
      return NULL;
   }

   char* logFilePath = (char*)calloc(256, sizeof(char));

   while(!feof(file)) {
      if(fscanf(file,"LOGFILE=%s", logFilePath) == 1) {
         break;
      }
      fgetc(file);
   }

   if((logFilePath != NULL) && (logFilePath[0] == '\0')) {
      printf("No se encontró el logfile path en el archivo de configuración\n"); 
      return NULL;
   }

   fclose(file);

   return logFilePath;

}
//METODOS DEL ARCHIVO TERMINAN ACA


/* Main function */
int main(int argc, char *argv[])
{

	//REVISAR FUNCION glibc
	/* It is also possible to use glibc function deamon()
	* at this point, but it is useful to customize your daemon. */
	//daemonize();

	//LEE LOS DATOS DEL ARCHIVO DE CONFIGURACION
	int* port = getPortFromConfigFile();
    char* logFilePath = getLogPathFromConfigFile();

	/* This global variable can be changed in function handling signal */
	running = 1;


	/* Try to open log file to this daemon */
	if (logFilePath != NULL) {
		log_stream = fopen(logFilePath, "a+");
		if (log_stream == NULL) {
			syslog(LOG_ERR, "Can not open log file: %s, error: %s",
				logFilePath, strerror(errno));
			log_stream = stdout;
		}
	} else {
		log_stream = stdout;
	}

	//AQUI COMIENZA EL CODIGO DEL SERVER

	int server_fd, new_socket; long valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Only this line has been changed. Everything is same.
    char *hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( *port );
    
    memset(address.sin_zero, '\0', sizeof address.sin_zero);
    
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
	
	static int counter = 0;

	/* Never ending loop of server */
	while (running == 1) {

		/* Debug print */
		
		int ret = fprintf(log_stream, "Debug: %d\n", counter++);
		if (ret < 0) {
			syslog(LOG_ERR, "Can not write to log stream: %s, error: %s",
				(log_stream == stdout) ? "stdout" : logFilePath, strerror(errno));
			break;
		}
		ret = fflush(log_stream);
		if (ret != 0) {
			syslog(LOG_ERR, "Can not fflush() log stream: %s, error: %s",
				(log_stream == stdout) ? "stdout" : logFilePath, strerror(errno));
			break;
		}


		printf("\n+++++++ Waiting for new connection ++++++++\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }
        
        char buffer[30000] = {0};
        valread = read( new_socket , buffer, 30000);
        printf("%s\n",buffer );
        write(new_socket , hello , strlen(hello));
        printf("------------------Hello message sent-------------------");
        close(new_socket);

		/* Real server should use select() or poll() for waiting at
		 * asynchronous event. Note: sleep() is interrupted, when
		 * signal is received. */
		sleep(delay);
	}

	return EXIT_SUCCESS;
}
