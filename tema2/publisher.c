#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#define BUFLEN 1501

void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server\n",file);
	exit(0);
}

/*
*	Utilizare: ./client ip_server port_server 
*/
int main(int argc,char**argv)
{
	if (argc!=3)
		usage(argv[0]);
	
	int sock, size;
	
	struct sockaddr_in to_station;
	char buff[BUFLEN];
	memset(buff, 0, BUFLEN);

	/*Deschidere socket pentru a trimite mesaje UDP*/
	sock = socket(AF_INET, SOCK_DGRAM, 0); 
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons (atoi(argv[2])); //port server
	inet_aton(argv[1], &(to_station.sin_addr)); //ip_server

	//trimite pe socket ceea ce primeste clientul de la tastatura
	memset(buff, 0, BUFLEN);
	while ((size = read(0, buff, BUFLEN))) {
		printf("ai introdus: %s\n", buff);
		if (strcmp(buff, "exit\n") == 0) {
			break;
		}
		//se trimite stringul introdus (fara caracterul '\n')
		sendto(sock, buff, size - 1, 0, (struct sockaddr *)&to_station, sizeof(struct sockaddr_in));
		memset(buff, 0, BUFLEN);
	}


	/*Inchidere socket*/
	close(sock);

	return 0;
}
