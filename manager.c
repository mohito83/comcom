/*
 * manager.c
 *
 *  Created on: Sep 28, 2013
 *      Author: csci551
 */

#include "file_io_op.h"
#include "sock_op.h"
#include <sys/shm.h>
#include <sys/ipc.h>

#define SEGSIZE 10
#define MAXSIZE 256

/**
 * a[0]: stage #
 * a[1]: # of clients
 * a[2]: nounce
 */
int a[3];
const char* output_filename = "stage1.manager.out";
/*
 * For shared memory
 */
int shmid, cntr;
char *segptr;

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

		//tokenize the string base on space delimiter and then parse into integer
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

	//close the file stream
	close_file(fp);
}

void writeshm(char *segptr, char *text) {
	strcpy(segptr, text);
	//printf("Done...\n");
}

int readshm(char *segptr, char *s) {
	memset(s, 0, SEGSIZE);
	strcpy(s, segptr);
	return segptr == NULL;
}

void do_client() {
	//printf("I am child process!!\n");
	int shmid, server_port = 0;
	key_t key;
	char *shm, s[SEGSIZE], buffer[MAXSIZE];
	memset(buffer, 0, sizeof(buffer));

	int pid = getpid(); // to be sent to the server.

	//set up TCP connection
	struct sockaddr_in tcp_server;
	int tcp_client_sock_fd = create_tcp_socket();

	//populate the server addr structure
	//first wait till the process read the server port information
	// REUSED CODE :- http://www.tldp.org/LDP/lpg/node81.html, http://www.cs.cf.ac.uk/Dave/C/node27.html
	//--START
	key = ftok("./test", 'S');	//5678;
	/*
	 * Locate the segment.
	 */
	if ((shmid = shmget(key, SEGSIZE, 0666)) < 0) {
		perror("shmget");
		exit(1);
	}

	/*
	 * Now we attach the segment to our data space.
	 */
	if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		exit(1);
	}

	// --END

	//connect to the server
	while (connect(tcp_client_sock_fd, (struct sockaddr *) &tcp_server,
			sizeof(tcp_server)) < 0) {
		readshm(shm, s);
		printf(
				"do_client(): server port number read from shared memory is: %s\n",
				s);
		printf("do_client(): trying to connect the server\n");
		server_port = atoi(s);
		populate_sockaddr_in(&tcp_server, "localhost", server_port);
	}

	if (read(tcp_client_sock_fd, buffer, MAXSIZE - 1) < 0) {
		perror("recv");
	}

	printf("do_client: data received from server: %s\n", buffer);
	//perror("ERROR connecting");

	close(tcp_client_sock_fd);
}

int main(int argc, char *argv[]) {
	//check for correct usage
	if (argc < 2) {
		printf("The correct usage is ./proja <filename>\n");
		perror("Incorrect usage of program!! exiting!!\n");
		return -1;
	}

	char *filename = argv[1];
	//char output[256];

	//1. read the input parameter file
	read_input_file(filename);

	//4. Create shared memory area with the child processes.
	// REUSED CODE :- http://www.tldp.org/LDP/lpg/node81.html
	//--START
	key_t key;

	/* Create unique key via call to ftok() */
	key = ftok("./test", 'S');

	if ((shmid = shmget(key, SEGSIZE, IPC_CREAT | 0666)) < 0) {
		perror("shmget");
		exit(1);
	}

	/* Attach (map) the shared memory segment into the current process */
	if ((segptr = (char *) shmat(shmid, NULL, 0)) == (char *) -1) {
		perror("shmat");
		exit(1);
	}

	//--END

	//2. fork N child processes
	int i = 0;
	//char temp[10];
	while (i < a[1]) {
		if (fork() == 0) {
			do_client();
			exit(0);
		}

		i++;
	}

	//open file in output stream
	FILE* out_file_stream = open_file(output_filename, "w");
	fprintf(out_file_stream, "stage 1\n");

	//wait(0);
	//3. set up TCP server at manager
	struct sockaddr_in tcp_server, tmp, tcp_client;
	int tcp_serv_sock_fd = create_tcp_socket();
	populate_sockaddr_in(&tcp_server, "localhost", 0);
	if (bind_address(tcp_serv_sock_fd, tcp_server) < 0) {
		perror("Error biding the address to socket. Exiting!!");
		exit(0);
	}

	//get the port number information
	socklen_t size = sizeof(tmp);
	if (getsockname(tcp_serv_sock_fd, (struct sockaddr *) &tmp, &size) < 0) {
		perror("Error getting port number information!!");
		exit(0);
	} else {
		fprintf(out_file_stream, "manager port: %u\n", ntohs(tmp.sin_port));
	}

	//5. Put manager's port # in shared memory so that child processes and use it to connect the manager (server) socket
	//writeshm(segptr, temp);
	sprintf(segptr, "%u", ntohs(tmp.sin_port));

	//listen for incomming connections
	listen(tcp_serv_sock_fd, 5);

	int client_sock_fd, client = 1;
	socklen_t tcp_client_addr_len = sizeof(tcp_client);
	i = 0;
	while (i < a[1]) {
		printf("server:: waiting for connection!!\n");
		client_sock_fd = accept(tcp_serv_sock_fd,
				(struct sockaddr *) &tcp_client, &tcp_client_addr_len);
		if (client_sock_fd < 0)
			perror("ERROR on accept");

		fprintf(out_file_stream, "client %d port: %u\n", client,
				tcp_client.sin_port);

		//send nounce to the client
		if (write(client_sock_fd, "hello!!", 8) < 0)
			perror("Error in sending data to client\n");

		close(client_sock_fd);
		client++;
		i++;
	}

	//6. Do data transfer and log it in the log file.

	//removing shared memory area
	shmctl(shmid, IPC_RMID, 0);

	//close server socket before exiting the application
	close(tcp_serv_sock_fd);

	//printf("Array value are a[0]=%ld, a[1]=%ld, a[2]=%ld\n",a[0],a[1],a[2]);

	return 0;
}
