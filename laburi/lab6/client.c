/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "helpers.h"

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc!=4)
		usage(argv[0]);
	

	int fd, sock, size;
	
	struct sockaddr_in to_station;
	char buff[BUFLEN];
	memset(buff, 0, BUFLEN);

	/*Deschidere socket*/
	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	
	/* Deschidere fisier pentru citire */
	DIE((fd=open(argv[3],O_RDONLY))==-1,"open file");
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons (atoi(argv[2]));
	inet_aton(argv[1], &(to_station.sin_addr));

	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/	
	
	while ((size = read(fd, buff, BUFLEN - 1))) {
		sendto(sock, buff, size, 0, (struct sockaddr *)&to_station, sizeof(struct sockaddr_in));
		memset(buff, 0, BUFLEN);
	}

	memset(buff, 0, BUFLEN);
	sprintf(buff, "stop");
	sendto(sock, buff, 5, 0, (struct sockaddr *)&to_station, sizeof(struct sockaddr_in));

	/*Inchidere socket*/
	close(sock);

	
	/*Inchidere fisier*/
	close(fd);

	return 0;
}
