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
	char topic_name[50];
	int st; //1 sau 0
} __attribute__((packed))topic;

//structura care reprezinta un mesaj trimis de catre clientul TCP catre server
//(mesaj de tipul subscribe/unsubscribe)
typedef struct client_request {
	char request_type; //'u' pentru unsubscribe si 's' pentru subscribe
	topic request_topic;
	char client_id[11];//id_clientului care a facut cererea
} __attribute__((packed))client_request;

//structura care descrie un client
typedef struct client {
	char id_client[11]; //id unic asocit pentru fiecare client
	topic *topics; //vector de structuri care retine topicurile la care este abonat clientul
	int topics_nr; //numarul de topicuri la care este abonat clientul
	int connected; //1 - client online, 0 - client deconectat
	int socket;
	///TODO: buffer care retine topicurile pentru care este activat SF
} __attribute__((packed))client;

//structura care descrie un mesaj trimis de la server catre un client TCP
typedef struct subscriber_message {
	char topic_name[51];
	char data_type[11]; //specifica tipul de date
	char message[1501]; //valoarea mesajului
	char ip_source[16]; //ip-ul clientului udp care a trimis un mesaj catre server
	unsigned short client_port; //portul clientului udp care a trimis mesajul
} __attribute__((packed))subscriber_message;

//structura care descrie un mesaj pe care clientul UDP il trimite serverului
typedef struct publisher_message {
	char topic_name[50];
	uint8_t data_type; //specifica tipul de date
	char message[1501]; //valoarea mesajului
} __attribute__((packed)) publisher_message;

//structura folosita pentru a trimite de la server catre clientii tcp feedback 
//pentru comenzile 'subscribe', 'unsubscribe' si pentru conectarea unui client nou
typedef struct server_response {
	int ok; //1 - cererea clientului a fost prelucrata cu succes la nivelul serverului, 0 - input-ul dat de client este eronat
	char error_message[60]; //mesajul de eroare trimis de catre server
} __attribute__((packed))server_response;

