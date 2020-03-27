#include "skel.h"

struct route_table_entry *rtable; //reprezentarea tabelei de rutare
struct arp_entry *arp_table; //tabela arp 
int arp_table_len; //nr de intrari din tabela arp
int rtable_size; //nr te intrari din tabela de rutare 

//descrie o intrare in tabela de rutare
struct route_table_entry {
	uint32_t prefix; //unsigned int
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

struct arp_entry {
	uint32_t ip; //adresa ip
	uint8_t mac[6]; //adresa MAC asociata
}__attribute__((packed));


//adauga o noua intrare in tabele ARP
void update_arp_table(uint32_t ip, u_char mac[]) {
	FILE *fp = fopen("arp_table.txt", "w");
	if(fp == NULL) {
		printf("Error...rtable.txt file not found\n");
		exit(1);
	}
	fseek(fp, 0, SEEK_END); 
	struct in_addr ip_addr;
    ip_addr.s_addr = ntohl(ip);

    //tranforma adresa mac din formatul uint8_t (integer from) in formatul char*
    char string_mac[18];
  
    snprintf(string_mac, sizeof(string_mac), "%02x:%02x:%02x:%02x:%02x:%02x",
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fprintf(fp, "%s %s\n", inet_ntoa(ip_addr), string_mac);
	fclose(fp);
}

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
	rtable_size = read_rtable();
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

				for(i = 0 ; i < 4; i++) {
					printf("%d  ", arp_hdr->arp_tpa[i]);
				}
				
				char *interface_ip = get_interface_ip(m.interface);
				printf("intreface_ip: %s\n", interface_ip);

				uint8_t mac;
				get_interface_mac(m.interface, &mac);
				printf("mac %d ", mac);

				//verifica daca pachetul primit imi este destinat mie routerului
				//adica compar adresa mea ip(interface_ip) cu adresa target_ip din arp_hdr pt pachetul primit
				//adresa target ip este in format binar in headerul ip, asa ca o voi converti in char* (mai intai in integer, apoi din integer convertesc in char*)
				char target_ip_address[4];

				uint32_t ip_address = 0;
				
				for(i = 0; i < 4; i++) {
					int j;
					int putere = 1;
					for(j = 1; j <= 3-i; j++) {
						putere = putere * 256;
					}
					ip_address += (arp_hdr->arp_spa[i]) * putere;
				}

				struct in_addr ip_addr;
   				ip_addr.s_addr = ntohl(ip_address);
   				sprintf(target_ip_address, "%s", inet_ntoa(ip_addr));

				if(strcmp(target_ip_address, interface_ip) == 0) {
					memcpy(arp_hdr->arp_sha, &mac, 6); //mac-ul cautat
					u_char tmp[4];
					//inverseaza spa si tpa
					memcpy(tmp, arp_hdr->arp_spa, 4);
					memcpy(arp_hdr->arp_spa, arp_hdr->arp_tpa, 4);
					memcpy(arp_hdr->arp_tpa, tmp, 4);

					uint16_t new_code = htons(0x0002);
					printf("new_op_code: %d\n", new_code);
					memcpy(&(arp_hdr->arp_op), &new_code, 2);

					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
					memcpy(eth_hdr->ether_shost, arp_hdr->arp_sha, 6);
					//memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);


					send_packet(m.interface, &m);
				}
				
			}

			else if(arp_type == ARPOP_REPLY) { //ARP response
				printf("reply\n");
				//adresa mac este in shost
				//trebuie sa adaugam in tabela arp o noua intrare avend ip egal cu arp_hdr->srp_spa
				//si adresa mac salvata in arp_hdr->sha
				//trebui sa convertim adresa ip din binary form in forma integer
				uint32_t ip_address = 0;
				int i;
				for(i = 0; i < 4; i++) {
					int j;
					int putere = 1;
					for(j = 1; j <= 3-i; j++) {
						putere = putere * 256;
					}
					ip_address += (arp_hdr->arp_spa[i]) * putere;
				}
				printf("reply ip address: %d\n", ip_address);

				update_arp_table(ip_address, arp_hdr->arp_sha);

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
