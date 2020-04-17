#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <netinet/tcp.h>
#include "utils.h"

#define BUFLEN 1501
#define MAX_CLIENTS 1000
#define MAX(a,b) (((a)>(b))?(a):(b))

//mesajul primit de la un client UPD este de forma (topic, tip_date, continut)

//mesajul primit de la un client TCP este de forma

//argumentul primit este portul pe care serverul va deschide socketi

client *clients; //vector de structuri de tip client care retine toti clientii la un moment dat
int clients_nr; //numarul de clienti 'administrati' de server

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[]) 
{
	if (argc < 2) {
		usage(argv[0]);
	}

	int pass_socket_udp; // socket folosit pentru receptarea datagramelor de la udp
	int pass_socket_tcp; //socket folosit pentru stabilirea conexiunii serer-clienti tcp

	struct sockaddr_in udp_sock;
	struct sockaddr_in tcp_sock, client_addr;


	fd_set read_fds; //multimea de descriptori folosita de select()
	fd_set tmp_fds;  //multime auxiliara
	int fd_max;   //maximul valorilor fd din multimea read_fd

	int fd, sock_udp_fd;
	

	char buff[BUFLEN];
	//deschidere socket care asigura receptare mesaje server-client udp
	pass_socket_udp = socket(AF_INET, SOCK_DGRAM, 0);

	//deschidere socket pentru conexiunea tcp
	pass_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);

	udp_sock.sin_family = AF_INET;
	udp_sock.sin_port = htons(atoi(argv[1]));
	udp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	memset((void *)&tcp_sock, 0, sizeof(tcp_sock));
	tcp_sock.sin_family = AF_INET;
	tcp_sock.sin_port = htons(atoi(argv[1]));
	tcp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	//printf("port socket: %hu\n", ntohs(tcp_sock.sin_port));

	//aloca intial spatiu pentru 1 client
	clients = (client *)malloc(1 * sizeof(client));
	clients_nr = 0; //numarul de clienti

	//asociaza adresa pentru socketul 'udp_sock' tocmai deschis
	bind(pass_socket_udp, (struct sockaddr *) &udp_sock, sizeof(udp_sock));

	//asociaza adresa pentru socketul 'tcp_sock' tocmai deschis
	bind(pass_socket_tcp, (struct sockaddr *) &tcp_sock, sizeof(tcp_sock));

	listen(pass_socket_tcp, MAX_CLIENTS);

	// se adauga un nou file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(pass_socket_tcp, &read_fds);
	fd_max = pass_socket_tcp;

	// se adauga un nou file descriptor (socketul pe care se asteapta datagrame de la clientii udp) in multimea read_fds
	FD_SET(pass_socket_udp, &read_fds);
	fd_max = MAX(fd_max, pass_socket_udp);

	//introduce fd '0' in 'read_fds' pentru a permite citirea de la tastatura
    FD_SET(0, &read_fds);
    fd_max = MAX(fd_max, 0);

	memset(buff, 0, BUFLEN);
	
	while(1) {
		tmp_fds = read_fds; 

		select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);

		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if(i == 0) {
					printf("aici\n");
					memset(buff, 0, BUFLEN);
					fgets(buff, BUFLEN - 1, stdin);
	        
					if (strcmp(buff, "exit\n") == 0) {
						//inchidere server si toti clientii TCP conectati in acest moment la server
						goto exit;
					}
				}

				if(i == pass_socket_tcp) { //s-a primit o cerere de conexiune pe socketul pasiv
					//serverul accepta cererea de conexiune de la client
					printf("s-a primit o cerere de conexiune\n");
					socklen_t socklen;
					socklen_t client_len = sizeof(client_addr);
					int new_sock_fd = accept(pass_socket_tcp, (struct sockaddr *) &client_addr, &client_len);

					//dezactiveaza algoritmul lui Nagle
					int check = 1;
					setsockopt(new_sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(new_sock_fd, &read_fds);
					if (new_sock_fd > fd_max) { 
						fd_max = new_sock_fd;
					}
					memset(buff, 0, BUFLEN);
					recv(new_sock_fd, buff, sizeof(buff), 0);

					char ip[INET_ADDRSTRLEN]; 
        			inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); 

					printf("New client (%s) connected from %s:%hu.\n",
							buff, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					//serverul stie ca dupa ce a acceptat o conexiune trebuie sa primeasca Client_ID-ul celui care s-a conectat
					
				//	printf("Client_ID: %s\n", buff);
					//creeaza un membru 'client', continand id-ul celui care s-a conectat
					client* new_client = (client *)malloc(sizeof(client));
					//new_client.id_client = 
					//adauga clientul conectat in lista de clienti
					//clients_nr++;
					//clients = (client *)realloc(clients, 2 * clients_nr * sizeof(client));
					
					
				}
				else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buff, 0, BUFLEN);	
					int result_recv = recv(i, buff, sizeof(buff), 0);
					printf("am primit sirul inainte de conexiune inchisa: %s\n", buff);
					if (result_recv == 0) { //conexiune inchisa
						printf("Client (**%d**) disconnected\n", i);
						close(i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					}
					else {
						printf ("Clientul de pe socketul %d a trimis mesajul: %s\n", i, buff);
					}
				}

			}
		}

	/*	int recv = recvfrom(pass_socket_udp, buff, BUFLEN, 0, (struct sockaddr *) &udp_sock, &socklen);
		printf("am primit: %s\n", buff);
		memset(buff, 0, BUFLEN);*/
	}

	exit:

	close(pass_socket_tcp);
	close(pass_socket_udp);

	return 0;
}