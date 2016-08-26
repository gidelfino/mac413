#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_SIZE 	1024

int main() {
	int  i, status;
    char path[MAX_SIZE], prompt[MAX_SIZE];
	char *input, *token, *command;
    char *parameters[MAX_SIZE];
	pid_t pid;

    while (42) {	// inicio do shell
    	if (getcwd(path, sizeof(path)) != NULL) {
    		sprintf(prompt, "(%s): ", path);
    		input = readline(prompt); // impressao do prompt leitura do input do usuario 
			if (input != NULL) {		  // adicao do input ao historico
				if (strcmp(input, "") == 0) 
					continue; // se o input for vazio, pular para a proxima iteracao
				add_history(input);
				token = strtok(input, " ");
				command = token;	  // separacao do input entre comando e parametros
				parameters[0] = token;
				for (i = 1; token != NULL; i++) {
					token = strtok(NULL, " ");
					parameters[i] = token;
				}
			}
			else 
				perror("readline()");
		}
 		else
 			perror("getcwd()");

 		if (strcmp(command, "chmod") == 0) {     // chmod <modo numerico> <nome do diretorio atual>
 			printf("%s %s\n", parameters[1], parameters[2]);
 			if (chmod(parameters[2], parameters[1]) != 0)
 				perror("chmod()");
 		}
 		else if (strcmp(command, "id") == 0) {
 			if (strcmp(parameters[1], "-u") == 0)
 				printf("%d\n", (int)getuid());		
 		}
 		else                                  // /bin/ls -1     e    ./ep1 <argumentos do EP1>
 			switch (pid = fork()) {
				case -1: // erro
					perror("fork()");
					exit(EXIT_FAILURE);
				case 0:  // estamos em um processo filho
					status = execve(command, parameters, 0);
					exit(status);
				default: // estamos em um processo pai 
					if ((waitpid(pid, &status, 0)) < 0) {
						perror("waitpid()");
						exit(EXIT_FAILURE);
					}
			}
		free(input);
	}
	return 0;
}
