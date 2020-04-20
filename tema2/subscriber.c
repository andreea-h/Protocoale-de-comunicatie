#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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

int exit_error(char *string_error) {
	printf("%s", string_error);
	perror(string_error);
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
	}

	FD_SET(sock_fd, &read_fds);
    fdmax = sock_fd;

    //introduce fd '0' in 'read_fds' pentru a permite citirea de la tastatura
    FD_SET(0, &read_fds);

    int port_nr = atoi(argv[3]);
    if(port_nr == 0) {
    	exit_error("[!] Data converting error...\n");
    }
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port_nr);
	int ret = inet_aton(argv[2], &sock_addr.sin_addr);

	if(ret == 0) {
		exit_error("[!] Data converting error...\n");
	}

	//printf("adresa ip: %d\n", sock_addr.sin_addr.s_addr);

	val = connect(sock_fd, (struct sockaddr*) &sock_addr, sizeof(sock_addr));
	//imediat ce subscriber-ul s-a conectat la server, se trimite catre server si Client_ID aferent acestuia
	if(val < 0) {
		exit_error("[!] Connect error for client...\n");
	}

	ret = send(sock_fd, argv[1], strlen(argv[1]), 0);
	if(ret < 0) {
		exit_error("[!] Send error for client...\n");
	}

//	printf("port: %hu\n", sock_addr.sin_port);

	while(1) {
		tmp_fds = read_fds;
		val = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if(val < 0) {
			exit_error("[!] Connection error in client...\n");
		}

		if (FD_ISSET(0, &tmp_fds)) { 
	        
	        //se va citi de la tastatura mesajul pe care subscriber doreste sa il trimita catre server
	        memset(buffer, 0, BUFLEN);

			fgets(buffer, BUFLEN - 1, stdin);
	        
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			//verifica daca comanda pe care o trimite clientul catre server respecta formatul enuntat
			char *buff_temp = (char *)malloc(BUFLEN * sizeof(char));
			strcpy(buff_temp, buffer);
			int nr_params = 1;
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
			//extrage topicul
			strcpy(request_topic.topic_name, token);

			if(request.request_type == 's') {
				//extrage valoarea pentru sf
				token = strtok(NULL, " ");
				
				request_topic.st = atoi(token);
			}
			else { //comanda pentru unsubscribe are setata valoarea pentru st egala cu -1
				request_topic.st = -1;
				//ultimul caracter din nume_topic pentru input corect, in cazul unsubscribe este '\n'
				//este inlocuit ultimul caracter din topic cu '\0'
				request_topic.topic_name[strlen(request_topic.topic_name) - 1] = '\0';
			}
			
			
			strcpy(request.client_id, argv[1]);
			request.request_topic = request_topic;

			// se trimite mesaj la server
			int sending1 = send(sock_fd, &request, sizeof(request), 0);
			if(sending1 < 0) {
				exit_error("[!] Send error for client...\n");
			}


			//printf("ai introdus: %s\n", buffer);
		}
		else {
			memset(buffer, 0, BUFLEN);
            int val = recv(sock_fd, buffer, BUFLEN, 0);
            if(val < 0) {
				exit_error("[!] Receiving error for client...\n");
			}
            printf("mesaj primit de la server: %s\n", buffer);
            //trateaza situatia in care clientul incearca sa se conecteze cu un id_client deja existent
            /*if(strlen(buffer) >=4 && strncmp(buffer, "exit", 4) == 0){
            	break;
            }*/
            if(val == 0) {
            	printf("Serverul a inchis conexiunea!\n");
            	break;
            }
            if(strlen(buffer) >=8 && strncmp(buffer, "Problema", 8) == 0){
            	break;
            }
		}
	}
	
	close(sock_fd);

	return 0;
}