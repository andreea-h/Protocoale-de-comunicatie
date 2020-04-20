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

int exit_error(char *string_error) {
	printf("%s", string_error);
	perror(string_error);
	return 1;
}

//intoarce 1 daca exista deja un client online cu id_client egal cu 'id'
//intoarce -1 daca nu exista inregistrat niciun client cu id_client egal cu 'id'
//intoarce 0 daca exista inregistrat un client cu acest id, dar care este momentan deconectat
int check_client_unique(char id[]) {
	int i;
	for(i = 0; i < clients_nr; i++) {
		if(strcmp(clients[i].id_client, id) == 0 && clients[i].connected == 1) { 
			return 1;
		}
		if(strcmp(clients[i].id_client, id) == 0 && clients[i].connected == 0) {
			return 0;
		}
	}
	return -1; 
}

//intoarce clientul care are un anumit client_id
client *get_client_by_id(char id[]) {
	int i;
	for(i = 0; i < clients_nr; i++) {
		if(strcmp(clients[i].id_client, id) == 0) {
			return &clients[i];
		}
	}
	return NULL;
}

//intoarce id-ul clientului conectat care are asociat un anumit socket
client *get_client(int socket) {
	int i;
	for(i = 0; i < clients_nr; i++) {
		if(clients[i].connected == 1 && clients[i].socket == socket) {
			return &clients[i];
		}
	}
	return NULL;
}


//inchide conexiunea cu toti cientii
void close_all_clients(fd_set fds, int fd_max, int tcp_sock) {
	int i;
	char *disconnect = (char *)malloc(30 * sizeof(char));
	strcpy(disconnect, "exit");
	for(i = 0; i <= fd_max; i++) {
		if (FD_ISSET(i, &fds)) {
			//send(i, disconnect, strlen(disconnect), 0);
			printf("mesaj trimis catre %d, %s\n", i, disconnect);
			close(i);
			//FD_CLR(i, &fds);
		}
	}
}

