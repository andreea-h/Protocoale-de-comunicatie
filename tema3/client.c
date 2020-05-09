#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <stdbool.h>
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
	char *auth_token; //token jwt obtinut la enter_library, adaugat in headerul 'Authorization
	bool cookie_set; //semnaleaza daca a s-a primit cookie-ul de sesiune
	bool token_set; //semnaleaza daca s-a primit un token JWT
} client_info;

//afiseaza array-ul json alcatuit din carti
void print_json_array(JSON_Array *books, size_t nr_books) {
	size_t index;
	for(index = 0; index < nr_books; index++) {
		JSON_Object *json_obj = json_array_get_object(books, index);
		double value1 = json_object_dotget_number(json_obj, "id");
		const char *value2 = json_object_dotget_string(json_obj, "title");
		printf("--> Book Id - %d; Title: %s\n", (int)value1, value2);
	}
}

//afiseaza un mesajul de eroare trimis de catre server daca acesta exista
//intoarce 1 daca serverul intoarce un mesaj de eroare
//intoarce 0 daca din raspunsul serverului se poate extrage token-ul jwt

//context este un string care arata contextul in care poate sa apara un mesaj de eroare de la server
int parse_error_msg(JSON_Object *json_obj, const char *field, client_info *client, char *context) {
    //extrage valoarea asociata campului 'field' din data response 
    const char *message = json_object_dotget_string(json_obj, field);
    if(strcmp(field, "error") == 0 && message != NULL) { //serverul a intors un mesaj de eroare
    	if(strcmp(context, "get_books") != 0) {
    		printf("******************************************************************\n");
    		printf("[!] Error message from server: %s\n", message);
    		printf("******************************************************************\n");
    	}
    	else if(strcmp(context, "get_books") == 0) {
    		printf("*************************************************************************\n");
    		printf("[!] Error message from server: You are not allowed to access the library!\n");
    		printf("*************************************************************************\n");
    	}
    	return -1;
   	}
    else if(strcmp(field, "token") == 0) {
    	//se memoreaza authorizarion_token-ul
   		client->auth_token = (char *)calloc(MAX_LEN, sizeof(char));
    	memcpy(client->auth_token, message, strlen(message));
    	client->token_set = true;
    	return 0;
    }
    return 1;
}

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
        printf("[!] Connect error.");
        exit(1);
    }

    char *data = (char *)malloc(BUFFLEN * sizeof(char));
    sprintf(data, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
    
    message = compute_post_request(ADDR, user_register, content_type, &data, 1, NULL, 0, NULL);
    send_to_server(sockfd, message); //trimite cerere de autentificare la server
    response = receive_from_server(sockfd);
   // printf("\n%s\n\n", response); //afiseaza raspunsul dat de server
    //verifica daca serverul a trimis un mesaj de eroare
    //(user folosit de catre altcineva)
    JSON_Value *server_response;
    server_response = json_parse_string(strchr(response,'{'));

    //se creeaza obiectul JSON
    JSON_Object *json_obj = json_object(server_response); 
    int server_response_count = json_object_get_count(json_obj);
    if(server_response_count != 0) {
    	parse_error_msg(json_obj, "error", client, "register"); 
    	close_connection(sockfd);
    	return -1; 
    }
    else {
    	//cerere de autentificare realizata cu succes
    	//sunt memorate datele user-ului
    	printf("************************************************************************\n");
    	printf("Successful registering. Welcome, %s! Please enter next command.\n", username);
    	printf("************************************************************************\n");
		close_connection(sockfd);
		return 0;
    }
}

