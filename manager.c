/*
 * manager.c
 *
 *  Created on: Sep 28, 2013
 *      Author: csci551
 */


#include "file_io_op.h"
#include "sock_op.h"


/**
 * a[0]: stage #
 * a[1]: # of clients
 * a[2]: nounce
 */
long a[3];

void read_input_file(char *filename) {
	char buff[256];
	memset(buff, 0, sizeof(buff));

	//open file
	FILE* fp = open_file(filename, "r");
	int i = 0;
	while (read_line(fp, buff, sizeof(buff))) {
		//printf("Data read from file Line #%d: %s\n", i, buff);
		if (buff[0] == '#')
			continue;

		//tokenize the string base on spce delimiter and then parse into integer
		char* token = strtok(buff, " ");
		int k = 0;
		while (token) {
			if (k == 1)
				a[i] = atol(token);
			token = strtok(NULL, " ");
			k++;
		}
		i++;
		memset(buff, 0, sizeof(buff));
	}
}



int main(int argc, char *argv[]){
	//check for correct usage
	if(argc<2){
		printf("The correct usage is .//proja <filename>\n");
		perror("Incorrect usage of program!! exiting!!\n");
		return -1;
	}

	char *filename = argv[1];

	read_input_file(filename);

	//printf("Array value are a[0]=%ld, a[1]=%ld, a[2]=%ld\n",a[0],a[1],a[2]);


	return 0;
}
