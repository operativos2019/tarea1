#include <stdio.h>
#include <stdlib.h>

#define CONFIG_FILE_PATH  "config.conf"
#define CONFIG_FILE_DATA "PORT=8081"
 
/*
 * Funcion que crea el archivo de configuracion en caso que exista
 */
void createConfigFile() {
   
   FILE* file = fopen(CONFIG_FILE_PATH, "w");
   
   if(file == NULL) {
      printf("No fue posible crear el archivo de configuración.\n");
      exit(EXIT_FAILURE);
   }
   
   else {
      fputs(CONFIG_FILE_DATA, file);
      fclose(file);
   }
      
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