//elimina din lista de topicuri la care este abonat clientul 'id_client' acel topic care are numele 'topic_name'
void remove_topic(char id_client[], char topic_name[]) {
	client *changed_client = get_client_by_id(id_client);
	int i, poz = -1;
	for(i = 0; i < changed_client->topics_nr; i++) {
		if(strcmp(changed_client->topics[i].topic_name, topic_name) == 0) {
			poz = i;
		}
	}

	for(i = poz; i < changed_client->topics_nr - 1; i++) {
		changed_client->topics[i] = changed_client->topics[i + 1];
	}
	changed_client->topics_nr--;
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
	if(pass_socket_udp < 0) {
		exit_error("[!] Socket error...\n");
		return 1;
	}

	//deschidere socket pentru conexiunea tcp
	pass_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if(pass_socket_tcp < 0) {
		exit_error("[!] Socket error...\n");
		return 1;
	}

	int port = atoi(argv[1]);
	if(port == 0) {
		exit_error("[!] Converting function error..\n");
		return 1;
	}

	udp_sock.sin_family = AF_INET;
	udp_sock.sin_port = htons(port);
	udp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	memset((void *)&tcp_sock, 0, sizeof(tcp_sock));
	tcp_sock.sin_family = AF_INET;
	tcp_sock.sin_port = htons(port);
	tcp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	//printf("port socket: %hu\n", ntohs(tcp_sock.sin_port));

	//aloca intial spatiu pentru 1 client
	clients = (client *)malloc(1 * sizeof(client));
	clients_nr = 0; //numarul de clienti

	//asociaza adresa pentru socketul 'udp_sock' tocmai deschis
	int ret = bind(pass_socket_udp, (struct sockaddr *) &udp_sock, sizeof(udp_sock));
	if(ret < 0) {
		exit_error("[!] Bind error\n");
		return 1;
	}

	//asociaza adresa pentru socketul 'tcp_sock' tocmai deschis
	ret = bind(pass_socket_tcp, (struct sockaddr *) &tcp_sock, sizeof(tcp_sock));
	if(ret < 0) {
		exit_error("[!] Bind error\n");
		return 1;
	}

	ret = listen(pass_socket_tcp, MAX_CLIENTS);
	if(ret < 0) {
		exit_error("[!] Listen error\n");
		return 1;
	}

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

		ret = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
		if(ret < 0) {
			exit_error("[!] Select error\n");
			return 1;
		}

		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if(i == 0) { //se citeste de la stdin
					memset(buff, 0, BUFLEN);
					fgets(buff, BUFLEN - 1, stdin);
	        
					if (strcmp(buff, "exit\n") == 0) {
						//inchidere server si toti clientii TCP conectati in acest moment la server
						close_all_clients(read_fds, fd_max, pass_socket_tcp);
						return 0;
					}
					else { //singura comanda pe care o poate primi serverul de la stdin este 'exit'
						printf("[!] This command is not allowed...\n");
					}
				}

				else if(i == pass_socket_tcp) { //s-a primit o cerere de conexiune pe socketul pasiv
					//serverul accepta cererea de conexiune de la client daca aceasta este valida
					printf("s-a primit o cerere de conexiune, nr curent de clienti: %d\n", clients_nr);

					socklen_t socklen;
					socklen_t client_len = sizeof(client_addr);
					int new_sock_fd = accept(pass_socket_tcp, (struct sockaddr *) &client_addr, &client_len);

					if(new_sock_fd < 0) {
						exit_error("[!] Accept error...\n");
						return 1;
					}

					//dezactiveaza algoritmul lui Nagle
					int check = 1;
					setsockopt(new_sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 

					//primeste de la client id_ul acestuia
					memset(buff, 0, BUFLEN);
					recv(new_sock_fd, buff, sizeof(buff), 0);

					//verifica daca cererea primita este una de conectare de la un nou client 
					//sau este o cerere de reconectare
					//sau este o situatie care nu este permisa (conectarea simultana a doi clienti avand acelasi id_client)
					if(check_client_unique(buff) == -1) {
						printf("CLIENT NOU\n");
						//adauga clientul conectat in lista de clienti in situatia in care clientul este unul nou
						//adaugarea unui client in lista de clienti se face doar daca nu avem o reconectare

						//client new_client;

						clients = (client *)realloc(clients, (clients_nr + 1)* sizeof(client));

						strcpy(clients[clients_nr].id_client, buff); //id-ul clientului
						clients[clients_nr].connected = 1;
						clients[clients_nr].socket = new_sock_fd; //socketul pe care s-a acceptat conexiunea clientului su serverul
						clients[clients_nr].topics = (topic *)malloc(1 * sizeof(topic));
						clients[clients_nr].topics_nr = 0;
						clients_nr++;

						char ip[INET_ADDRSTRLEN]; 
	        			inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); 

						printf("New client (%s) connected from %s:%hu.\n",
								buff, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						printf("Socket pentru conectare: %d\n", new_sock_fd);
						//serverul stie ca dupa ce a acceptat o conexiune trebuie sa primeasca Client_ID-ul celui care s-a conectat
						
					//	printf("Client_ID: %s\n", buff);
						//creeaza un membru 'client', continand id-ul celui care s-a conectat
						
						// se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(new_sock_fd, &read_fds);
						if (new_sock_fd > fd_max) { 
							fd_max = new_sock_fd;
						}
					}
					else if(check_client_unique(buff) == 0) { //s-a facut o cerere de reconectare
						printf("RECONECTARE\n");
						printf("New client (%s) connected from %s:%hu.\n",
								buff, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						printf("Socket pentru conectare: %d\n", new_sock_fd);

						// se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(new_sock_fd, &read_fds);
						if (new_sock_fd > fd_max) { 
							fd_max = new_sock_fd;
						}

						//seteaza campul 'connected' pentru clientul care se reconecteaza
						client *current_client = get_client_by_id(buff);
						current_client->connected = 1;
						current_client->socket = new_sock_fd;
					}
					else { //id_client-ului nu este valid
						char *msg = (char *)malloc(50 * sizeof(char));
						strcpy(msg, "Problema la conectare! Acest id este deja folosit! Schimba id_client!\n");
						ret = send(new_sock_fd, msg, strlen(msg), 0);
						if(ret < 0) {
							exit_error("[!] Send error...\n");
							return 1;
						}
						close(new_sock_fd);
					}	
				}
				else if(i == pass_socket_udp) { //serverul va primi datagrame de la clienti udp
					socklen_t socklen;
					
					int rec = recvfrom(pass_socket_udp, buff, BUFLEN, 0, (struct sockaddr *) &udp_sock, &socklen);
					if(rec < 0) {
						exit_error("[!] Receiving error\n");
						return 1;
					}
					printf("am primit: %s, dimensiune date: %ld\n", buff, strlen(buff));
					
					//mesajul upd trebuie trimis mai departe catre clientii tcp abonati la topicul din mesaj
					//formateaza mesajul udp primit
					char topic[51];
					strncpy(topic, buff, 50);
					topic[50] = '\0';
					printf("topic mesaj primit: %s\n", topic);

					//tipul de date din mesajul primit
					printf("tipul de date: %hu\n", buff[50]);
					
					//payload 
					char msg[1500];
					int k;
					for(k = 0; k < 1500; k++) {
						msg[k] = buff[k + 51];
					}

					printf("mesaj: %s\n", msg);
					
					//se construieste mesajul tcp catre clienti pe baza mesajului upd primit

					//*****TODO*******
					

					
					memset(buff, 0, BUFLEN);
				}

				else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buff, 0, BUFLEN);	
					client_request client_msg;
					client *tcp_client = get_client(i);
					int result_recv = recv(i, &client_msg, sizeof(buff), 0);
					if(result_recv < 0) {
						exit_error("[!] Receiving error in server...\n");
						return 1;
					}
					
					if(result_recv == 0) { //conexiune inchisa
						//gaseste id_client
						printf("DECONECTARE\n");
						char *id = (char *)malloc(11 * sizeof(char));
						
						strcpy(id, tcp_client->id_client);
						tcp_client->connected = 0;
						tcp_client->socket = -1;

						printf("Client (%s) disconnected\n", id);
						printf("Socket pentru deconectare: %d\n", i);
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
						close(i);
					}
					else {
						printf ("Clientul (%s) de pe socketul %d a trimis mesajul:\n", tcp_client->id_client, tcp_client->socket);
						printf("topic_name: %s\n", client_msg.request_topic.topic_name);
						printf("type_req: %d\n", client_msg.request_topic.st);
						printf("client_id: %s\n", client_msg.client_id);
						if(client_msg.request_type == 'u') {
							printf("unsubscribe\n");
							//sterge topicul cu numele primit din lista topicurilor la care este abonat clientul
							remove_topic(tcp_client->id_client, client_msg.request_topic.topic_name); 
							printf("********************** lista topicuri\n");
							int l;
							for(l = 0; l < tcp_client->topics_nr; l++) {
								printf("topic_name: %s\n", tcp_client->topics[l].topic_name);
							}

							printf("**********************\n");

						}
						else if(client_msg.request_type == 's') {
							printf("subscribe\n");
							//TODO : trateaza situatia in care clientul cere sa se aboneze din nou la un topic la care s-a abonat deja
							//adauga topicul in lista de topicuri la care clientul este abonat
							topic new_topic;
							strcpy(new_topic.topic_name, client_msg.request_topic.topic_name);
							new_topic.st = client_msg.request_topic.st;
							tcp_client->topics = (topic *)realloc(tcp_client->topics, (tcp_client->topics_nr + 2) * sizeof(topic));
							tcp_client->topics[tcp_client->topics_nr++] = new_topic;
							printf("********************** lista topicuri\n");
							int l;
							for(l = 0; l < tcp_client->topics_nr; l++) {
								printf("topic_name: %s\n", tcp_client->topics[l].topic_name);
							}

							printf("**********************\n");
						}		
					}
				}
			}
		}

	/*	int recv = recvfrom(pass_socket_udp, buff, BUFLEN, 0, (struct sockaddr *) &udp_sock, &socklen);
		printf("am primit: %s\n", buff);
		memset(buff, 0, BUFLEN);*/
	}

	
	//inchide conexiunea cu toti clientii
	//(trimite un mesaj de deconectare catre clienti)
//	close_all_clients(read_fds, fd_max, pass_socket_tcp);

//	close(pass_socket_tcp);
//	close(pass_socket_udp);


	return 0;
}