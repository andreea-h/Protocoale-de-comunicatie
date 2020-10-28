#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

void close_all_clients(fd_set fds, int fd_max, int tcp_sock) {
	int i;
	for(i = 0; i <= fd_max; i++) {
		if (FD_ISSET(i, &fds)) {
			close(i);
		}
	}
}

int check_client(char **nume_clienti, char *nume, int n) {
	int i;
	for(i = 0; i < n; i++) {
		if(nume_clienti[i] != NULL) {
			if(strcmp(nume_clienti[i], nume) == 0) {
				return 0; //numele de client a mai fost folosit
			}
		}
	}
	return 1; //"nume" nu a mai fost folosit pana acum
}

int main(int argc, char *argv[])
{
	produs produse[1000];
	int nr_produse = 0;
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];

	char **nume_clienti = (char **)malloc(1000 * sizeof(char *));

	char notification[100];
	char temp[4];

	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	int clients[10];
	int nr_clients = 0;

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");



		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) { 
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					//primeste numele clientului imediat dupa conectare
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					//printf("client nou: %s\n", buffer);
					//retine asociere intre socket si nume_client

					//verifica daca exista un client cu acel nume TO DO
					int check = check_client(nume_clienti, buffer, n);
					if(check == 1) { //se face adaugarea clientului doar daca numele sau nu a mai fost folosit
						nume_clienti[newsockfd] = (char *)malloc(500 * sizeof(char));
						strcpy(nume_clienti[newsockfd], buffer);
					}
					else if(check == 0) {
						continue;
					}

					

					//printf("client_adaugat %s\n", nume_clienti[newsockfd]);

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}

					printf("conexiune noua de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					//clientul este adaugat in lista de clienti
					clients[nr_clients++] = newsockfd;

				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Clientul de pe socketul %d a inchis conexiunea\n", i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						//primeste un mesaj de un client
						if(strncmp(buffer, "add", 3) == 0) {
							
							//extrage descrierea
							char *descriere = (char *)malloc(500 * sizeof(char));
							strcpy(descriere, buffer + 4);

							produs nou;
							nou.descriere = (char *)malloc(500 * sizeof(char));
							nou.nume_client = (char *)malloc(500 * sizeof(char));

							//gaseste nume_client asociat la socketul de pe care s-a primit mesajul
							char *client = (char *)malloc(500 * sizeof(char));
							strcpy(client, nume_clienti[i]);

							//printf("client: %s\n", client);
							strcpy(nou.nume_client, client);
							strcpy(nou.descriere, descriere);

							//printf("s-a adaugat: %s %s\n", nou.nume_client, nou.descriere);

							produse[nr_produse] = nou;
							nr_produse++;
						}
						else if(strncmp(buffer, "all", 3) == 0) {
							// timite catre client un string cu lista de produse
							printf("Cerere 'all' de la clientul de pe socketul %d\n", i);
							int j;
							char *response = (char *)malloc(5000 * sizeof(char));
							if(nr_produse != 0) {
								sprintf(response, "%s %s\n", produse[0].nume_client, produse[0].descriere);
								for(j = 1; j < nr_produse; j++) {
									char *temp = (char *)malloc(500 * sizeof(char));
									sprintf(temp, "%s %s\n", produse[j].nume_client, produse[j].descriere);
									strcat(response, temp);
								}

								//trimite raspuns catre client
								//printf("%s\n", response);
								n = send(i, response, strlen(response), 0);
								DIE(n < 0, "send");
							}
							else if(nr_produse == 0) { //trimite un mesaj corespunzator
								char *response = (char *)malloc(5000 * sizeof(char));
								strcpy(response, "Lista de cumparaturi este goala\n");

								n = send(i, response, strlen(response), 0);
								DIE(n < 0, "send");
							}
						}
						else if(strncmp(buffer, "show", 4) == 0) {
							//extrage numele de client (care a fost trimis impreuna cu comanda)
							// pentru a identifica in server numele clientului
							char *username = (char *)malloc(500 * sizeof(char));
							strcpy(username, buffer + 4);
							printf("Cerere de show de la :%s\n", username);

							//cauta produsele care au fost comandate de 'username' in vectorul 'produse'
							//daca exista produse in lista de cumparaturi
							char *response = (char *)malloc(5000 * sizeof(char));

							int gol_flag = 0;
							if(nr_produse != 0) {

								if(strcmp(produse[0].nume_client, username) == 0) {
									sprintf(response, "%s %s\n", produse[0].nume_client, produse[0].descriere);
								}
								int j;
								for(j = 1; j < nr_produse; j++) {
									char *nume = (char *)malloc(500 * sizeof(char));
									strcpy(nume, produse[j].nume_client);
									if(strcmp(nume, username) == 0) {
										char *temp = (char *)malloc(500 * sizeof(char));
										sprintf(temp, "%s %s\n", produse[j].nume_client, produse[j].descriere);
										strcat(response, temp);
										gol_flag = 1;
									}	
								}

								//trimite raspuns catre client
								//printf("%s\n", response);
								n = send(i, response, strlen(response), 0);
								DIE(n < 0, "send");
							}
							else if(nr_produse == 0 || gol_flag == 0) { //trimite un mesaj corespunzator daca nu s-au gasit produse adaugate
								char *response = (char *)malloc(5000 * sizeof(char));
								strcpy(response, "Lista de cumparaturi este goala\n");

								n = send(i, response, strlen(response), 0);
								DIE(n < 0, "send");
							}

						}


						/*
						int dest = atoi(buffer);
						send(dest, buffer + 2, strlen(buffer) - 2, 0);

						printf ("Clientul de pe socketul %d a trimis mesajul: %s\n", i, buffer);
					*/
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
