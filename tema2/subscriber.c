#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFLEN 1501

// comenzi:
// subscribe topic SF    (SF=0/1)
// unsubscribe topic
// exit

// feedback de la server: (un)subscribed topic (dupa ce a fost trimisa catre server comanda)
// va afisa toate mesajele pe care le primeste de la server: 'IP:PORT client_UDP - topic - tip_date - valoare mesaj'

void usage(char *file)
{
	fprintf(stderr, "Usage: %s <ID_Client> <IP_Server> <Port_Server>\n", file);
	exit(0);
}

int main(int argc, char *argv[]) 
{
	int sock_fd;
	struct sockaddr_in sock_addr;
	char buffer[BUFLEN];

	int val;

	if (argc < 4) {
		usage(argv[0]);
	}

	fd_set read_fds;
	fd_set tmp_fds;

	FD_ZERO(&tmp_fds);
    FD_ZERO(&read_fds);

	int fdmax;

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	FD_SET(sock_fd, &read_fds);
    fdmax = sock_fd;

    //introduce fd '0' in 'read_fds' pentru a permite citirea de la tastatura
    FD_SET(0, &read_fds);

	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(atoi(argv[3]));
	inet_aton(argv[2], &sock_addr.sin_addr);

	printf("adresa ip: %d\n", sock_addr.sin_addr.s_addr);

	val = connect(sock_fd, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
	//imediat ce subscriber-ul s-a conectat la server, se trimite catre server si Client_ID aferent acestuia
	send(sock_fd, argv[1], strlen(argv[1]), 0);
	printf("port: %hu\n", sock_addr.sin_port);

	while(1) {
		tmp_fds = read_fds;
		val = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);

		if (FD_ISSET(0, &tmp_fds)) { 
	        
	        //se va citi de la tastatura mesajul pe care subscriber doreste sa il trimita catre server
	        memset(buffer, 0, BUFLEN);

			fgets(buffer, BUFLEN - 1, stdin);
	        
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			// se trimite mesaj la server
			int sending = send(sock_fd, buffer, strlen(buffer), 0);
			printf("ai introdus: %s\n", buffer);
		}
		else {
			memset(buffer, 0, BUFLEN);
            recv(sock_fd, buffer, BUFLEN, 0);
            printf("mesaj primit de la server: %s\n", buffer);
		}
	}

	close(sock_fd);

	return 0;
}