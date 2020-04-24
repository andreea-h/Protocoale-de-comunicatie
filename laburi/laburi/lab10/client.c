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

int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
    
    // Ex 1.1: GET dummy from main server
    char *dummy = "/api/v1/dummy";
    char *addr = "3.8.116.10";
    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(addr, dummy, NULL, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);

    memset(message, 0, strlen(message));
    memset(response, 0, strlen(response));
    // Ex 1.2: POST dummy and print response from main server
    printf("\n**************************************\n");
    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    char *mesg = "ceva_mesajfvbgnhmj,..ikjblalalalala;dckfdnv";
    char *form = "application/x-www-form-urlencoded";
    message = compute_post_request(addr, dummy, form, &mesg, 1, NULL, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);

    // Ex 2: Login into main server
    printf("\n**************************************\n");
    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    char *send_data = "username=student&password=student";
    char *dummy_login = "/api/v1/auth/login";

   
    message = compute_post_request(addr, dummy_login, form, &send_data, 1, NULL, 0);
    printf("%s\n", message);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);

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
   
    close_connection(sockfd);

    // Ex 3: GET weather key from main server
    // Ex 4: GET weather data from OpenWeather API
    // Ex 5: POST weather data for verification to main server
    // Ex 6: Logout from main server
    printf("\n**************************************\n");
    memset(message, 0, strlen(message));
    memset(response, 0, strlen(response));
    char *dummy_weather = "/api/v1/weather/key";
    char *new_addr = "188.166.16.132";
    int new_sockfd = open_connection(new_addr, 80, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(new_addr, dummy_weather, NULL, &cookie, 1);

    send_to_server(new_sockfd, message);
    response = receive_from_server(new_sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(new_sockfd);

    printf("\n**************************************\n");
    memset(message, 0, strlen(message));
    memset(response, 0, strlen(response));
    char *dummy_logout = "/api/v1/auth/logout";
    
    sockfd = open_connection(addr, 8080, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(addr, dummy_logout, NULL, &cookie, 1);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    printf("SERVER_RESPONSE:\n%s\n", response);
    close_connection(sockfd);

    // BONUS: make the main server return "Already logged in!"
    // free the allocated data at the end!    
    return 0;
}
