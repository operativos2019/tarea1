#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONFIG_FILE_DIR   "/etc/webserver"
#define CONFIG_FILE_PATH  "/etc/webserver/config.conf"
#define CONFIG_FILE_DATA  "PORT=8081"

struct stat s; 

/*
 * En caso de que el directorio especificado no exista lo crea
 */
void verifyDir() {

   int err = stat(CONFIG_FILE_DIR, &s);

   if(-1 == err) {
      mkdir(CONFIG_FILE_DIR, 0700);
      printf("Directorio creado.\n");
   }

}

/*
 * Funcion que crea el archivo de configuracion en caso que exista
 */
void createConfigFile() {

   verifyDir();
   
   FILE* file = fopen(CONFIG_FILE_PATH, "w");
   
   if(file == NULL) {
      printf("No fue posible crear el archivo de configuración.\n Intente con sudo ./ejecutable");
      exit(EXIT_FAILURE);
   }
   
   else {
      fputs(CONFIG_FILE_DATA, file);
      fclose(file);
   }

   printf("Archivo creado.\n");
      
}

/*
 *  Funcion que retorna el puerto especificado en el archivo de configuracion
 */
int readConfigFile() {

   int port;
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

   if(fscanf(file, "PORT=%d", &port) == 1){ 
      if (port < 1024 || port > 65536|| port == 0 ) { 
         printf("ell puerto: %d, no es valido\n", port); 
         exit(1); 
      } 
      else printf("el puerto es: %d\n", port);
   }

   fclose(file);

   return port;

}


int main() {

   readConfigFile();

   return 0;

}
