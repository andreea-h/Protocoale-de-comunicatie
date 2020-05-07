#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define MAX_LEN 3000
#define BUFFLEN 1500
#define ADDR "3.8.116.10"

//structura care retine informatii despre client in cadrul aplicatiei
typedef struct client_info {
	char *username;
	char *password;
	char *cookie; //cookie de sesiune
	char *auth_token; //token jwt obtinut la enter_library, adaugat in headerul 'Authorization'
} client_info;

//Cerere de autentificare catre server
int user_register(client_info *client) {
	char username[BUFFLEN], password[BUFFLEN];
	printf("username=");
	fgets(username, BUFFLEN, stdin);
	username[strlen(username) - 1] = '\0';

	printf("password=");
	fgets(password, BUFFLEN, stdin);
	password[strlen(password) - 1] = '\0';

	char *message;
    char *response;
    int sockfd;

    char *user_register = "/api/v1/tema/auth/register";
    char *content_type = "application/json";

    sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("[!]Eroare de conectare la server.");
        exit(1);
    }

    char *data = (char *)malloc(BUFFLEN * sizeof(char));
    sprintf(data, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    
    message = compute_post_request(ADDR, user_register, content_type, &data, 1, NULL, 0, NULL);
    send_to_server(sockfd, message); //trimite cerere de autentificare la server
    response = receive_from_server(sockfd);
    printf("%s\n\n", response); //afiseaza raspunsul dat de server
    //verifica daca serverul a trimis un mesaj de eroare
    //(user folosit de catre altcineva)
    JSON_Value *server_response;
    server_response = json_parse_string(strchr(response,'{'));

    //se creeaza obiectul JSON
    JSON_Object *json_obj = json_object(server_response); 
    int server_response_count = json_object_get_count(json_obj);
    if(server_response_count != 0) {

    	const char *field = "error";
    	//extrage valoarea asociata campului de eroare
    	const char *error_message = json_object_dotget_string(json_obj, field);
    	printf("[!] Mesaj de eroare de la server: %s\n\n", error_message);
    	close_connection(sockfd);
    	return -1; 
    }
    else {
    	//cerere de autentificare realizata cu succes
    	client->username = (char *)calloc(BUFFLEN, sizeof(char));
		strcpy(client->username, username);
		client->password = (char *)calloc(BUFFLEN, sizeof(char));
		strcpy(client->password, password);
		close_connection(sockfd);
		return 0;
    }
    
    
}

//Cerere de autentificare
//Se memoreaza cookie-ul de sesiune pe care il ofera serverul
void login(client_info *client) {
	char username[BUFFLEN], password[BUFFLEN];
	printf("username=");
	fgets(username, BUFFLEN, stdin);
	username[strlen(username) - 1] = '\0';

	client->username = (char *)calloc(BUFFLEN, sizeof(char));
	strcpy(client->username, username);

	printf("password=");
	fgets(password, BUFFLEN, stdin);
	password[strlen(password) - 1] = '\0';
	char *login = "/api/v1/tema/auth/login";
	char *content_type = "application/json";

    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *data = (char *)calloc(BUFFLEN, sizeof(char));
    sprintf(data, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
   
    char *message = compute_post_request(ADDR, login, content_type, &data, 1, NULL, 0, NULL);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);


    char *cookie = (char *)malloc(500 * sizeof(char));
    //extrage din raspunsul serverului cookie-ul
    char *key = (char *)malloc(100 * sizeof(char));
    char *value = (char *)malloc(400 * sizeof(char));
    
    char *sub_string = (char *)malloc(BUFLEN * sizeof(char));
    sub_string = strtok(response, "\n");
    while(sub_string != NULL) {
        if(strstr(sub_string, "Set-Cookie") != NULL) {
            strcpy(cookie, sub_string);
            break;
        }
        sub_string = strtok(NULL, "\n");
    }
    printf("COOKIE: %s\n", cookie);
    key = strtok(cookie, "=");

    value = strtok(NULL, ";");  //extrage valoarea asociata cheii
    key = strtok(key, ":");
    key = strtok(NULL, ":");
    key = key + 1; //sterg primul caractere (' ') aparut la extragerea cheii din mesaj
    printf("Cheie:%s\n", key);
    printf("Valoare:%s\n", value);

    char *client_cookie = (char *)malloc(400 * sizeof(char));
    strcpy(client_cookie, key);
    strcat(client_cookie, "=");
    strcat(client_cookie, value);

    client->cookie = (char *)calloc(MAX_LEN, sizeof(char));
    memcpy(client->cookie, client_cookie, strlen(client_cookie));
}

