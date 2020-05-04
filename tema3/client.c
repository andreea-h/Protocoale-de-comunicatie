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

 

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    
    //Cerere de autentificare catre server
    char *addr = "3.8.116.10";
    char *user_register = "/api/v1/tema/auth/register";
    char *content_type = "application/json";

   
    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("[!]Eroare de conectare la server.");
        exit(1);
    }


    char *data = "{\"username\":\"aoleu\",\"password\":\"aoleu1\"}";
    message = compute_post_request(addr, user_register, content_type, &data, 1, NULL, 0);
    printf("Mesaj trimis catre server:\n%s\n", message);
 //   send_to_server(sockfd, message); //autentificarea
 //   response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);


    printf("---------------------------------------------\n\n");
    //Cerere de login
    char *login = "/api/v1/tema/auth/login";

    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    data = "{\"username\":\"aoleu\",\"password\":\"aoleu1\"}";
    message = compute_post_request(addr, login, content_type, &data, 1, NULL, 0);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
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


   

    printf("---------------------------------------------\n\n");
    //Cerere de acces in bibilioteca
    char *enter_library = "/api/v1/tema/library/access";

    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(addr, enter_library, NULL, &client_cookie, 1);
    printf("Mesaj trimis catre server:\n%s\n", message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);
    return 0;
}