//Cerere de autentificare
//Se memoreaza cookie-ul de sesiune pe care il ofera serverul
int login(client_info *client, bool *login_flag) {
	if(*login_flag == true) { //este logat intr-un cont
		return -1;
	}
	char username[BUFFLEN], password[BUFFLEN];
	printf("username=");
	fgets(username, BUFFLEN, stdin);
	username[strlen(username) - 1] = '\0';

	printf("password=");
	fgets(password, BUFFLEN, stdin);
	password[strlen(password) - 1] = '\0';
	char *login = "/api/v1/tema/auth/login";
	char *content_type = "application/json";

    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *data = (char *)calloc(BUFFLEN, sizeof(char));
    sprintf(data, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
   
    char *message = compute_post_request(ADDR, login, content_type, &data, 1, NULL, 0, NULL);
   
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

   // printf("\n%s\n\n", response); //raspunsul complet dat de server

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));
    JSON_Object *json_obj = json_object(server_response);
    int serv_response_count = json_object_get_count(json_obj);

    if(serv_response_count != 0){
    	parse_error_msg(json_obj, "error", client, "login"); //poate aparea o eroare de user, parola gresite
    	close_connection(sockfd);
    	return 1;
    }
    else {
    	//login efectuat cu succes, se extrage cookiul de sesiune
    	printf("*******************************************************\n");
    	printf("Successful authentification. Please enter next command.\n");
    	printf("*******************************************************\n");

    	client->username = (char *)calloc(BUFFLEN, sizeof(char));
		strcpy(client->username, username);
		client->password = (char *)calloc(BUFFLEN, sizeof(char));
		strcpy(client->password, password);

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
	    key = strtok(cookie, "=");
	    value = strtok(NULL, ";");  //extrage valoarea asociata cheii
	    key = strtok(key, ":");
	    key = strtok(NULL, ":");
	    key = key + 1; //sterg primul caractere (' ') aparut la extragerea cheii din mesaj
	    char *client_cookie = (char *)malloc(MAX_LEN * sizeof(char));
	    strcpy(client_cookie, key);
	    strcat(client_cookie, "=");
	    strcat(client_cookie, value);

	    client->cookie = (char *)calloc(MAX_LEN, sizeof(char));
	    memcpy(client->cookie, client_cookie, strlen(client_cookie)); 
	    client->cookie_set = true;
    }
    close_connection(sockfd);
    *login_flag = true;
    return 0;
}

//Cerere de acces in bibilioteca
void enter_library(client_info *client) {
    char *enter_library = "/api/v1/tema/library/access";
    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
    char *message = compute_get_request(ADDR, enter_library, NULL, &(client->cookie), 1, NULL);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    //printf("\n%s\n\n", response); //raspunsul complet dat de server

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));
    JSON_Object *json_obj = json_object(server_response);

    int serv_response_count = json_object_get_count(json_obj);

    if(serv_response_count != 0) {
    	//serverul a intors un mesaj de eroare
    	if(parse_error_msg(json_obj, "error", client, "enter_library") != -1) { 
    		//daca serverul nu a intors un mesaj de eroare
    		//se va pasa toke-ul JWT din raspunsul dat de server
    		parse_error_msg(json_obj, "token", client, "enter_library");
    		printf("**************************************************************************\n");
    		printf("Request for access to the library was approved. Please enter next command.\n");
    		printf("**************************************************************************\n");
    	}	
    }
    close_connection(sockfd);
}

//Cerere de informatii depre toate cartile
void get_books(client_info *client) {
    char *get_books = "/api/v1/tema/library/books";
    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
    //se adauga si Authorization token, obtint la logare (memorat in campul auth_token din client_info)
    char* message = compute_get_request(ADDR, get_books, NULL, &client->cookie, 1, client->auth_token);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
 
    const JSON_Value *server_response = json_parse_string(strchr(response,'{'));
    JSON_Object *json_obj = json_object(server_response);

    int serv_response_count = json_object_get_count(json_obj);
    if(serv_response_count != 0) {
    	if(parse_error_msg(json_obj, "error", client, "get_books") != -1) { //serverul nu a intors un mesaj de eroare
    		//afiseaza cartile din bibilioteca
    		printf("****************************************\n");
    		printf("Available books:\n");
    		server_response = json_parse_string(strchr(response,'['));
    		JSON_Array *books = json_value_get_array(server_response);
    		size_t nr_books = json_array_get_count(books);
    		print_json_array(books, nr_books);
    		printf("****************************************\n");
    	}
    }
    else {
    	printf("****************************************\n");
		printf("The library is empty.\n");
		printf("****************************************\n");
    }
    close_connection(sockfd);
}