//Cerere de acces in bibilioteca
void enter_library(client_info *client) {

    char *enter_library = "/api/v1/tema/library/access";
    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
    char *message = compute_get_request(ADDR, enter_library, NULL, &(client->cookie), 1, NULL);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

    const char *field = "token";
    const char *auth_token = json_object_dotget_string(json_obj, field);
    printf("token: %s\n", auth_token);

    client->auth_token = (char *)calloc(MAX_LEN, sizeof(char));
    memcpy(client->auth_token, auth_token, strlen(auth_token));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

//Cerere de informatii depre toate cartile
void get_books(client_info *client) {
	printf("---------------------------------------------\n\n");
    //Cerere de informatii sumare despre toate cartile
    char *get_books = "/api/v1/tema/library/books";

    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
    //se adauga si Authorization token, obtint la logare (memorat in campul auth_token)
    char* message = compute_get_request(ADDR, get_books, NULL, &client->cookie, 1, client->auth_token);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

   	printf("token: %s\n", json_object_dotget_string(json_obj, "error"));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

void add_book(client_info *client) {
	printf("---------------------------------------------\n\n");
	char *title = (char *)calloc(BUFFLEN, sizeof(char));
	char *author = (char *)calloc(BUFFLEN, sizeof(char));
	char *genre = (char *)calloc(BUFFLEN, sizeof(char));
	char *page_count_string = (char *)calloc(BUFFLEN, sizeof(char));
	int page_count;
	char *publisher = (char *)calloc(BUFFLEN, sizeof(char));

	//ofera clientului prompt pentru introducerea campurilor aferente cartii
	printf("title=");
	fgets(title, BUFFLEN, stdin);
	title[strlen(title) - 1] = '\0';

	printf("author=");
	fgets(author, BUFFLEN, stdin);
	author[strlen(author) - 1] = '\0';

	printf("genre=");
	fgets(genre, BUFFLEN, stdin);
	genre[strlen(genre) - 1] = '\0';

	printf("page_count=");
	fgets(page_count_string, BUFFLEN, stdin);
	page_count_string[strlen(page_count_string) - 1] = '\0';
	page_count = atoi(page_count_string);

	printf("publisher=");
	fgets(publisher, BUFFLEN, stdin);
	publisher[strlen(publisher) - 1] = '\0';

    //Cerere de informatii sumare despre toate cartile
    char *add_book = "/api/v1/tema/library/books";

    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *data = (char *)calloc(MAX_LEN, sizeof(char));
   	sprintf(data, "{\"title\":\"%s\",\"author\":\"%s\", \"genre\":\"%s\", \"page_count\":%d, \"publisher\":\"%s\"}", 
   		title, author, genre, page_count, publisher);

   	printf("%s\n", data);

   	char *content_type = "application/json";
    char *message = compute_post_request(ADDR, add_book, content_type, &data, 1, NULL, 0, client->auth_token);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

   	printf("token: %s\n", json_object_dotget_string(json_obj, "error"));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

//Vizualizarea delatiilor despre o carte
void get_book(client_info *client) {
	char *book_id = (char *)calloc(BUFFLEN, sizeof(char));
	printf("id=");
	fgets(book_id, BUFFLEN, stdin);
	book_id[strlen(book_id) - 1] = '\0';
	printf("%s\n", book_id);

	char *get_book = (char *)calloc(BUFFLEN, sizeof(char));
	strcpy(get_book, "/api/v1/tema/library/books/");
	strcat(get_book, book_id);

	printf("calea: %s\n", get_book);
	int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *message = compute_get_request(ADDR, get_book, NULL, &client->cookie, 1, client->auth_token);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

   	printf("token: %s\n", json_object_dotget_string(json_obj, "error"));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

//Stergerea unei carti
void delete_book(client_info *client) {
	printf("id=");
	char *book_id = (char *)calloc(BUFFLEN, sizeof(char));

	fgets(book_id, BUFFLEN, stdin);
	book_id[strlen(book_id) - 1] = '\0';

	char *delete_book = (char *)calloc(BUFFLEN, sizeof(char));
	strcpy(delete_book, "/api/v1/tema/library/books/");
	strcat(delete_book, book_id);

	int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *message = compute_delete_request(ADDR, delete_book, NULL, &client->cookie, 1, client->auth_token);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

   	printf("token: %s\n", json_object_dotget_string(json_obj, "error"));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

//Logout
void logout(client_info *client) {
	char *logout = "/api/v1/tema/auth/logout";

	int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
	//cookie-ul de sesiune este adaugat in cerere
	char *message = compute_get_request(ADDR, logout, NULL, &client->cookie, 1, NULL);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    printf("*******de la server: %ld\n",  json_object_get_count(json_obj));

   	printf("token: %s\n", json_object_dotget_string(json_obj, "error"));

    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
}

int main(int argc, char *argv[]) {	
	char command[BUFFLEN];

	client_info *my_client = (client_info *)malloc(sizeof(client_info));
	while(1) {
		fgets(command, BUFFLEN, stdin);
		if(strcmp(command, "register\n") == 0) {
			user_register(my_client);
		}
		else if(strcmp(command, "login\n") == 0) {
			login(my_client);
		}
		else if(strcmp(command, "enter_library\n") == 0) {
			enter_library(my_client);
		}
		else if(strcmp(command, "get_books\n") == 0) {
			get_books(my_client); //cere informatii despre toate cartile
		}
		else if(strcmp(command, "get_book\n") == 0) {
			get_book(my_client); //cere informatii depre o carte anume
		}
		else if(strcmp(command, "add_book\n") == 0) {
			add_book(my_client);
		}
		else if(strcmp(command, "delete_book\n") == 0) {
			delete_book(my_client);
		}
		else if(strcmp(command, "logout\n") == 0) {
			logout(my_client);
		}
		else if(strcmp(command, "exit\n") == 0) { //se iese din program
			return 0;
		}
	}

    

    return 0;
}
