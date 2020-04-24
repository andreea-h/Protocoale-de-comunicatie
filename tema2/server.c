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
#include <math.h>
#include "utils.h"

#define BUFLEN 1551
#define MAX_CLIENTS 1000
#define MAX(a,b) (((a)>(b))?(a):(b))


client *clients; //vector de structuri de tip client care retine toti clientii la un moment dat
int clients_nr; //numarul de clienti 'administrati' de server

void usage(char *file) {
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(1);
}

int exit_error(char *string_error) {
	printf("%s", string_error);
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
	for(i = 0; i <= fd_max; i++) {
		if (FD_ISSET(i, &fds)) {
			close(i);
		}
	}
}

//elimina din lista de topicuri la care este abonat clientul 'id_client' acel topic care are numele 'topic_name'
//intoarce -1 daca clientul nu este abonat la topicul 'topic_name'
int remove_topic(char id_client[], char topic_name[]) {
	client *changed_client = get_client_by_id(id_client);
	int i, poz = -1;
	for(i = 0; i < changed_client->topics_nr; i++) {
		if(strcmp(changed_client->topics[i].topic_name, topic_name) == 0) {
			poz = i;
		}
	}
	if(poz == -1) { //s-a trimis comanda 'unsubscribe <topic_inexistent>'
		return -1;
	}
	for(i = poz; i < changed_client->topics_nr - 1; i++) {
		changed_client->topics[i] = changed_client->topics[i + 1];
	}
	changed_client->topics_nr--;
	return 1; //topicul a fost eliminat cu succes din lista topicurilor la care este abonat clientul
}

//intoarce indicele corespunzator din lista de topicuri
//(atunci cand 'topic_name' exista in lista de topicuri 'topics_list' cu 'topics_nr' elemente)
//in caz contrar, intoarce -1 (topic nou)
int find_topic_name(topic *topics_list, int topics_nr, char topic_name[]) {
	int i;
	if(topics_nr == 0) {
		return -1;
	}
	for(i = 0; i < topics_nr; i++) {
		if(strcmp(topics_list[i].topic_name, topic_name) == 0) {
			return i;
		}
	}
	return -1;
}

//returneaza -1 in cazul in care formatul payload-ului din 'pub_message' este incorect
//returneaza 1 in cazul in care s-a extras cu succes payload-ul din 'pub_message'
int extract_payload(publisher_message *pub_message, subscriber_message *sub_message) {
	if(pub_message->data_type == 0) { //tipul INT
		//extrage octetul de semn
		uint8_t *sign = (uint8_t *)(pub_message->message);
		if(*sign != 0 && *sign != 1) {
			return -1;
		}
		uint32_t *number = (uint32_t *)calloc(1, sizeof(uint32_t));

		memcpy(number, pub_message->message + 1, 4);
		*number = ntohl(*number);
	
		int64_t result = *number;
		if(*sign == 1) {    //numar negativ
			result = (-1) * result;
		}
		
		sprintf(sub_message->message, "%lld", (long long)result);

		free(number);
		return 1;
	}
	else if(pub_message->data_type == 1) { //tipul SHORT_REAL
		uint16_t number;
		memcpy(&number, pub_message->message, 4);
		number = ntohs(number);
		double result = number;
		result = result / 100;
		sprintf(sub_message->message, "%.2f",result);
		return 1;
	}
	else if(pub_message->data_type == 2) { //tipul FLOAT

		//extrage octetul de semn
		uint8_t *sign = (uint8_t *)(pub_message->message);
		if(*sign != 0 && *sign != 1) { //format invalid
			return -1; 
		}
		uint32_t first_part;
		uint32_t *nr = (uint32_t *)(pub_message->message + 1);
		
		*nr = ntohl(*nr);
		memcpy(&first_part, nr, 4);

		uint8_t second_part;
		memcpy(&second_part, pub_message->message + 5, 1);

		double power = pow(10, second_part);
		double rez = first_part / power;

		if(*sign == 1) {
			rez = (-1) * rez;
		}

		sprintf(sub_message->message, "%lf", rez);
		return 1;
	}
	else if(pub_message->data_type == 3) { //tipul STRING
		memcpy(sub_message->message, pub_message->message, sizeof(sub_message->message));
		return 1;
	}
	return -1;
}

