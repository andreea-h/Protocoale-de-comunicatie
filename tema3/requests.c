#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"



char *compute_delete_request(char *host, char *url, char *query_params,char **cookies, int cookies_count, char *authorization)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    if (query_params != NULL) {
        sprintf(line, "DELETE %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "DELETE %s HTTP/1.1", url);
    }

    compute_message(message, line);
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (cookies_count != 0) {
       char *buffer = (char *)calloc(BUFLEN, sizeof(char));
       int i;
       for(i = 0; i < cookies_count; i++) {
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "Cookie: %s", cookies[i]);
            compute_message(message, buffer);
       }
    }
    //daca este nevoie, este adaugat headerul Authorization
    if (authorization != NULL) {
        memset(line, 0, BUFLEN);
        sprintf(line, "Authorization: Bearer %s", authorization);
        compute_message(message, line);
    }   
    compute_message(message, "");
    return message;
}



char *compute_get_request(char *host, char *url, char *query_params,char **cookies, int cookies_count, char *authorization)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    if (query_params != NULL) {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    } else {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    if (cookies_count != 0) {
       char *buffer = (char *)calloc(BUFLEN, sizeof(char));
       int i;
       for(i = 0; i < cookies_count; i++) {
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "Cookie: %s", cookies[i]);
            compute_message(message, buffer);
       }
    }
    //daca este nevoie, este adaugat headerul Authorization
    if (authorization != NULL) {
        memset(line, 0, BUFLEN);
        sprintf(line, "Authorization: Bearer %s", authorization);
        compute_message(message, line);
    }   
    compute_message(message, "");
    return message;
}

//auth este token ul authorization
char *compute_post_request(char *host, char *url, char* content_type, char **body_data,
                            int body_data_fields_count, char **cookies, int cookies_count, char *authorization)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);
    
    sprintf(line, "Host: %s", host);
    compute_message(message, line);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);


    sprintf(line, "Content-Length: %ld", strlen(*body_data));
    compute_message(message, line);
   
    if (cookies_count != 0) {
    	char *buffer = (char *)calloc(BUFLEN, sizeof(char));
        int i;

        for(i = 0; i < cookies_count; i++) {
            memset(buffer, 0, BUFLEN);
            sprintf(buffer, "Cookie: %s", cookies[i]);
            compute_message(message, buffer);
        }
    	
        free(buffer);
    }

    if (authorization != NULL) {
        sprintf(line, "Authorization: Bearer %s", authorization);
        compute_message(message, line);
    }   
   	sprintf(line, "%s", "");
    compute_message(message, line);

    memset(line, 0, LINELEN);

   
    char *buffer = (char *)calloc(BUFLEN, sizeof(char));
    int i;
    for(i = 0; i < body_data_fields_count; i++) {
        memset(buffer, 0, BUFLEN);
        sprintf(buffer, "%s", body_data[i]);
        compute_message(message, buffer);
    }

    return message;
}
