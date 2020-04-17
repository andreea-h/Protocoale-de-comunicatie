#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

//structura care descrie un topic
typedef struct topic {
	char topic[51];
	int st;
} topic;

//structura care descrie un client
typedef struct client {
	char id_client[11]; //id unic asocit pentru fiecare client
	//vector de structuri care retine topicurile la care este abonat clientul
	topic *topics; 
} client;

//mesaj trimis de la server catre un client TCP
typedef struct tcp_serv_message {
	char topic[51];
	char id_client[11];
	unsigned int data_type; //specifica tipul de date
	char msg_value[1501]; //valoarea mesajului
} tcp_serv_message;


//mesaj trimis de la clientul TCP catre server (subscribe/unsubscribe)
typedef struct tcp_cli_message {
	char topic[51]; //topicul pentru care un anumit client da subscribe/unsubscribe
	int sf; //0 sau 1

} tcp_cli_message;