//elibereaza memoria alocata pentru structura care retine informatiile despre clienti
void free_function() {
	int i;
	for(i = 0; i < clients_nr; i++) {
		//free pentru vectorul care retine topicurile la care e abonat fiecare client
		if(clients[i].topics_nr != 0) {
			free(clients[i].topics);
		}
		//elibereaza memoria alocata pentru stocarea mesajelor in situatia sf=1
		if(clients[i].stored != 0) {
			int k;
			for(k = 0; k < clients[i].stored; k++) {
			//	free(&(clients[i].stored_messages[k]));
			}
			free(clients[i].stored_messages);
		}
		else {
		//	free(&(clients[i].stored_messages[0]));
			free(clients[i].stored_messages);
		}
	}
	free(clients);
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


	fd_set read_fds;//multimea de descriptori folosita de select()
	FD_ZERO(&read_fds);
	fd_set tmp_fds; //multime auxiliara
	FD_ZERO(&tmp_fds);
	int fd_max;   //maximul valorilor fd din multimea read_fd

	int fd, sock_udp_fd;
	

	char buff[BUFLEN];
	//deschidere socket care asigura receptare mesaje server-client udp
	pass_socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if(pass_socket_udp < 0) {
		exit_error("[!] Socket error...\n");
		exit(1);
	}

	//deschidere socket pentru conexiunea tcp
	pass_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if(pass_socket_tcp < 0) {
		exit_error("[!] Socket error...\n");
		exit(1);
	}

	int port = atoi(argv[1]);
	if(port == 0) {
		exit_error("[!] Converting function error..\n");
		exit(1);
	}

	udp_sock.sin_family = AF_INET;
	udp_sock.sin_port = htons(port);
	udp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	memset((void *)&tcp_sock, 0, sizeof(tcp_sock));
	tcp_sock.sin_family = AF_INET;
	tcp_sock.sin_port = htons(port);
	tcp_sock.sin_addr.s_addr = htonl(INADDR_ANY);

	//aloca intial spatiu pentru 1 client
	clients = (client *)calloc(1, sizeof(client));
	clients_nr = 0; //numarul de clienti

	//asociaza adresa pentru socketul 'udp_sock' tocmai deschis
	int ret = bind(pass_socket_udp, (struct sockaddr *) &udp_sock, sizeof(udp_sock));
	if(ret < 0) {
		exit_error("[!] Bind error\n");
		exit(1);
	}

	//asociaza adresa pentru socketul 'tcp_sock' tocmai deschis
	ret = bind(pass_socket_tcp, (struct sockaddr *) &tcp_sock, sizeof(tcp_sock));
	if(ret < 0) {
		exit_error("[!] Bind error\n");
		exit(1);
	}

	ret = listen(pass_socket_tcp, MAX_CLIENTS);
	if(ret < 0) {
		exit_error("[!] Listen error\n");
		exit(1);
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
			exit(1);
		}

		for (int i = 0; i <= fd_max; i++) {
			if (FD_ISSET(i, &tmp_fds)) {

				if(i == 0) { //se citeste de la stdin
					memset(buff, 0, BUFLEN);
					fgets(buff, BUFLEN - 1, stdin);
	        
					if (strcmp(buff, "exit\n") == 0) {
						//inchidere server si toti clientii TCP conectati in acest moment la server
						free_function();
						close_all_clients(read_fds, fd_max, pass_socket_tcp);
						return 0;
					}
					else { //singura comanda pe care o poate primi serverul de la stdin este 'exit'
						printf("[!] This command is not allowed...\n");
					}
				}

				else if(i == pass_socket_tcp) { //s-a primit o cerere de conexiune pe socketul pasiv
					//serverul accepta cererea de conexiune de la client daca aceasta este valida
					socklen_t socklen;
					socklen_t client_len = sizeof(client_addr);
					int new_sock_fd = accept(pass_socket_tcp, (struct sockaddr *) &client_addr, &client_len);

					if(new_sock_fd < 0) {
						exit_error("[!] Accept error...\n");
						exit(1);
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
						//adauga clientul conectat in lista de clienti in situatia in care clientul este unul nou
						//adaugarea unui client in lista de clienti se face doar daca nu avem o reconectare

						clients = (client *)realloc(clients, (clients_nr + 1)* sizeof(client));

						strcpy(clients[clients_nr].id_client, buff); //id-ul clientului
						clients[clients_nr].connected = 1;
						clients[clients_nr].socket = new_sock_fd; //socketul pe care s-a acceptat conexiunea clientului cu serverul
						clients[clients_nr].topics = (topic *)calloc(1, sizeof(topic));
						clients[clients_nr].topics_nr = 0;
						clients[clients_nr].stored_messages = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						clients[clients_nr].stored = 0; 
						clients_nr++;

						char ip[INET_ADDRSTRLEN]; 
	        			inet_ntop(AF_INET, &(client_addr.sin_addr), ip, INET_ADDRSTRLEN); 

						printf("New client (%s) connected from %s:%hu.\n",
								buff, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						//serverul stie ca dupa ce a acceptat o conexiune trebuie sa primeasca Client_ID-ul celui care s-a conectat
						
						// se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(new_sock_fd, &read_fds);
						if (new_sock_fd > fd_max) { 
							fd_max = new_sock_fd;
						}
						//semnaleaza printr-un mesaj trimis catre client faptul ca client_id-ul este unul valid
						subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						strcpy(server_msg->message, "Available Client_Id.\n");
						
						ret = send(new_sock_fd, server_msg, sizeof(subscriber_message), 0);
						if(ret < 0) {
							exit_error("[!] Send error in server...\n");
							exit(1);
						}
						free(server_msg);
					}
					else if(check_client_unique(buff) == 0) { //s-a facut o cerere de reconectare
						printf("New client (%s) connected from %s:%hu.\n",
								buff, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						//se adauga noul socket intors de accept() la multimea descriptorilor de citire
						FD_SET(new_sock_fd, &read_fds);
						if (new_sock_fd > fd_max) { 
							fd_max = new_sock_fd;
						}

						//seteaza campul 'connected' pentru clientul care se reconecteaza
						client *current_client = get_client_by_id(buff);
						current_client->connected = 1;
						current_client->socket = new_sock_fd;

						//semnaleaza printr-un mesaj trimis catre client faptul ca client_id-ul este unul valid
						subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						strcpy(server_msg->message, "There are messages that need to be forwarded?\n");
						
						ret = send(new_sock_fd, server_msg, sizeof(subscriber_message), 0);
						if(ret < 0) {
							exit_error("[!] Send error in server...\n");
							exit(1);
						}

						//trimite catre client mesajele stocate pentru forward
						int c = 0;
						
						for(c = 0; c < current_client->stored; c++) {
							ret = send(new_sock_fd, &(current_client->stored_messages[c]), sizeof(subscriber_message), 0);
							if(ret < 0) {
								exit_error("[!] Send error in server...\n");
								exit(1);
							}
							
							//odata trimise mesajele, acestea vor fi eliminate din vector
							int j;
							printf("STORED pentru %s  ....  %d\n", current_client->id_client, current_client->stored);
							for(j = c; j < current_client->stored - 1; j++) {
								current_client->stored_messages[j] = current_client->stored_messages[j + 1];
							}
							c--;
							(current_client->stored)--;
							//free(&(current_client->stored_messages[c]));
						}	
						
						//dupa trimiterea mesajelor, se elibereaza memoria alocata pentru stocarea acestora
						free(server_msg);
					}
					else { //id_client-ului nu este valid -> se va trimite catre client un mesaj
						   //astfel incat clientul sa interpreteze eroarea din mesajul pe care acesta a intentionat sa il trimita catre server
						subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						strcpy(server_msg->message, "[!] This Client_ID is already in use! Try to use another Client_ID!\n");
						
						ret = send(new_sock_fd, server_msg, sizeof(subscriber_message), 0);
						if(ret < 0) {
							exit_error("[!] Send error in server...\n");
							exit(1);
						}
						free(server_msg);
						close(new_sock_fd);
					}	
				}
				else if(i == pass_socket_udp) { //serverul va primi datagrame de la clienti udp
					memset(buff, 0, BUFLEN);
					int check = 1;
					setsockopt(i, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 

					socklen_t socklen;
					
					//memoreaza mesajul udp trimis la server de catre clientul upd
					publisher_message *pub_message = (publisher_message *)calloc(1, sizeof(publisher_message));
					
					memset(buff, 0, BUFLEN);
					int rec = recvfrom(pass_socket_udp, buff, BUFLEN, 0, (struct sockaddr *) &udp_sock, &socklen);
				
					if(rec < 0) {
						exit_error("[!] Receiving error\n");
						exit(1);
					}
					
					//mesajul upd trebuie trimis mai departe catre clientii tcp abonati la topicul din mesaj
					//formateaza mesajul udp primit
					
					memmove(pub_message, buff, sizeof(publisher_message));

					//se construieste mesajul tcp catre clienti pe baza mesajului upd primit
					//'sub_message' retine mesajul trimis clientilor TCP de catre server
					subscriber_message *sub_message = (subscriber_message *)calloc(1, sizeof(subscriber_message));
					
					strcpy(sub_message->topic_name, pub_message->topic_name);
					if(pub_message->data_type == 0) {
						strcpy(sub_message->data_type, "INT");
					}
					else if(pub_message->data_type == 1) {
						strcpy(sub_message->data_type, "SHORT_REAL");
					}
					else if(pub_message->data_type == 2) {
						strcpy(sub_message->data_type, "FLOAT");
					}
					else if(pub_message->data_type == 3) {
						strcpy(sub_message->data_type, "STRING");
					}

					//formateaza payload-ul mesajului care trebuie trimis de catre server
					//completeaza campul 'sub_message.message' cu valoarea payloadului extras din 'pub_message'
					int set_payload = extract_payload(pub_message, sub_message);
					if(set_payload < 0) {
						exit_error("[!] Payload format error...\n");
						continue;
					}
				
					sub_message->client_port = ntohs(udp_sock.sin_port);
                    strcpy(sub_message->ip_source, inet_ntoa(udp_sock.sin_addr));

                    //cauta clientii care au in lista de topicuri acele topicuri cu numele 'sub_message.topic_name'
                    int i;
                    for(i = 0; i < clients_nr; i++) {
                    	//intoarce 1 daca topic_name exista in lista de topicuri la care este abonat clientul
                    	//clientul de la indexul 'i' este abonat la topic
                    	int pos = find_topic_name(clients[i].topics, clients[i].topics_nr, sub_message->topic_name);
                    	if(pos != -1 && clients[i].connected == 1) { //clientul este conectat
                    		int check = 1;
							setsockopt(clients[i].socket, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 

                    		int ret = send(clients[i].socket, sub_message, sizeof(subscriber_message), 0);
                    		if(ret < 0) {
								exit_error("[!] Send error...\n");
								exit(1);
							}
                    	}
                    	//clientul de la indexul 'i' este deconectat momentan
                    	//dar abonat la topic avand SF == 1
                    	else if(pos != -1 && clients[i].connected == 0 && clients[i].topics[pos].st == 1) {
                    		clients[i].stored_messages = realloc(clients[i].stored_messages, (clients[i].stored + 1) * sizeof(subscriber_message));
                    		memcpy(&clients[i].stored_messages[clients[i].stored], sub_message, sizeof(subscriber_message));
                    		(clients[i].stored)++;
                    		printf("STORED pentru %s  ....  %d\n", clients[i].id_client, clients[i].stored);
                    	}
                    }
                    free(pub_message);
                    free(sub_message);
				}

				else {
					int check = 1;
					setsockopt(i, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buff, 0, BUFLEN);	
					client_request client_msg;
					client *tcp_client = get_client(i);
					int result_recv = recv(i, &client_msg, sizeof(buff), 0);
					if(result_recv < 0) {
						exit_error("[!] Receiving error in server...\n");
						exit(1);
					}
					
					if(result_recv == 0) { //conexiune inchisa
						//gaseste id_client
						char *id = (char *)calloc(11, sizeof(char));
						
						strcpy(id, tcp_client->id_client);
						tcp_client->connected = 0;
						tcp_client->socket = -1;

						printf("Client (%s) disconnected\n", id);
						//se scoate din multimea de citire socketul inchis 
						free(id);
						FD_CLR(i, &read_fds);
						close(i);
					}
					else {
						if(client_msg.request_type == 'u') {
							
							//printf("unsubscribe\n");
							//sterge topicul cu numele primit din lista topicurilor la care este abonat clientul
							//functia 'remove_topic' intoarce -1 daca topicul nu este prezent in lista topicurilor la care este abonat clientul
							int result = remove_topic(tcp_client->id_client, client_msg.request_topic.topic_name); 
							if(result == -1) {
								//trimite catre client un mesaj de eroare prin intermediul caruia clientul poate sa interpreteze problema 
								//care a aparut in continutul datelor trimise catre server
								subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
								strcpy(server_msg->message, "[!] Invalid unsubscribe command! You are not subscribed to this topic!\n");
								ret = send(i, server_msg, sizeof(subscriber_message), 0);
								if(ret < 0) {
									exit_error("[!] Send error in server...\n");
									exit(1);
								}
							}
							else {
								subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
								strcpy(server_msg->message, "Successfully subscribed topic\n");
								ret = send(i, server_msg, sizeof(subscriber_message), 0);
								if(ret < 0) {
									exit_error("[!] Send error in server...\n");
									exit(1);
								}
							}
						}
						else if(client_msg.request_type == 's') {
							int check_topic = find_topic_name(tcp_client->topics, tcp_client->topics_nr, client_msg.request_topic.topic_name);
							
							//verifica daca clientul s-a mai abonat la acest topic
							if(check_topic != -1) {
								//verifica daca valoarea pentru SF este una diferita
								int old_sf = tcp_client->topics[check_topic].st;
								int new_sf = client_msg.request_topic.st;
								if(new_sf != old_sf) { //modifica valoarea pentru sf si trimite un mesaj sugestiv catre client
									tcp_client->topics[check_topic].st = client_msg.request_topic.st;
									subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						
									strcpy(server_msg->topic_name, tcp_client->topics[check_topic].topic_name);
									strcpy(server_msg->message, "SF option updated successfully\n");
									
									ret = send(i, server_msg, sizeof(subscriber_message), 0);
									if(ret < 0) {
										exit_error("[!] Send error in server...\n");
										exit(1);
									}
									free(server_msg);
								}
								if(new_sf == old_sf) { //cerere de subscribe invalida
									subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
						
									strcpy(server_msg->topic_name, tcp_client->topics[check_topic].topic_name);
									strcpy(server_msg->message, "[!] Subscribe error...You have already subscribed to this topic...\n");
									
									ret = send(i, server_msg, sizeof(subscriber_message), 0);
									if(ret < 0) {
										exit_error("[!] Send error in server...\n");
										exit(1);
									}
									free(server_msg);
								}
							}

							else if(check_topic == -1) {
								//adauga topicul in lista de topicuri la care clientul este abonat
								topic new_topic;
								strcpy(new_topic.topic_name, client_msg.request_topic.topic_name);
								new_topic.st = client_msg.request_topic.st;
								tcp_client->topics = (topic *)realloc(tcp_client->topics, (tcp_client->topics_nr + 2) * sizeof(topic));
								tcp_client->topics[tcp_client->topics_nr++] = new_topic;
								
								subscriber_message *server_msg = (subscriber_message *)calloc(1, sizeof(subscriber_message));
								//strcpy(server_msg->topic_name, tcp_client->topics[check_topic].topic_name);
								strcpy(server_msg->message, "Successfully subscribed topic\n");
									
								ret = send(i, server_msg, sizeof(subscriber_message), 0);
								if(ret < 0) {
									exit_error("[!] Send error in server...\n");
									exit(1);
								}
								free(server_msg);
							}
						}		
					}
				}
			}
		}
	}

	return 0;
}