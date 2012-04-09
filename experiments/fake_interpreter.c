#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv){
	char *pwd = getcwd(NULL, 0);
	printf("pwd: %s\n", pwd);
	free(pwd);
	
	printf("args:\n");
	for(int i = 0; i < argc; i++)
		printf("%d: %s\n", i, argv[i]);
}