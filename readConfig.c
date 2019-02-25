#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define CONFIG_FILE_DIR   "/etc/webserver"
#define CONFIG_FILE_PATH  "/etc/webserver/config.conf"
#define CONFIG_FILE_DEFAULT_PORT "PORT=8081"
#define CONFIG_FILE_DEFAULT_LOG_PATH "LOGFILE=/lo/que/gabito/quiera"  

struct stat s; 

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
      exit(EXIT_FAILURE);
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

int main() {

   int* port = getPortFromConfigFile();
   char* logFilePath = getLogPathFromConfigFile();

   if(port != NULL) {
      printf("Puerto = %d\n",*port);   
   }

   if(logFilePath != NULL) {
      printf("LogFile path = %s\n",logFilePath);   
   }

   return 0;

}
