// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int usage(char* name)
{
	printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
	return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name)
{
	int ret;
	struct addrinfo hints, *result, *p;

	memset (&hints, 0, sizeof(hints));
	// TODO: set hints
	hints.ai_family = AF_UNSPEC; //filtrare si pentru IPv4 si pentru IPv6
	hints.ai_protocol = IPPROTO_TCP;

	// TODO: get addresses
	//rezultatul este o lista inlantuita care este parcursa
	// cu ai_next (lista de structuri de tipul addrinfo)
	ret = getaddrinfo(name, NULL, &hints, &result);
	if (ret < 0) {
		gai_strerror(ret);
	}

	// TODO: iterate through addresses and print them

	p = result;
	while (p != NULL) {
		//ipv4
		if (p->ai_family == AF_INET) {
		 	char ip_addr[16]; //retine adresa ipv4
		 	struct sockaddr_in* address = (struct sockaddr_in*)p->ai_addr;
		 	inet_ntop(p->ai_family, &(address->sin_addr), ip_addr, 100);
		 	unsigned short port = address->sin_port;

		 	printf("IP: %s; port: %d\n", ip_addr, ntohs(port));

		} else if (p->ai_family == AF_INET6) { //ipv6
		 	char ip_addr[46]; //retine adresa ipv6
		 	struct sockaddr_in6* address = (struct sockaddr_in6*)p->ai_addr;
		 	inet_ntop(p->ai_family, &(address->sin6_addr), ip_addr, 100);
		 	unsigned short port = address->sin6_port;

		 	printf("IP: %s; port: %d\n", ip_addr, ntohs(port));
		}
		p = p->ai_next;
	}

	// TODO: free allocated data
	freeaddrinfo(result);
}

// Receives an address and prints the associated name and service
void get_name(char* ip)
{
	int ret;
	struct sockaddr_in addr;
	char host[1024];
	char service[20];

	// TODO: fill in address data
	addr.sin_family = AF_INET;
    addr.sin_port = htons (8080);
    inet_aton(ip, &addr.sin_addr);

	// TODO: get name and service
	getnameinfo ((struct sockaddr *)&addr, sizeof(struct sockaddr_in), host, 1024, service, 20, 0);

	// TODO: print name and service
	printf("name: %s; service: %s\n", host, service);
}

int main(int argc, char **argv)
{
	if (argc < 3) {
		return usage(argv[0]);
	}

	if (strncmp(argv[1], "-n", 2) == 0) {
		get_ip(argv[2]);
	} else if (strncmp(argv[1], "-a", 2) == 0) {
		get_name(argv[2]);
	} else {
		return usage(argv[0]);
	}

	return 0;
}
