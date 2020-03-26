#include "skel.h"

struct route_table_entry *rtable; //reprezentarea tabelei de rutare

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
/*	for(index = 0; index < rtable_size; index++) {
		printf("%u %u %u %d\n", rtable[index].prefix, rtable[index].next_hop, rtable[index].mask, rtable[index].interface);
	}*/


	setvbuf(stdout, NULL, _IONBF, 0);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		struct ether_header *eth_hdr = (struct ether_header *)(m.payload);
		struct ether_arp *arp_hdr = (struct ether_arp *) (m.payload + sizeof(struct ether_header));
		
		//struct ether_arp *arp_hdr = (struct ether_arp *)(m.payload + sizeof(struct ether_header));
		//struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_arp) + sizeof(struct ether_header));

		u_short type = ntohs(eth_hdr->ether_type); // 0x0806 - arp sau 0X0800 - ip
		//am convertit adresa din network byte order in host byte order

		if(type == 2054) {//pachet ARP
			printf("ARP packet\n");

			u_short arp_type = ntohs(arp_hdr->arp_op);
			printf("%hu\n", arp_type);
			if(arp_type == ARPOP_REQUEST) {  //ARP request
				printf("request\n");
				
				int i;
				printf("sender: ");
				for(i = 0 ; i < 4; i++) {
					printf("%d ", arp_hdr->arp_spa[i]);
				}
				
				printf("target: ");

				char *my_target_ip_address = (char *)malloc(32*sizeof(char));

				for(i = 0 ; i < 4; i++) {
					printf("%d  ", arp_hdr->arp_tpa[i]);
				//	char number[3];
				//	sprintf(number, "%d", arp_hdr->arp_tpa[i]);
				//	strcpy(my_target_ip_address + i*3, number);
				}
				printf("adresa %s\n", my_target_ip_address);
				
				char *interface_ip = get_interface_ip(m.interface);
				printf("intreface_ip: %s\n", interface_ip);
				uint8_t mac;
				get_interface_mac(m.interface, &mac);
				printf("mac %d ", mac);

				//verifica daca adresa IP a interfetei este egala cu adresa target_ip
				//memcpy(arp_hdr->arp_spa, interface_ip, 4);
				memcpy(arp_hdr->arp_sha, &mac, 6);
				u_char tmp[4];
				memcpy(tmp, arp_hdr->arp_spa, 4);
				memcpy(arp_hdr->arp_spa, arp_hdr->arp_tpa, 4);
				
				memcpy(arp_hdr->arp_tpa, tmp, 4);

				uint16_t new_code = htons(0x0002);
				printf("%d\n", new_code);
				memcpy(&(arp_hdr->arp_op), &new_code, 2);

				memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
				memcpy(eth_hdr->ether_shost, &mac, 6);
				//memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);


				send_packet(m.interface, &m);
			}

			else if(arp_type == ARPOP_REPLY) { //ARP response
				printf("reply\n");


			}
		}
		if(type == 2048) {
			printf("IP packet\n");
			//am primit un arp reply cu o noua adresa MAC 
			//trebuie sa updatez tabela de rutare
		} //2048 == 0x0800 //pachet IP

		printf("%hu\n", type);

	}
}
