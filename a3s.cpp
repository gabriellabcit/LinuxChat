
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_TCP_PORT 7000	// Default port
#define BUFLEN	1024		//Buffer length
#define LISTENQ	5

// Function Prototypes
static void SystemFatal(const char* );

int main (int argc, char **argv) {
	int i, maxi, nready, bytes_to_read, arg;
	int listen_sd, new_sd, port, maxfd, client[FD_SETSIZE];
	socklen_t client_len;
	struct sockaddr_in server, client_addr;
	char *bp, buf[BUFLEN];
   	ssize_t n;
   	fd_set rset, allset;

	switch(argc) {
		case 1:
			port = SERVER_TCP_PORT;	// Use the default port
		break;
		case 2:
			port = atoi(argv[1]);	// Get user specified port
		break;
		default:
			fprintf(stderr, "Usage: %s [port]\n", argv[0]);
			exit(1);
	}

	// Create a stream socket
	if ((listen_sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		SystemFatal("Cannot Create Socket!");
	}

	// set SO_REUSEADDR so port can be resused imemediately after exit, i.e., after CTRL-c
	arg = 1;
	if (setsockopt (listen_sd, SOL_SOCKET, SO_REUSEADDR, &arg, sizeof(arg)) == -1){
		SystemFatal("setsockopt");
	}
	// Bind an address to the socket
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connections from any client

	if (bind(listen_sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		SystemFatal("bind error");
	}
	// Listen for connections
	// queue up to LISTENQ connect requests
	listen(listen_sd, LISTENQ);

	maxfd	= listen_sd;	// initialize
	maxi	= -1;		// index into client[] array

	for (i = 0; i < FD_SETSIZE; i++){
		client[i] = -1;             // -1 indicates available entry
	}
	FD_ZERO(&allset);
	FD_SET(listen_sd, &allset);


	while (1){
		rset = allset;               // structure assignment
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listen_sd, &rset)){ // new client connection
			client_len = sizeof(client_addr);
			if ((new_sd = accept(listen_sd, (struct sockaddr *) &client_addr, &client_len)) == -1){
				SystemFatal("accept error");
			}
			printf("Client connected: remote address is  %s\n", inet_ntoa(client_addr.sin_addr));

			for (i = 0; i < FD_SETSIZE; i++){
				if (client[i] < 0){
					client[i] = new_sd;	// save descriptor
					break;
				}
			}
			if (i == FD_SETSIZE){
				printf ("Too many clients\n");
				continue;
			}

			FD_SET (new_sd, &allset);     // add new descriptor to set
			if (new_sd > maxfd){
				maxfd = new_sd;	// for select
			}
			if (i > maxi){
				maxi = i;	// new max index in client[] array
			}
			if (--nready <= 0){
				continue;	// no more readable descriptors
			}
		}

		for (i = 0; i <= maxi; i++){	// check all clients for data

			if (client[i] < 0){
				continue;
			}
			if (FD_ISSET(client[i], &rset)){
				printf("client %d is set\n", i);
				bp = buf;
				bytes_to_read = BUFLEN;
				//while ((n = read(client[i], bp, bytes_to_read)) > 0){
				n = read(client[i], bp, bytes_to_read);
				printf("Read %d bytes from client %d\n", n, i);
				if (n > 0){
					bp += n;
					bytes_to_read -= n;
				}
				if (n == 0) {//client disconnected
					printf("Client %d disconnected\n", i);
					FD_CLR(client[i], &rset);
					client[i] = -1;
					//printf("Client %d disconnected\n", i);
				}


				//echos message to all clients except the one that sent it
				for(int j = 0; j <= maxi; j++){
					if(client[j] >= 0 && j != i){
						write(client[j], buf, BUFLEN);
					}
				}
				if (--nready <= 0){
					break;        // no more readable descriptors
				}
			}
		}
	}
	return(0);
}

// Prints the error stored in errno and aborts the program.
static void SystemFatal(const char* message){
    perror (message);
    exit (EXIT_FAILURE);
}