//intorce 0 daca datele sunt incomplete sau nu respecta formatarea
//altfel, intoarce 1
int check_data(char *title, char *author, char *genre, int page_count, char *publisher) {
	if(page_count <= 0) {
		return 0;
	}
	if(strlen(title) == 0 || strlen(author) == 0 || strlen(genre) == 0 || strlen(publisher) == 0) {
		return 0;
	}
	return 1;
}

//intoarce false daca 'name' etse string valid (nu contine doar cifre)
bool check_valid_string(char *name) {
	int i;
	for(i = 0; i < strlen(name); i++) {
		char c = name[0];
		if(c > '9' || c < '0') { //caracterul nu este cifra
			return false;
		}
	}
	return true; //'name' contine doar cifre
}

void add_book(client_info *client) {
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

	if(check_data(title, author, genre, page_count, publisher) == 0 || check_valid_string(title) == true 
		|| check_valid_string(author) == true || check_valid_string(genre) == true 
		|| check_valid_string(publisher) == true){ 
  		printf("************************************************************\n");
  		printf("Wrong data format. Please check the information you entered.\n");
  		printf("************************************************************\n");
  		return;
  	}
  	else {
  		char *add_book = "/api/v1/tema/library/books";
	    int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

	    char *data = (char *)calloc(MAX_LEN, sizeof(char));
	   	sprintf(data, "{\"title\":\"%s\",\"author\":\"%s\", \"genre\":\"%s\", \"page_count\":%d, \"publisher\":\"%s\"}", 
	   		title, author, genre, page_count, publisher);

	   	char *content_type = "application/json";
	    char *message = compute_post_request(ADDR, add_book, content_type, &data, 1, NULL, 0, client->auth_token);
	   
	    send_to_server(sockfd, message);
	    char *response = receive_from_server(sockfd);
	    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

	    JSON_Object *json_obj = json_object(server_response);
	    int serv_response_count = json_object_get_count(json_obj);
	  	if(serv_response_count != 0) {
	  		parse_error_msg(json_obj, "error", client, "add_book");
	  	}
	  	//verifica daca informatiile introduse sunt incomplete sau nu respecta formatare
	  	else {
	  		printf("**********************************************\n");
	  		printf("Book \"%s\" successfully added to the library.\n", title);
	  		printf("**********************************************\n");
	  	}
	  	close_connection(sockfd);
	}
}

//Vizualizarea detaliilor despre o carte
void get_book(client_info *client) {
	char *book_id = (char *)calloc(BUFFLEN, sizeof(char));
	printf("id=");
	fgets(book_id, BUFFLEN, stdin);
	book_id[strlen(book_id) - 1] = '\0';

	char *get_book = (char *)calloc(BUFFLEN, sizeof(char));
	strcpy(get_book, "/api/v1/tema/library/books/");
	strcat(get_book, book_id);

	int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);

    char *message = compute_get_request(ADDR, get_book, NULL, &client->cookie, 1, client->auth_token);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);

    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

    JSON_Object *json_obj = json_object(server_response);
    int serv_response_count = json_object_get_count(json_obj);

    if(serv_response_count != 0) {
    	if(parse_error_msg(json_obj, "error", client, "get_book") != -1) { //serverul nu a intors un mesaj de eroare
    		//printeaza informatiile cerute despre carte
    		printf("****************************************\n");
			const char *title = json_object_dotget_string(json_obj, "title");
			const char *author = json_object_dotget_string(json_obj, "publisher");
			const char *genre = json_object_dotget_string(json_obj, "genre");
			double page_count = json_object_dotget_number(json_obj, "page_count");
			printf("Title: %s\n", title);
			printf("Author: %s\n", author);
			printf("Genre: %s\n", genre);
			printf("Page-cout: %d\n", (int)page_count);	
    		printf("****************************************\n");
    	}
    }
    
    close_connection(sockfd);
}

