#include "skel.h"
#include "queue.h"

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
	u_char mac[6]; //adresa MAC asociata
};



struct arp_entry *get_arp_entry(__u32 ip) {
    int i;
    for(i=0 ; i < arp_table_len; i++) {
    //	printf("mac din tabela: %s\n", arp_table[i].mac);
    	if(arp_table[i].ip == ip) {
    		return &arp_table[i];
    	}
    }
    return NULL;
}


void parse_arp_table() 
{
	FILE *f;
	f = fopen("arp_table.txt", "r");
	char line[100];
	int i = 0;
	for(i = 0; fgets(line, sizeof(line), f); i++) {
		char ip_str[50], mac_str[50];
		sscanf(line, "%s %s", ip_str, mac_str);
		fprintf(stderr, "IP: %s MAC: %s\n", ip_str, mac_str);
		arp_table[i].ip = inet_addr(ip_str);
		int rc = hwaddr_aton(mac_str, arp_table[i].mac);
	}
	arp_table_len = i;
	fclose(f);
}

void print_arp() {
	int i,j;
	for(i = 0; i < arp_table_len; i++) {
		for(j = 0 ; j < 6; i++) {
				printf("%02x ", arp_table[i].mac[j]);
		}
		printf("\n");
	}
}

//adauga o noua intrare in tabele ARP
void update_arp_table(uint32_t ip, u_char mac[]) {
	FILE *fp = fopen("arp_table.txt", "a");
	if(fp == NULL) {
		printf("Error...\n");
		exit(1);
	}
	//fseek(fp, 0, SEEK_END); 

	struct in_addr ip_addr;
    ip_addr.s_addr = ntohl(ip);
    printf("adresa catre CARE TRIMIT este %s\n", inet_ntoa(ip_addr));

    //tranforma adresa mac din formatul uint8_t (integer from) in formatul char*
    char string_mac[18];
  	arp_table = (struct arp_entry*)realloc(arp_table, (arp_table_len + 6) * sizeof(struct arp_entry));
  	
  	arp_table[arp_table_len].ip = ip;
  	//memcpy(arp_table[arp_table_len].mac, mac, 6);
  	int i;
  	for(i= 0; i < 6; i++) {
  		arp_table[arp_table_len].mac[i] = mac[i];
  		//printf("%02x", arp_table[arp_table_len].mac[i]);
  	}
    
    snprintf(string_mac, sizeof(string_mac), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fprintf(fp, "%s %s\n", inet_ntoa(ip_addr), string_mac);
	fseek(fp, 0, SEEK_END);
	arp_table_len++;
	printf("%d\n", arp_table_len);	
	fclose(fp);
}


//extrage intrarea din tabela de rutare cu cea mai buna potrivire
struct route_table_entry *get_best_route(__u32 dest_ip) {
    int i;
    int max_len = 0;
 	__u32 dest = ntohl(dest_ip);
    struct route_table_entry* candidate = NULL;

    //adresa care se potiveste si are cei mai muti biti de 1
    for(i = 0; i < rtable_size; i++) {
    	
		if((rtable[i].prefix == (dest & rtable[i].mask)) && (__builtin_popcount(rtable[i].mask) > max_len)) {

    		candidate = &rtable[i];
    		max_len = __builtin_popcount(rtable[i].mask);
    	}
    }
	return candidate;
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

	struct queue *packets = queue_create();

	//parsarea tabelei de rutare
	//initial aloca spatiu pentru o singura intrare in tabela de rutare
	rtable = (struct route_table_entry *)malloc(sizeof(struct route_table_entry));
	rtable_size = read_rtable();
	
	//arp_table = malloc(sizeof(struct  arp_entry) * 100);
	int index;
	for(index = 0; index < rtable_size; index++) {
	//	printf("%u %u %u %d\n", rtable[index].prefix, rtable[index].next_hop, rtable[index].mask, rtable[index].interface);
	}

	arp_table = (struct arp_entry *)malloc(5 * sizeof(struct arp_entry));
	arp_table_len = 0;
	
	setvbuf(stdout, NULL, _IONBF, 0);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		struct ether_header *eth_hdr = (struct ether_header *)(m.payload);
		struct ether_arp *arp_hdr = (struct ether_arp *) (m.payload + sizeof(struct ether_header));
	
		u_short type = ntohs(eth_hdr->ether_type); // 0x0806 - arp sau 0X0800 - ip
		//am convertit adresa din network byte order in host byte order

		
		if(type == 2048) {
			printf("IP packet: ");
			
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
			__u32 dest = ip_hdr->daddr;

			

			struct in_addr ip_addr;
    		ip_addr.s_addr = dest;
    		printf("dest ip address: %s; ", inet_ntoa(ip_addr));
    		ip_addr.s_addr = ntohl(ip_hdr->saddr);
    		printf("ip_source_address %s\n", inet_ntoa(ip_addr));

    		//verificam initial daca pachetul icmp primit este destinat routerului
    		//in caz afirmativ, routerul va trimite un ICMP echo reply (type = 0)
    		//altfel, se va deduce next-hopul pentru pachet din tabela de rutare
            
    		//determinam adresa IP a interfetei routerului cu hostul
    		uint8_t my_mac;
	 		get_interface_mac(m.interface, &my_mac);
	 		char mac_dest[6];
	 		memcpy(mac_dest, eth_hdr->ether_dhost, 6);
	 		printf("MAC addr: ");

	 		uint8_t dest_mac;
	 		hwaddr_aton(mac_dest, &dest_mac);


	 		int i;
	 		for(i = 0; i < 6; i++) {
	 			printf("%02x ", mac_dest[i]);
	 		}


	 		printf("%u %u\n", my_mac, dest_mac);
    		if(my_mac == dest_mac) {
    			//pachet destinat routerului
    			//verifca daca este icmp echo request
    			struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    			u_int8_t type = icmp_hdr->type;
    			u_int8_t code = icmp_hdr->code;
    			if(type == 8 && code == 0) { //echo request
    				// trimite ICMP echo reply
    				printf("routerul trimite reply\n");
    				icmp_hdr->type = 0;

    				uint8_t mac;
	 			    memcpy(&mac, &(eth_hdr->ether_shost), 6);
	 			    memcpy(&(eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
	 			    memcpy(&(eth_hdr->ether_dhost), &mac, 6);
	 				
	 				uint32_t src;
 					memcpy(&src, &(ip_hdr->saddr), 4);
 					memcpy(&(ip_hdr->saddr), &(ip_hdr->daddr), 4);
 					memcpy(&(ip_hdr->daddr), &src, 4);

	 				send_packet(m.interface, &m);
	 				continue;

    			}
    		}
    		
    		
    		//verifica ttl-ul
    		if(ip_hdr->ttl <= 1) {
    			printf("AICI\n");
    			//trimite icmp corect sursei
    			struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    	
    	 		icmp_hdr->type = 11;
				uint8_t mac;
	 			memcpy(&mac, &(eth_hdr->ether_shost), 6);
	 			memcpy(&(eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
	 			memcpy(&(eth_hdr->ether_dhost), &mac, 6);
	 				
	 			uint32_t src;
 				memcpy(&src, &(ip_hdr->saddr), 4);
 				memcpy(&(ip_hdr->saddr), &(ip_hdr->daddr), 4);
 				memcpy(&(ip_hdr->daddr), &src, 4);
	 			send_packet(m.interface, &m);
	 			continue;
    			//pachet aruncat
    		}
    		
    		(ip_hdr->ttl) -= 1;
    		
    		
    		//extrag intrarea din tabela de rutare cu cea mai buna potrivire cu daddr, ca sa aflu next_hop ul
    		struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
    		
    		if(best_route != NULL) {

    			printf("NEXT HOP %u\n", best_route->next_hop);
    			//afiseaza tabela ARP
    		
    			//cautam adresa MAC a next-hop ului in tabela ARP
    			struct arp_entry *arp_entry = get_arp_entry(best_route->next_hop); 
    			int i;
    		//	printf("TABELA ARP\n");
 				if(arp_entry != NULL) {
 					
 					//print_arp();
 					//adresa MAC este cunoscuta local, asa ca voi trimite acum pachetul catre destinatie
 					//seteaza adresa MAC destinatie ca fiind adresa MAC ce tocmai a fost extrasa din tabela arp
 					memcpy(eth_hdr->ether_dhost, arp_entry->mac, 6);

 					//seteaza adresa mac sursa ca fiind adresa mac a interfetei routerului 	
 					uint8_t mac;
 					get_interface_mac(best_route->interface, &mac);
 					
 					memcpy(eth_hdr->ether_shost, &mac, 6);
 					int i;
					printf("aici se trimite un pachet cu adesa mac sursa: ");
					for(i = 0; i < 6; i++) { 
						printf("%02x ", eth_hdr->ether_shost[i]);
					}

					printf("\n");
					send_packet(best_route->interface, &m); 
 				}
 				else {
 					printf("CAUTA ADRESA MAC !!\n");
 					//trimite pachetul catre hoxt-hop
	 				//modifica adresele MAC ale pachetului care urmeaza sa fie trimis
	 				
	 				//obtine adresa MAC a interfetei routeului
	 				//seteaza tipul de mesaj pe arp request


 					
					uint8_t mac;
					get_interface_mac(m.interface, &mac);
				//	printf("mac: %u\n ", mac);

					packet arp_request_pck; //vreau sa trimit un packet arp-request ca sa aflu adresa MAC pentru next-hop
					// cu adresa MAC pe care o "cer" acum o sa completez tabela arp atunci cand voi primi un arp reply de la 
					// hostul catre cate trimit acum acest pachet ARP request
					arp_request_pck.len = 42;
					struct ether_header *eth_hdr = (struct ether_header *)(arp_request_pck.payload);
					struct ether_arp *new_arp_hdr = (struct ether_arp *)(arp_request_pck.payload + sizeof(struct ether_header));

	 				u_char broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	 				memcpy(eth_hdr->ether_dhost, &broadcast, 6);
	 				memcpy(eth_hdr->ether_shost, &mac, 6); //adresa MAC a interfetei routerului

	 				//pachet ARP
					uint16_t new_code1 = htons(0x0806); 
					memcpy(&(eth_hdr->ether_type), &new_code1, 2);

	 				u_short ar_hrd = htons(0x0001); //format of hardware address
	 				memcpy(&(new_arp_hdr->arp_hrd), &ar_hrd, 2);

	 				u_short ar_pro = htons(0x0800); //format of protocol address
	 				memcpy(&(new_arp_hdr->arp_pro), &ar_pro, 2);

	 				u_char len1 = 0x06; //hardware size
	 				memcpy(&(new_arp_hdr->arp_hln), &len1, 1);

	 				u_char len2 = 0x04; //protocol size
	 				memcpy(&(new_arp_hdr->arp_pln), &len2, 1);

	 				uint16_t new_code2 = htons(0x0001);
					memcpy(&(new_arp_hdr->arp_op), &new_code2, 2);

					memcpy(new_arp_hdr->arp_sha, eth_hdr->ether_shost, 6);

	 				char *interface_ip = get_interface_ip(m.interface);
	 				//converteste adresa IP din format char* in format hexa
	 				char src_ip_addr[4];
	 				int i;
	 				char *token = strtok(interface_ip, ".");
	 				for(i = 0; i < 4; i++) {
	 				//	printf("token: %s\n", token);
	 					src_ip_addr[i] = atoi(token);
	 					if(token != NULL) {
	 						token = strtok(NULL, ".");
	 					}	
	 				}

	 				
	 				memcpy(new_arp_hdr->arp_spa, src_ip_addr, 4); //adresa ip a interfetei routerului	
	 				u_char tpa[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	 				memcpy(new_arp_hdr->arp_tha, tpa, 6);

	 				struct in_addr ip_addr;
    				ip_addr.s_addr = ntohl(best_route->next_hop);
    				char *dest = inet_ntoa(ip_addr);
	 				char dest_ip_addr[4];
	 				
	 				char *tokens = strtok(dest, ".");
	 				for(i = 0; i < 4; i++) {
	 				//	printf("token: %s\n", tokens);
	 					dest_ip_addr[i] = atoi(tokens);
	 					if(tokens != NULL) {
	 						tokens = strtok(NULL, ".");
	 					}	
	 				}

	 				memcpy(new_arp_hdr->arp_tpa, dest_ip_addr, 4);
	 				send_packet(best_route->interface, &arp_request_pck); //trimite ARP request
	 				// dupa ce s-a trimis un arp request pentru a afla adresa MAC a destinatiei
	 				// pachetul este adaugat in coada, urmand sa fie dirijat cand routerul primeste ARP reply
	 				
	 				printf("aiccccccccccccccci %d\n", eth_hdr->ether_type);
	 				if(eth_hdr->ether_type == htons(0X0800)) {
	 					m.interface = best_route->interface;
	 					
	 					//memcpy(&(m.interface),&(best_route->interface), 4);
	 					void *p = &m;
	 					printf("m.interface: %u\n", m.interface);
	 					printf("type %d\n", eth_hdr->ether_type);
	 					queue_enq(packets, (&m));
	 					if(queue_empty(packets) == 1) {
	 						printf("coada goalaaaaaaaaaaaaaaaaaaaaaaaa");
	 					}
	 				}

	 				continue;
 				}
    		}
    		if(best_route == NULL) {
    			printf("NU STIU CUM SA TRIMIT ACEST PACHET!!!!\n");
    			// DESTINATION HOST UNREACHABLE (pachetul acesta nu imi este detinatie mie, routerului)
    			// pachet cu adresa inexistenta in tabela de rutare
    			// vom trimite un mesaj IMCP corect sursei (destination unreachable)
    			/*packet unreachable;
    			unreachable.len = 42;
    			struct ether_header *new_eth_hdr = (struct ether_header *)(unreachable.payload);
    			struct iphdr *new_ip_hdr = (struct iphdr*)(unreachable.payload + sizeof(struct ether_header));
    			struct icmphdr *new_icmp_hdr = (struct icmphdr *)(unreachable.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    			new_eth_hdr->ether_type = 2048;
    			//seteaza adresa sursa din headerul ethernet ca fiind adresa mac a interfetei routerului 	
 				uint8_t mac;
 			    ///get_interface_mac(m.interface, &mac);
 			    memcpy(&mac, &(eth_hdr->ether_shost), 6);
 			    memcpy(&(new_eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
 			    memcpy(&(new_eth_hdr->ether_dhost), &mac, 6);
 				
 				uint32_t src;
 				memcpy(&src, &(ip_hdr->saddr), 4);
 				memcpy(&(new_ip_hdr->saddr), &(ip_hdr->daddr), 4);
 				memcpy(&(new_ip_hdr->daddr), &src, 4);
 				
 				new_ip_hdr->version = 4; //se "seteaza" versiunea IPv4
				new_ip_hdr->ihl = 5; //lungimea headerului (5 cuvinte de 32 de biti fiecare)
				                 // imi arata cate cuvinte(de 32 biti fiecare) se citesc mai departe pt header
				new_ip_hdr->ttl = 64;  //seteaza ttl-ul incremental
				new_ip_hdr->protocol = IPPROTO_ICMP; //tipul de protocol
				new_ip_hdr->id = htons(getpid());
			    new_ip_hdr->daddr = ip_addr.s_addr; //destination address - address in network byte order
			    									//(uint32_t)
			    
			    new_ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			    //calculeaza checksum-ul
			    new_ip_hdr->check = 0;
			    new_ip_hdr->check = checksum(new_ip_hdr, sizeof(struct iphdr));
			    //seteaza campurile aferente header-ului icmp
			    new_icmp_hdr->code = 0;
				new_icmp_hdr->type = 3;
				new_icmp_hdr->un.echo.id = htons(getpid());
				
				//seteaza checksum-ul
				new_icmp_hdr->checksum = 0;
				new_icmp_hdr->checksum = checksum(new_icmp_hdr, sizeof(struct icmphdr));
				send_packet(m.interface, &unreachable);*/
    			uint8_t mac;
 			    ///get_interface_mac(m.interface, &mac);
 			    memcpy(&mac, &(eth_hdr->ether_shost), 6);
 			    memcpy(&(eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
 			    memcpy(&(eth_hdr->ether_dhost), &mac, 6);
 				
 			    struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));

 				uint32_t src;
 				uint32_t addr1 = ip_hdr->saddr;
 				memcpy(&src, &addr1, 4);
 				uint32_t addr2 = ip_hdr->daddr;
 				memcpy(&(ip_hdr->saddr), &addr2, 4);
 				memcpy(&(ip_hdr->daddr), &src, 4);
 				

 				icmp_hdr->code = 0;
				icmp_hdr->type = 3;

				send_packet(m.interface, &m);
    		}
		} //2048 == 0x0800 //pachet IP


		if(type == 2054) {//pachet ARP
			printf("ARP packet: ");

			u_short arp_type = ntohs(arp_hdr->arp_op);
			printf("%hu\n", arp_type);
			if(arp_type == ARPOP_REQUEST) {  //ARP request, trebuie verificat daca pachetul primit este destinat routerului
				//se va compara adresa MAC destinatie din pachetul primit cu adresa MAC a interfetei routerului
				printf("request: ");
				
				int i;
				printf("sender: ");
				for(i = 0 ; i < 4; i++) {
					printf("%d ", arp_hdr->arp_spa[i]);
				}
				
				printf("target: ");

				for(i = 0 ; i < 4; i++) {
					printf("%d  ", arp_hdr->arp_tpa[i]);
				}
				printf("\n");
				char *interface_ip = get_interface_ip(m.interface);
			//	printf("intreface_ip: %s\n", interface_ip);

				uint8_t mac;
				get_interface_mac(m.interface, &mac);
			//	printf("mac %d ", mac);

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
					ip_address += (arp_hdr->arp_tpa[i]) * putere;
				}

				struct in_addr ip_addr;
   				ip_addr.s_addr = ntohl(ip_address);
   				sprintf(target_ip_address, "%s", inet_ntoa(ip_addr));

   				//daca requestul este destinat routeului, voi trimite reply cu adresa mea MAC
   				//adica voi trimite adresa MAC a interfetei routerului cu hostul care a facut cererea
				if(strcmp(target_ip_address, interface_ip) == 0) {

					memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
					memcpy(eth_hdr->ether_shost, &mac, 6);

					u_char old_sha[6];
					memcpy(old_sha, arp_hdr->arp_sha, 6);

				    memcpy(arp_hdr->arp_tha, old_sha, 6);
					memcpy(arp_hdr->arp_sha, &mac, 6); //mac-ul cautat
					u_char tmp[4];

					//inverseaza spa si tpa
					memcpy(tmp, arp_hdr->arp_spa, 4);
					memcpy(arp_hdr->arp_spa, arp_hdr->arp_tpa, 4);
					memcpy(arp_hdr->arp_tpa, tmp, 4);

					uint16_t new_code = htons(0x0002);
					printf("new_op_code: %d\n", new_code);
					memcpy(&(arp_hdr->arp_op), &new_code, 2);

					send_packet(m.interface, &m); //trimite ARP reply cu adresa MAC a interfetei pe care s-a primit un pachet
					
					continue;
				}
				
			}

			else if(arp_type == ARPOP_REPLY) { //ARP response
				printf("reply: ");
				//adresa mac este in shost
				//trebuie sa adaugam in tabela arp o noua intrare avand ip egal cu arp_hdr->srp_spa
				//si adresa mac salvata in arp_hdr->sha
				//trebui sa convertim adresa ip din binary form in forma integer
				uint32_t ip_address = 0;
				int i;
				printf("adresa ip sursa pt reply: \n");
				for(i = 0; i < 4; i++) {
					int j;
					int putere = 1;
					for(j = 1; j <= 3-i; j++) {
						putere = putere * 256;
					}
					ip_address += (arp_hdr->arp_spa[i]) * putere;
					printf("%d.", arp_hdr->arp_spa[i]);
				}
				
				printf("\n");
				update_arp_table(ip_address, arp_hdr->arp_sha);
		
				if(queue_empty(packets) != 1) { //coada nu este goala

					void *pck = queue_deq(packets);
					packet *my_packet = (packet*)pck;

					//schimba adresa MAC destinatie cu adresa MAC primita prin ARP reply
					struct ether_header *new_hdr = (struct ether_header *)(my_packet->payload);
					printf("PACHET SCOS DIN COADA: %u\n", new_hdr->ether_type);
					
					//adresa MAC a intrefetei routerului
					uint8_t mac;
					get_interface_mac(my_packet->interface, &mac);
					memcpy(new_hdr->ether_shost, &mac, 6);
					memcpy(new_hdr->ether_dhost, arp_hdr->arp_sha, 6);
					if(new_hdr->ether_type != 1544) {
						send_packet(my_packet->interface, my_packet); //transmite un pachet extras din coada
					}
					
				}
				else {
					printf("coada goala\n");
				}
			}
		}
				
	}

					

}