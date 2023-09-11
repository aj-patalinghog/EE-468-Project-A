/*
 *  This is a simple shell program from
 *  rik0.altervista.org/snippetss/csimpleshell.html
 *  It's been modified a bit and comments were added.
 *
 *  But it doesn't allow misdirection, e.g., <, >, >>, or |
 *  The project is to fix this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/types.h>
#define BUFFER_SIZE 80
#define ARR_SIZE 80

// #define DEBUG 1  // In case you want debug messages

void parse_args(char *buffer, char *args[ARR_SIZE][ARR_SIZE], size_t args_size, size_t *nargs) {
   char *buf_args[args_size]; 
   char *wbuf = buffer;
   buf_args[0] = buffer; 
   /*
    *  The following replaces delimiting characters with '\0'. 
    *  Example:  " Aloha World\n" becomes "\0Aloha\0World\0\0"
    *  Note that the for-loop stops when it finds a '\0' or it
    *  reaches the end of the buffer.
    */   
   for(char **cp = buf_args; (*cp = strsep(&wbuf, " \n\t")) != NULL ;) {
      if((**cp != '\0') && (++cp >= &buf_args[args_size]))
         break; 
   }

   // Copy 'buf_args' into 2D array 'args' with each row being a separate command
   size_t j = 0;
   size_t k = 0;
   for(size_t i = 0; buf_args[i] != NULL; i++) { 
      if(strlen(buf_args[i]) > 0) // Store only non-empty tokens
         if(!strcmp(buf_args[i], "|")) { // When end of command is found, go to next row in 'args'
            args[j++][k] = NULL;
            k = 0;
         } else {
            args[j][k++] = buf_args[i];
         }
   }

   *nargs = (args[0][0] == NULL) ? 0 : j+1; // j is an index, add one to get total number
   args[j][k] = NULL;

   #ifdef DEBUG
   for(int a = 0; args[a][0] != NULL; a++) {
      printf("command %d: ", a+1);
      for(int b = 0; args[a][b] != NULL; b++) {
         printf("%s ", args[a][b]);
      }
      printf("\n");
   }
   #endif
}

int main(int argc, char *argv[], char *envp[]) {
   char buffer[BUFFER_SIZE];
   char *args[ARR_SIZE][ARR_SIZE];
   size_t num_args;
   pid_t pid;
   int fd[num_args][2];
   int executed;

   while(1) {
      printf("ee468>> "); 
      fgets(buffer, BUFFER_SIZE, stdin); // Read in command line
      parse_args(buffer, args, ARR_SIZE, &num_args); 

      if(num_args == 0) continue;
      if(num_args > 5) {
         perror("Too many pipes (4 max)\n");
         exit(1);
      }
      if(!strcmp(args[0][0], "exit")) exit(0);       

      for(int i = 0; i < num_args; i++) {
         // printf("i = %d\n", i);
         if(pipe(fd[i]) == -1) {
            perror("pipe");
            exit(1);
         }
         if((pid = fork()) < 0) {
            perror("fork");
            exit(1);
         } else if(pid) { // Parent
            #ifdef DEBUG
            printf("Executed commands = %d\n", executed);
            printf("Waiting for child (%d)\n", pid);
            #endif

            close(fd[i][1]); // Parent doesn't need write end of pipes
            pid = wait(NULL);
            executed++;

            #ifdef DEBUG
            printf("Child (%d) finished\n", pid);
            #endif
         } else { // Child executing the command
            if(i != 0) { // First command doesn't need to map stdin
               dup2(fd[i-1][0], STDIN_FILENO); // Map read end of previous command's pipe to stdin
               close(fd[i-1][0]);
            }

            if(i+1 != num_args) { // Last command prints to stdout
               dup2(fd[i][1], STDOUT_FILENO); // Map stdout to write end of current command's pipe 
               close(fd[i][1]);
            }

            if(execvp(args[i][0], args[i])) {
               puts(strerror(errno));
               exit(127);
            }
         }
      }
   }    
   return 0;
}