//Stergerea unei carti
void delete_book(client_info *client) {
	printf("id=");
	char *book_id = (char *)calloc(BUFFLEN, sizeof(char));

	fgets(book_id, BUFFLEN, stdin);
	book_id[strlen(book_id) - 1] = '\0';

	//verifica daca id-ul introdus are format valid
	int check_id = atoi(book_id);
	if(check_id == 0) { //format invalid
		printf("****************************************\n");
		printf("[!] Invalid Book Id.\n");
		printf("****************************************\n");
	}
	else {
		char *delete_book = (char *)calloc(BUFFLEN, sizeof(char));
		strcpy(delete_book, "/api/v1/tema/library/books/");
		strcat(delete_book, book_id);

		int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
	    char *message = compute_delete_request(ADDR, delete_book, NULL, &client->cookie, 1, client->auth_token);
	    send_to_server(sockfd, message);
	    char *response = receive_from_server(sockfd);

	    JSON_Value *server_response = json_parse_string(strchr(response,'{'));

	    JSON_Object *json_obj = json_object(server_response);
	    int serv_response_count = json_object_get_count(json_obj);
	    if(serv_response_count != 0) {
	    	parse_error_msg(json_obj, "error", client, "delete_book");
	    }
	    else {
	    	printf("****************************************\n");
	    	printf("Book with Id %s successfully deleted.\n", book_id);
	    	printf("****************************************\n");
	    }
	    close_connection(sockfd);
	}
}

//Logout
//conditia ca un user sa fiu logat este validata prin raspunsul dat de server
void logout(client_info *client, bool *login_flag) {
	char *logout = "/api/v1/tema/auth/logout";
	int sockfd = open_connection(ADDR, 8080, AF_INET, SOCK_STREAM, 0);
	//cookie-ul de sesiune este adaugat in cerere
	char *message = compute_get_request(ADDR, logout, NULL, &client->cookie, 1, NULL);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    
    JSON_Value *server_response = json_parse_string(strchr(response,'{'));
    JSON_Object *json_obj = json_object(server_response);

    int serv_response_count = json_object_get_count(json_obj);  
    if(serv_response_count != 0) {
    	parse_error_msg(json_obj, "error", client, "logout");
    }
    else {
    	//logout efectuat cu succes
    	printf("*********************************************\n");
    	printf("Successful logout. Bye-bye, %s!\n", client->username);
    	printf("*********************************************\n");
    	*login_flag = false;
    	memset(client->username, 0, BUFFLEN);
    	memset(client->password, 0, BUFFLEN);
    	if(client->cookie_set == true) { //s-a setat un cookie la un moment dat
    		memset(client->cookie, 0, MAX_LEN);
    		client->cookie_set = false;
    	}
    	if(client->token_set == true) { //s-a setat un auth_token la un moment dat
    		memset(client->auth_token, 0, MAX_LEN);
    		client->token_set = false;
    	}
    }
    close_connection(sockfd);
}

int main(int argc, char *argv[]) {	
	char command[BUFFLEN];

	client_info *my_client = (client_info *)malloc(sizeof(client_info));
	my_client->cookie_set = false;
	my_client->token_set = false;
	bool login_flag = false;
	while(1) {
		fgets(command, BUFFLEN, stdin);
		if(strcmp(command, "register\n") == 0) {
			user_register(my_client);
		}
		else if(strcmp(command, "login\n") == 0) {
			int rez = login(my_client, &login_flag);
			if(rez == -1) {
				printf("**************************************************************************\n");
				printf("[!] Error message: You are already logged in with username \"%s\"\n", my_client->username);
				printf("**************************************************************************\n");
			}
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
			logout(my_client, &login_flag);
		}
		else if(strcmp(command, "exit\n") == 0) { //se iese din program
			return 0;
		}
		else {
			printf("[!] This command in not allowed\n");
		}

	}

    

    return 0;
}
