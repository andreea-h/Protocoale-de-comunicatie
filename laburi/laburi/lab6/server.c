/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
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
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	int fd;
	socklen_t socklen;

	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in from_station ;
	char buf[BUFLEN];
	memset(buf, 0, BUFLEN);


	/*Deschidere socket*/
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	
	
	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */

	from_station.sin_family = AF_INET;
    from_station.sin_port = htons(atoi(argv[1]));
    from_station.sin_addr.s_addr = INADDR_ANY;
	
	/* Legare proprietati de socket */
    bind(sock, (struct sockaddr *) &from_station, sizeof(from_station));

	/* Deschidere fisier pentru scriere */
	DIE((fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1,"open file");
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/


	while (1) {
       int rev = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from_station, &socklen);
       if (strcmp(buf, "stop") == 0) {
           break;
       }
       write(fd, buf, rev);
       memset(buf, 0, BUFLEN - 1);
    }


	/*Inchidere socket*/	
	close(sock);

	
	/*Inchidere fisier*/
	close(fd);


	return 0;
}
