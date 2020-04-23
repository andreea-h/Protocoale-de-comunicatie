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

void exit_error(char *string_error) {
	printf("%s", string_error);
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
	if(sock_fd < 0) {
		exit_error("[!] Socket open error...\n");
		exit(1);
	}

	FD_SET(sock_fd, &read_fds);
    fdmax = sock_fd;

    //introduce fd '0' in 'read_fds' pentru a permite citirea de la tastatura
    FD_SET(0, &read_fds);

    int port_nr = atoi(argv[3]);
    if(port_nr == 0) {
    	exit_error("[!] Data converting error...\n");
    	exit(1);
    }
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port_nr);
	int ret = inet_aton(argv[2], &sock_addr.sin_addr);

	if(ret == 0) {
		exit_error("[!] Data converting error...\n");
		exit(1);
	}

	val = connect(sock_fd, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
	//imediat ce subscriber-ul s-a conectat la server, se trimite catre server si Client_ID aferent acestuia
	if(val < 0) {
		exit_error("[!] Connect error for client...\n");
		exit(1);
	}

	//subscriber-ul trimite catre server, imediat dupa stabilirea conexiunii, client_id-ul cu care a fost pornit
	ret = send(sock_fd, argv[1], strlen(argv[1]), 0);
	if(ret < 0) {
		exit_error("[!] Send error for client...\n");
		exit(1);
	}

	//dezactiveaza algoritmul lui Nagle
	int check = 1;
	setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&check, sizeof(int)); 

	subscriber_message *client_msg = (subscriber_message *)malloc(1 * sizeof(subscriber_message));
	val = recv(sock_fd, client_msg, sizeof(subscriber_message), 0);

    //verifica daca mesajul pe care il trimite serverul ca raspuns la cererea de conectare semnaleaza o eroare 
    //in ceea ce priveste inputul la nivelul clientului
    char *check_msg = (char *)malloc(100 * sizeof(char));
    memcpy(check_msg, client_msg->message, 100);
    if(strcmp(check_msg, "[!] This Client_ID is already in use! Try to use another Client_ID!\n") == 0) {
        exit_error("[!] This Client_ID is already in use! Try to use another Client_ID!\n");
		close(sock_fd);
		return 0;
    } 
    //verifica daca mesajul trimis de server nu indica faptul ca clientul are de primit mesaje 
    // stocate cat timp acesta era deconectat
    if(strcmp(check_msg, "There are messages that need to be forwarded?\n") == 0) {
    	printf("aici\n");
    }    

	while(1) {
		tmp_fds = read_fds;
		val = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(val < 0) {
			exit_error("[!] Connection error in client...\n");
			exit(1);
		}

		if (FD_ISSET(0, &tmp_fds)) { 
	        
	        //se va citi de la tastatura mesajul pe care subscriber doreste sa il trimita catre server
	        memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
	        
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			//verifica daca comanda pe care o trimite clientul catre server respecta formatul enuntat
			//in caz de input incorect, este afisat un mesaj de eroare adresat clientului
			char *buff_temp = (char *)malloc(BUFLEN * sizeof(char));
			strcpy(buff_temp, buffer);
			int nr_params = 1; //nr de parametri din comanda preluata de la stdin

			char *tmp_token = strtok(buff_temp, " ");
			printf("fst_token: %s\n", tmp_token);

			if(strcmp(tmp_token, "subscribe") != 0 && strcmp(tmp_token, "unsubscribe") != 0) {
				printf("[!] Wrong format. Correct usage: \"subscribe <topic_name> <SF>\" or  \"unsubscribe <topic_name>\" or \"exit\"\n");
				continue;
			}
			if(strcmp(tmp_token, "unsubscribe") == 0) {
				nr_params++;
			}
			while(tmp_token != NULL) {
				nr_params++;
				tmp_token = strtok(NULL, " ");
			}

			printf("params:%d\n", nr_params);
			if((nr_params - 1) != 3) {
				printf("[!] Wrong format. Correct usage: \"subscribe <topic_name> <SF>\" or  \"unsubscribe <topic_name>\" or \"exit\"\n");
				continue;
			}

			//daca comanda are formatul valid, se va trimite cererea aferenta catre server
			client_request request;
			topic request_topic;

			//extrage din 'buff' topicul, respectiv tipul de cerere trimisa (subscribe/unsubscribe)
			//si valoarea pentru sf (1/0)
			char *token = strtok(buffer, " ");
			if(token[0] == 's') {
				request.request_type = 's';
			}
			else if(token[0] == 'u') {
				request.request_type = 'u';
			}

			token = strtok(NULL, " ");
			//extrage numele topicului
			printf("token: %s\n", token);
			strcpy(request_topic.topic_name, token);

			if(request.request_type == 's') {
				//extrage valoarea pentru sf
				token = strtok(NULL, " ");
				if(strcmp(token, "0\n") == 0) {
					request_topic.st = 0;
				}
				else {
					request_topic.st = atoi(token);
					printf("sotpic sf: %d\n", request_topic.st);
					//verifica daca valoarea oferita la stdin pentru campul 'st' (in cazul comenzii 'subscribe') este cea corecta
					if(request_topic.st != 1) {
						exit_error("[!] Wrong SF value! Values accepted: 0 or 1...\n");
						continue;
					}
				}
			}
			else { //comanda pentru unsubscribe are setata valoarea pentru st ('store') egala cu -1
				
				request_topic.st = -1;
				//ultimul caracter din nume_topic pentru input corect, in cazul unsubscribe este '\n'
				//este inlocuit ultimul caracter din topic cu aracterul '\0'
				request_topic.topic_name[strlen(request_topic.topic_name) - 1] = '\0';
			}
			
			strcpy(request.client_id, argv[1]);
			printf("argv[1] %s\n", argv[1]);
			printf("id_client: %s\n", request.client_id);
			request.request_topic = request_topic;

			printf("client: %s\n", request.client_id);
			printf("topic_name: %s\n", request.request_topic.topic_name);

			// se trimite mesaj la server
			int sending1 = send(sock_fd, &request, sizeof(client_request), 0);
			if(sending1 < 0) {
				exit_error("[!] Send error for client...\n");
				continue;
			}
			
			//analizeaza raspunsul pe care il da serverul in aceasta situatie
			subscriber_message server_msg;
			ret = recv(sock_fd, &server_msg, sizeof(subscriber_message), 0);
			if(ret < 0) {
				exit_error("[!] Send error in client...\n");
				exit(1);
			}

			//verifica raspunsul de la server in functie de tipul de cerere care a fost trimisa
			if(request.request_type == 's') {
				if(strcmp(server_msg.message, "SF option updated successfully\n") == 0) {
					printf("SF option updated successfully\n");
					continue;
				}
				else if(strcmp(server_msg.message, "[!] Subscribe error...You have already subscribed to this topic...\n") == 0) {
					printf("[!] Subscribe error...You have already subscribed to this topic...\n");
					continue;
				}
			}
			if(request.request_type == 'u') {
				//verifica prin mesajul primit de la client daca s-a trimis o comanda valida de unsubscribe
				if(strcmp(server_msg.message, "[!] Invalid unsubscribe command! You are not subscribed to this topic!\n") == 0) {
					printf("[!] Invalid unsubscribe command! You are not subscribed to this topic!\n");
					continue;
				}
			}
	
			//afiseaza feedback pentru comanda tocmai trimisa catre server de catre client 
			//daca aceasta a fost acceptata cu succes de catre server
			if(request.request_type == 's') {
				printf("subscribed %s\n", request_topic.topic_name);
			}
			else if(request.request_type == 'u') {
				printf("unsubscribed %s\n", request_topic.topic_name);
			}
		}
		else {
            memset(buffer, 0, BUFLEN);

            //subscriberul primeste notificare de la server pentru unul dintre topicurile la care este abonat
            subscriber_message *client_msg = (subscriber_message *)malloc(1 * sizeof(subscriber_message));
            int val = recv(sock_fd, client_msg, sizeof(subscriber_message), 0);

            //verifica daca mesajul pe care il trimite serverul semnaleaza o eroare
            if(val < 0) {
				exit_error("[!] Receiving error for client...\n");
				continue;
			}

            if(val == 0) {
            	//printf("Serverul a inchis conexiunea!\n");
            	break;
            } 

            /*if(strlen(client_msg->message) == 0) {
				exit_error("[!] This Client_ID is already in use! Try to use another Client_ID!\n");
				close(sock_fd);
				return 0;
			}*/

			printf("%s:%hu - %s - %s - %s\n", client_msg->ip_source, client_msg->client_port, 
				client_msg->topic_name, client_msg->data_type, client_msg->message);
		}
	}
	
	close(sock_fd);

	return 0;
}