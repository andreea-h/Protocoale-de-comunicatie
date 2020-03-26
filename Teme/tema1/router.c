#include "skel.h"

struct route_table_entry *rtable; //tabela de rutare

//descrie o intrare in tabela de rutare
struct route_table_entry {
	uint32_t prefix; //unsigned int
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));


//plaseaza in 'rtable' reprezentarea interna a tabelei de rutare(preluata din "rtable.txt")
int read_rtable() {
	int rtable_size = 0;
	FILE *fp = fopen("rtable.txt", "r");
	if(fp == NULL) {
		printf("Error...rtable.txt not found\n");
		exit(1);
	}

	char *buffer = (char *)malloc(256*sizeof(char));
	size_t buffsize;
	size_t len;
	while((len = getline(&buffer, &buffsize, fp)) != -1) {
	//	printf("%s\n", buffer);
		rtable_size++;
	}
	fclose(fp);

	rtable = realloc(rtable, rtable_size * (sizeof(struct route_table_entry)));
	int index = 0;
	int k = 0;
	fp = fopen("rtable.txt", "r");
	while((len = getline(&buffer, &buffsize, fp)) != -1) {
	//	printf("%s\n", buffer);
		char *tokens = strtok(buffer, " ");
		while(tokens != NULL) {
			
			if(k % 4 == 0) {
				rtable[index].prefix = ntohl(inet_addr(tokens));
			}
			else if(k % 4 == 1) {
				rtable[index].next_hop = ntohl(inet_addr(tokens));
			}
			else if(k % 4 == 2) {
				rtable[index].mask = ntohl(inet_addr(tokens));
			}
			else if(k % 4 == 3) {
				rtable[index].interface = atoi(tokens);
			}
			tokens = strtok(NULL, " ");
			k++;
		}
		index++;
	}
	fclose(fp);
	return rtable_size;
}

struct route_table_entry *rtable; //reprezentarea tabelei de rutare

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init();

	//parsarea tabelei de rutare
	//initial aloca spatiu pentru o singura intrare in tabela de rutare
	rtable = (struct route_table_entry *)malloc(sizeof(struct route_table_entry));
	int rtable_size = read_rtable();
	int index;
	for(index = 0; index < rtable_size; index++) {
		printf("%u %u %u %d\n", rtable[index].prefix, rtable[index].next_hop, rtable[index].mask, rtable[index].interface);
	}


	setvbuf(stdout, NULL, _IONBF, 0);



	while (1) {
		rc = get_packet(&m); //routerul primeste un pachet de la oricare din interfetele adiacente
		DIE(rc < 0, "get_message");
		
		// se va extrage adresa ip destinatie asociata pachetului
		// localizeaza cea mai specifica intrare din tabela de rutare
		// transmite pachetul catre urmatorul hop

		//extrage ethernet header-ul pachetului primit
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
	//	struct ether_arp* ether_arp = (struct ether_arp *)(m.payload + sizeof(struct ether_header));
		//extrage apr header-ul pachetului primit
		//struct arphdr *arp_hdr = (struct arphdr *)(m.payload + sizeof(struct ether_header));
		struct ether_arp *eth_arp = (struct ether_arp *)(m.payload + sizeof(struct ether_header));
		struct arphdr arp_hdr = eth_arp->ea_hdr;

		//extrage ip header-ul pachetului primit
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct ether_arp));
		//extrage header-ul imcp al pachetului primit
		struct icmphdr *imcp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct ether_arp) + sizeof(struct iphdr));
		
		//extrage adresa IP destinatie a pachetului care tocmai a fost primit
		
		u_short type = ntohs(eth_hdr->ether_type); // 0x0806 - arp sau 0X0800 - ip
		//am convertit adresa din network byte order in host byte order

		if(type == 2054) {
			
			//se determina tipul de pachet arp: reply sau request
			u_short arp_type = ntohs(arp_hdr.ar_op);
			//printf("%hu\n", arp_type);
			if(arp_type == ARPOP_REQUEST) { 
				printf("request\n");
				/*int i;
				for(i = 0; i < ROUTER_NUM_INTERFACES; i++) {
					printf("%d ", interfaces[i]);
					printf("%s ", get_interface_ip(interfaces[i]));
				    uint8_t mac;
					get_interface_mac(i, &mac);
					uint8_t correct_mac = ntohs(mac); //adresa mac a subretelei
					printf("%hu\n", correct_mac);
				}*/
				//cauta interfata asociata pachetului tocmai primit
				//determina adresa mac aferenta adresei ip care a fost primita
				int i; //asta inseamna ca atunci cand am ajuns aici stiu ca adresa ip a pachetului care se vrea trimis nu este in arp table
				//vreau sa afisez adresa ip destinatie
			//	u_char *destination = malloc(7*sizeof(u_char));
				//memcpy(destination, eth_hdr->ether_dhost, 6);
			//	printf("%s\n", eth_hdr->ether_dhost);
			//	printf("%s\n", destination);
			//	free(destination);
			//	printf("%d\n", m.interface);
				/*
				for(i = 0; i < rtable_size; i++) {
					if(daddr == ntohl(rtable[i].prefix)) {
						printf("%d %d\n", i, rtable[i].interface);
					}
				}*/

		//!!!!		//caut adresa MAC asociata adresei ip a pachetului primit
		///!!!		//sau iau adresa MAC din tabela de rutare
 
				//for(i = 0; i < 6; i++)
				//	printf("%d.", eth_hdr->ether_dhost); //adresa mac a sursei
			/*	struct in_addr ip_addr;
				ip_addr.s_addr = daddr;
				printf("%s\n", inet_ntoa(ip_addr)); 
				printf("%d\n", m.interface);*/
				/*


				__u32 destination = ip_hdr->daddr;
				__u32 source = ip_hdr->saddr;
			//	printf("%u %u\n", source, destination);
			//	printf("%s\n", eth_hdr->ether_dhost);
			//	printf("%s\n", eth_hdr->ether_shost);
				char *result = (char *)malloc(20*sizeof(char));
				strcpy(result, get_interface_ip(m.interface));
				uint8_t mac;
				get_interface_mac(m.interface, &mac);
				//printf("%s - %d\n",result, mac); //adresa mac
				//trimite un arp relpy cu aceasta adresa mac
				memcpy(&eth_hdr->ether_dhost, &mac, 6);
				send_packet(m.interface, &m);*/

				//obtine ip-ul interfetei pe care a venit mesajul
			    printf("%s\n", get_interface_ip(m.interface));
			    //id-ul interfetei pe care trebuie trimis un packet: in tabela de rutare
			    //cauta campul target_ip
				printf("%u\n", ip_hdr->daddr);
				printf("%s\n", eth_hdr->ether_dhost);

			}

			else if(arp_type == ARPOP_REPLY) {

				printf("reply\n");
			}
			
		} //2054 = 0x0806 //pachet ARP
		else if(type == 2048) {
			//am primit un arp reply cu o noua adresa MAC 
			//trebuie sa updatez tabela de rutare
		} //2048 == 0x0800 //pachet IP
		//printf("%hu\n", type);

		
		//headere: ethernet, arp, ipv4, icmp
	}
}
