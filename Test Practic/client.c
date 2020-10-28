#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_address server_port client_name\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc < 3) {
		usage(argv[0]);
	}

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);

	int fdmax;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	FD_SET(sockfd, &read_fds);
    fdmax = sockfd;
    FD_SET(0, &read_fds);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	ret = inet_aton(argv[1], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	//imediat dupa ce se conecteaza, clientul isi trimite numele
	n = send(sockfd, argv[3], strlen(argv[3]), 0);
	DIE(n < 0, "send");

	while (1) {
  		// se citeste de la tastatura
		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        if (FD_ISSET(0, &tmp_fds)) { //se citeste de la tastatura
	        
	        memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

	        buffer[strlen(buffer) - 1] = '\0';

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}

			if(strncmp(buffer, "add", 3) == 0) {
				//printf("aici\n");
				n = send(sockfd, buffer, strlen(buffer), 0);
				DIE(n < 0, "send");
			}
			
			if(strncmp(buffer, "all", 3) == 0) {
				// trimite comanda all la server pentru a cere lista de produse
				n = send(sockfd, "all", strlen("all"), 0);
				DIE(n < 0, "send");
			}

			if(strncmp(buffer, "show", 3) == 0) {
				// trimite comanda show prin care care cere lista cu produsele adaugate de userul curent
				// trimite impreuna cu "show" si numele de utilizator
				char *request = (char *)malloc(500 * sizeof(char));
				strcpy(request, "show");
				strcat(request, argv[3]);
				
				n = send(sockfd, request, strlen(request), 0);
				DIE(n < 0, "send");
			}

		}
		else {
			//clientul primeste ceva de la server
			
			memset(buffer, 0, BUFLEN);
            recv(sockfd, buffer, BUFLEN, 0);
            printf("%s\n", buffer);
		}
	}

	close(sockfd);

	return 0;
}
