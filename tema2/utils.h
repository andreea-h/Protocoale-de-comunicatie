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
	char topic_name[51];
	int st; //1 sau 0
} topic;

//structura care reprezinta un mesaj trimis de catre clientul TCP catre server
//(mesaj de tipul subscribe/unsubscribe)
typedef struct client_request {
	char request_type; //'u' pentru unsubscribe si 's' pentru subscribe
	topic request_topic;
	char client_id[11];//id_clientului care a facut cererea
} client_request;

//structura care descrie un client
typedef struct client {
	char id_client[11]; //id unic asocit pentru fiecare client
	topic *topics; //vector de structuri care retine topicurile la care este abonat clientul
	int topics_nr; //numarul de topicuri la care este abonat clientul
	int connected; //1 - client online, 0 - client deconectat
	int socket;
	///TODO: buffer care retine topicurile pentru care este activat SF
} client;

//mesaj trimis de la server catre un client TCP
typedef struct tcp_serv_message {
	
	char topic[51];
	char id_client[11];
	unsigned int data_type; //specifica tipul de date
	char msg_value[1501]; //valoarea mesajului
} tcp_serv_message;



