/*
 * Protocoale de comunicatii
 * Laborator 7 - TCP
 * Echo Server
 * client.c
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "helpers.h"

void run_client(int sockfd) {
    char buf[BUFLEN];
    memset(buf, 0, BUFLEN);

    int byte_count;
    int bytes_received;
    int number_of_client;

    recv(sockfd, buf, 2, 0);

    if(buf[0] == '1') {
        number_of_client = 1;
    }
    else {
        number_of_client = 2;
    }
    
    //clientul 1
    // citesc de la tastaura
    // trimit catre server
    // primesc de la server
    // afisez pe ecran
    if(number_of_client == 1) { 
        while (read(STDIN_FILENO, buf, BUFLEN - 1) > 0 && !isspace(buf[0])) {
            byte_count = strlen(buf) + 1;
            
            int bytes_send = 0;
            int bytes_remaining = byte_count;
            int bytes_received;

            // TODO 4: Trimiteti mesajul catre server prin sockfd
            while(bytes_remaining > 0){
                bytes_send = send(sockfd, buf, byte_count, 0);
                //numarul de bytes al mesajului trimis
                if(bytes_send == -1){
                    perror("error..send_client");
                    exit(1);
                }
                bytes_remaining -= bytes_send;
            }

            memset(buf, 0, BUFLEN);
            // TODO 5: Receptionati un mesaj venit de la server
            bytes_received = recv(sockfd, buf, BUFLEN, 0);

            fprintf(stderr, "Received from client nr. 2 %s", buf);

            memset(buf, 0, BUFLEN);  
        }
    }
    else {  //primesc de la server //pentru client 2-ordine inversa!
            //afisez pe ecran
            //citesc de la tastatura
            //trimit catre server
        do{

            // TODO 5: Receptionati un mesaj venit de la server
            bytes_received = recv(sockfd, buf, BUFLEN, 0);
            fprintf(stderr, "Received from client nr. 1: %s", buf);
            memset(buf, 0, BUFLEN);

            // TODO 4: Trimiteti mesajul catre server prin sockfd
            if(read(STDIN_FILENO, buf, BUFLEN - 1) == -1) {
                perror("receiving from 1\n");
                exit(1);
            }
            
            byte_count = strlen(buf) + 1;
            int bytes_send = 0;
            int bytes_remaining = byte_count;

            while(bytes_remaining > 0) {
                bytes_send = send(sockfd, buf, byte_count, 0);
                if(bytes_send == -1){
                    perror("error...send_client");
                    exit(1);
                }
                bytes_remaining -= bytes_send;
            }
        } while(bytes_received != 0);
    }   
}

int main(int argc, char* argv[])
{
    int sockfd = -1;
    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);
    memset(&serv_addr, 0, socket_len);

    if (argc != 3) {
        printf("\n Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    // TODO 1: Creati un socket TCP pentru conectarea la server
    sockfd = socket(PF_INET, SOCK_STREAM, 0);

    // TODO 2: Completati in serv_addr adresa serverului, familia de adrese si portul pentru conectare
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = INADDR_ANY; //this host

    // TODO 3: Creati conexiunea catre server
    connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr_in));

    run_client(sockfd);

    // TODO 6: Inchideti conexiunea si socketul creat
    shutdown(sockfd, 2);
    close(sockfd);

    return 0;
}
