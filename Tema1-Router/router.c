#include "skel.h"
#include "queue.h"

struct route_table_entry *rtable; //reprezentarea tabelei de rutare
struct arp_entry *arp_table; //tabela arp 
int arp_table_len; //nr de intrari din tabela arp
int rtable_size; //nr de intrari din tabela de rutare 

//structura care descrie o intrare in tabela de rutare
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

//returneaza intrarea din tabela arp care are adresa ip cea primita ca parametru
//daca nu exista o astfel de intrare, returneaza NULL
struct arp_entry *get_arp_entry(__u32 ip) {
    int i;
    for(i=0 ; i < arp_table_len; i++) {
    	if(arp_table[i].ip == ip) {
    		return &arp_table[i];
    	}
    }
    return NULL;
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

//adauga o noua intrare in tabela ARP 
//este adaugata o linie noua in fisieul "arp_table.txt" si un element nou in vectorul arp_table
void update_arp_table(uint32_t ip, u_char mac[]) {
	FILE *fp = fopen("arp_table.txt", "a");
	if(fp == NULL) {
		printf("Error...\n");
		exit(1);
	}
	//adresa ip va fi tranformata in format text ACII pentru a afisarea in fisier, folosind inet_ntoa
	struct in_addr ip_addr;
    ip_addr.s_addr = ntohl(ip);
    
    char string_mac[18];
  	arp_table = (struct arp_entry*)realloc(arp_table, (arp_table_len + 6) * sizeof(struct arp_entry));
  	arp_table[arp_table_len].ip = ip;
  	int i;
  	for(i= 0; i < 6; i++) {
  		arp_table[arp_table_len].mac[i] = mac[i];
  	}

  	//tranforma adresa MAC din format u_char[] in formatul char* pentru a face afisarea in fisier
    snprintf(string_mac, sizeof(string_mac), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	fprintf(fp, "%s %s\n", inet_ntoa(ip_addr), string_mac);
	fseek(fp, 0, SEEK_END);
	arp_table_len++;
	fclose(fp);
}


// functia get_best_route intoarce prima intrare din tabela sortata care reprezinta best_route pentru adresa destinatie 'dest_ip'
int get_best_route(__u32 dest_ip) {
	int left = 0;
	int right = rtable_size - 1;
	int result = -1;
	while(left <= right) {
		int middle = (right + left)/2;
		if(htonl(rtable[middle].prefix) == (dest_ip & htonl(rtable[middle].mask))) {
			result = middle;
			right = middle - 1;
		}
		else if(htonl(rtable[middle].prefix) > (dest_ip & htonl(rtable[middle].mask))) {
			right = middle - 1;
		}
		else {
			left = middle + 1;
		}
	}
    return result;
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
		rtable_size++; //rtable_size memoreaza numarul de intrari din tabela de rutare
	}
	fclose(fp);

	//aloca spatiu pe heap pentru cele rtable_size intrari din tabela de rutare
	rtable = realloc(rtable, rtable_size * (sizeof(struct route_table_entry)));
	int index = 0;
	int k = 0;
	fp = fopen("rtable.txt", "r");
	while((len = getline(&buffer, &buffsize, fp)) != -1) {
		char *tokens = strtok(buffer, " ");
		while(tokens != NULL) {
			// am memorat adresele in tabela de rutare in host byte order
			//am folosit inet_addr pentru a converti stringul care memoreaza adresa in formatul integer
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

//compara 2 intrari din tabela de rutare crescator dupa prefix si descrescator dupa masca
int comparator(const void *entry1, const void *entry2)  
{ 
    uint32_t prefix1 = htonl(((struct route_table_entry *)entry1)->prefix); 
    uint32_t prefix2 = htonl(((struct route_table_entry *)entry2)->prefix); 
    if(prefix1 != prefix2) { //in situatia in care 2 intrari au acelasi prefix, ele voi fi comparate dupa numarul de biti din masca
    	return (prefix1 - prefix2);
    }
    uint32_t mask1 = ((struct route_table_entry *)entry1)->mask; 
    uint32_t mask2 = ((struct route_table_entry *)entry2)->mask; 
    //ordonand 2 intrari cu acelasi prefix descrescator dupa masca, prima aparitie a unui prefix va fi cea are are asociata masca cea mai lunga
    return (__builtin_popcount(mask2) - __builtin_popcount(mask1));
} 

int main(int argc, char *argv[])
{
	packet m;
	int rc;
	int received = 0;
	init();

	// coada pentru memorarea pachetelor a caror MAC destinatie nu este cunoscut momentan
	// aceste pachete vor fi dirijate dupa ce routerul primeste ARP request
	struct queue *packets = queue_create(); 

	//parsarea tabelei de rutare
	//initial aloca spatiu pentru o singura intrare in tabela de rutare
	rtable = (struct route_table_entry *)malloc(sizeof(struct route_table_entry));
	rtable_size = read_rtable();

	//sorteaza tabela de rutare
	qsort((void*)rtable, rtable_size, sizeof(struct route_table_entry), comparator);


	arp_table = (struct arp_entry *)malloc(5 * sizeof(struct arp_entry));
	arp_table_len = 0;
	
	setvbuf(stdout, NULL, _IONBF, 0);

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		struct ether_header *eth_hdr = (struct ether_header *)(m.payload);
		struct ether_arp *arp_hdr = (struct ether_arp *) (m.payload + sizeof(struct ether_header));
	
		u_short type = ntohs(eth_hdr->ether_type); // 0x0806 - arp sau 0X0800 - ip
		//folosind ntohs, am convertit continutul de la adresa aferenta campului type din network byte order in host byte order

		if(type == 2048) {
			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
			__u32 dest = ip_hdr->daddr;

    		//verificam initial daca pachetul icmp primit este destinat routerului
    		//in caz afirmativ, routerul va trimite un ICMP echo reply (type = 0)
    		//altfel, se va cauta next-hopul pentru pachet din tabela de rutare
    		char *router_ip = get_interface_ip(m.interface); //adresa ip a interfetei routerului cu hostul care a trimis pachetul
    		//transormam adresa IP din format ascii text (router_ip) in binary form (in network byte order)
    		struct in_addr dest_ip_address; 
    		inet_aton(router_ip, &dest_ip_address); 

			__u32 ip_address = dest_ip_address.s_addr; 

			if(ip_address == dest) {
    			//pachet destinat routerului
    			//verifca daca este icmp echo request
    			struct icmphdr *icmp_hdr = (struct icmphdr *)(m.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    			u_int8_t type = icmp_hdr->type;
    			u_int8_t code = icmp_hdr->code;
    			if(type == 8 && code == 0) { //pachet ICMP echo request -> // trimite ICMP echo reply

					if(checksum(ip_hdr, sizeof(struct iphdr)) != 0) //daca un pachet are checksum gresit, este aruncat
						continue;

		    		(ip_hdr->ttl) -= 1;
		    		
		    		ip_hdr->check = 0;
		    		ip_hdr->check = checksum(ip_hdr, sizeof(struct iphdr));
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

    		//daca TTL <= 1 trimite sursei in pachet ICMP de tipul Time Exceeded
    		if(ip_hdr->ttl <= 1) {
    		
	 			packet timeout;
	 			timeout.len = 42;

    			struct ether_header *new_eth_hdr = (struct ether_header *)(timeout.payload);
    			struct iphdr *new_ip_hdr = (struct iphdr*)(timeout.payload + sizeof(struct ether_header));
    			struct icmphdr *new_icmp_hdr = (struct icmphdr *)(timeout.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    			
    			//seteaza adresa sursa din headerul ethernet ca fiind adresa destinatie a pachetului primit	
 			    uint8_t mac;
 			    memcpy(&mac, eth_hdr->ether_shost, 6); 
 			    memcpy(&(new_eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
 			    memcpy(&(new_eth_hdr->ether_dhost), &mac, 6); //adresa destinatie este adresa sursa a pachetului primit
 				new_eth_hdr->ether_type = htons(0X0800); //pachet icmp

 				new_ip_hdr->version = 4; //se "seteaza" versiunea IPv4
 				new_ip_hdr->ihl = 5; //lungimea headerului (5 cuvinte de 32 de biti fiecare)
 				new_ip_hdr->tot_len = htons(28);
 				new_ip_hdr->id = htons(getpid());
				
 				new_ip_hdr->frag_off = 0x00;
 				new_ip_hdr->ttl = 64;  
				new_ip_hdr->protocol = 1; //tipul de protocol (icmp)

				new_ip_hdr->check = 0;
			    new_ip_hdr->check = checksum(new_ip_hdr, sizeof(struct iphdr));

				uint8_t mac2;
				memcpy(&mac2, &(ip_hdr->saddr), 4);
 				memcpy(&(new_ip_hdr->saddr), &mac2, 4);
 				uint8_t mac3;
 				memcpy(&mac3, &(ip_hdr->daddr), 4);
 				memcpy(&(new_ip_hdr->daddr), &mac3, 4);
			   
			    //seteaza campurile aferente header-ului icmp
			   	new_icmp_hdr->type = 11;
			    new_icmp_hdr->code = 0;
				
				//seteaza checksum-ul
				new_icmp_hdr->un.echo.id = htons(getpid());
				
				new_icmp_hdr->un.echo.sequence = htons(received++);
				new_icmp_hdr->checksum = 0;
				new_icmp_hdr->checksum = checksum(new_icmp_hdr, sizeof(struct icmphdr));
				
				send_packet(m.interface, &timeout); //trimite pachetul timeout pe intrefata de pe care a venit pachetul m
				continue; //se arunca vechiul pachet (m)
    		}
    		
    		//daca pachetul primit are un checksum gresit, este aruncat
			if(checksum(ip_hdr, sizeof(struct iphdr)) != 0)
				continue;

    		(ip_hdr->ttl) -= 1;
    		
    		ip_hdr->check = 0;
    		ip_hdr->check = checksum(ip_hdr, sizeof(struct iphdr)); //updateaza checksum-ul pentru pachetul IP
    		
    		//este extrasa intrarea din tabela de rutare cu cea mai buna potrivire cu daddr, cu scopul aflarii next_hop-ului
    		
    		int best_route_pos = get_best_route(ip_hdr->daddr); //indicele din tabela de rutare pentru best_route
    	
    		struct route_table_entry *best_route = NULL;

    		if(best_route_pos != -1) {
    			best_route = &rtable[best_route_pos];
    		}

    		if(best_route != NULL) { 
    			//cautam adresa MAC a next-hop ului in tabela ARP
    			struct arp_entry *arp_entry = get_arp_entry(best_route->next_hop); 
 				if(arp_entry != NULL) {
 					
 					//adresa MAC este cunoscuta local, asa ca voi trimite acum pachetul catre destinatie
 					//seteaza adresa MAC destinatie ca fiind adresa MAC ce tocmai a fost extrasa din tabela arp
 					memcpy(eth_hdr->ether_dhost, arp_entry->mac, 6);

 					//seteaza adresa mac sursa ca fiind adresa mac a interfetei routerului cu hostul catre care trebui trimis pachetul
 					uint8_t mac;
 					get_interface_mac(best_route->interface, &mac);
 					
 					memcpy(eth_hdr->ether_shost, &mac, 6);
					send_packet(best_route->interface, &m); 
 				}
 				else { 
	 				//routerul va trimite un mesaj de tipul ARP request pentru a afla adresa MAC destinatie a pachetului primit
 					
					uint8_t mac;
					get_interface_mac(m.interface, &mac);

					packet arp_request_pck; 
					// cu adresa MAC pe care o "cer" acum o sa completez tabela arp atunci cand voi primi un arp reply de la 
					// hostul catre cate trimit acum acest pachet ARP request

					arp_request_pck.len = 42;
					struct ether_header *new_eth_hdr = (struct ether_header *)(arp_request_pck.payload);
					struct ether_arp *new_arp_hdr = (struct ether_arp *)(arp_request_pck.payload + sizeof(struct ether_header));

	 				u_char broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
	 				memcpy(new_eth_hdr->ether_dhost, &broadcast, 6);
	 				memcpy(new_eth_hdr->ether_shost, &mac, 6); //adresa MAC a interfetei routerului cu hostul de pe care a fost trimis pachetul

	 				//pachet ARP
					uint16_t new_code1 = htons(0x0806); 
					memcpy(&(new_eth_hdr->ether_type), &new_code1, 2);

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

					uint8_t mac2;
					get_interface_mac(best_route->interface, &mac2);
					memcpy(new_arp_hdr->arp_sha, &mac2, 6); //adresa mac sursa din headerul arp este adresa MAC a interfetei pe care va fi trimis pachetul

	 				char *interface_ip = get_interface_ip(m.interface);
	 				//converteste adresa IP din format char* in format hexa
	 				char src_ip_addr[4];
	 				int i;
	 				char *token = strtok(interface_ip, ".");
	 				for(i = 0; i < 4; i++) {
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
	 					dest_ip_addr[i] = atoi(tokens);
	 					if(tokens != NULL) {
	 						tokens = strtok(NULL, ".");
	 					}	
	 				}

	 				memcpy(new_arp_hdr->arp_tpa, dest_ip_addr, 4);
	 				send_packet(best_route->interface, &arp_request_pck); //trimite ARP request
	 				// dupa ce s-a trimis un arp request pentru a afla adresa MAC a destinatiei
	 				// pachetul este adaugat in coada, urmand sa fie dirijat cand routerul primeste ARP reply
	 				
	 				//pachetul adaugat in coada trebuie sa fie un pachet IP
	 				if(eth_hdr->ether_type == htons(0X0800)) {
	 					m.interface = best_route->interface;
	 					packet send_packet = m;
	 					void *p = &send_packet;
	 					queue_enq(packets, p);
	 				}
	 				continue;
 				}
    		}
    		
    		if(best_route == NULL) {
    			// DESTINATION HOST UNREACHABLE (pachetul acesta nu imi este detinatie mie, routerului), nu exista ruta pana la destinatie
    			// este construit in pachet icmp, avand tipul precizat mai sus
    			
    			packet unreachable;
	 			unreachable.len = 42;

    			struct ether_header *new_eth_hdr = (struct ether_header *)(unreachable.payload);
    			struct iphdr *new_ip_hdr = (struct iphdr*)(unreachable.payload + sizeof(struct ether_header));
    			struct icmphdr *new_icmp_hdr = (struct icmphdr *)(unreachable.payload + sizeof(struct ether_header) + sizeof(struct iphdr));
    			
    			//seteaza adresa sursa din headerul ethernet ca fiind adresa mac destinatie a pachetului
 			    uint8_t mac;
 			    memcpy(&mac, eth_hdr->ether_shost, 6); 
 			    memcpy(&(new_eth_hdr->ether_shost), &(eth_hdr->ether_dhost), 6);
 			    memcpy(&(new_eth_hdr->ether_dhost), &mac, 6); //adresa mac destinatie a pachetului este fosta adresa sursa
 				new_eth_hdr->ether_type = htons(0X0800); //tip de pachet - ip

 				new_ip_hdr->version = 4; //se "seteaza" versiunea IPv4
 				new_ip_hdr->ihl = 5; //lungimea headerului (5 cuvinte de 32 de biti fiecare)
 				new_ip_hdr->tot_len = htons(28);
 				new_ip_hdr->id = htons(getpid());
				
 				new_ip_hdr->frag_off = 0x00;
 				new_ip_hdr->ttl = 64;  
				new_ip_hdr->protocol = 1; //tipul de protocol - icmp

				new_ip_hdr->check = 0;
			    new_ip_hdr->check = checksum(new_ip_hdr, sizeof(struct iphdr));

				uint8_t mac2;
				memcpy(&mac2, &(ip_hdr->saddr), 4);
 				memcpy(&(new_ip_hdr->saddr), &mac2, 4); //adresa ip sursa a pachetului trimis va fi adresa ip sursa a pachetului primit
 				uint8_t mac3;
 				memcpy(&mac3, &(ip_hdr->daddr), 4);
 				memcpy(&(new_ip_hdr->daddr), &mac3, 4); //adresa ip destinatie a pachetului trimis va fi adresa ip destinatie a pachetului primit
			   
			    //seteaza campurile aferente header-ului icmp
			   	new_icmp_hdr->type = 3; //pachet icmp de tipul 'destination host unreachable'
			    new_icmp_hdr->code = 0;
			
				new_icmp_hdr->un.echo.id = htons(getpid());
				new_icmp_hdr->un.echo.sequence = htons(received++);
				
				//seteaza checksum-ul
				new_icmp_hdr->checksum = 0;
				new_icmp_hdr->checksum = checksum(new_icmp_hdr, sizeof(struct icmphdr));
				
				send_packet(m.interface, &unreachable); //pachetul de tipul 'destination host unreachable' este trimis pe intrefata de care care s-a incercat trimiterea pachetului
				continue; //se arunca vechiul pachet (m)
    		}
		} //2048 == 0x0800 //pachet IP


		if(type == 2054) {//pachet ARP
			u_short arp_type = ntohs(arp_hdr->arp_op); //tipul de pacher ARP, reply sau request 
			if(arp_type == ARPOP_REQUEST) {  //ARP request, trebuie verificat daca pachetul primit este destinat routerului
				//se va compara adresa destinatie din pachetul primit (target protocol address) cu adresa IP a interfetei routerului
				
				char *interface_ip = get_interface_ip(m.interface); //adresa IP a interfetei routerului cu hostul care a trimis un pachet
			
				uint8_t mac;
				get_interface_mac(m.interface, &mac); //adresa MAC a interfetei routerului cu hostul sursa
		
				//verifica daca pachetul primit imi este destinat mie, routerului
				//adica compar adresa mea ip(interface_ip) cu adresa target_ip din arp_hdr pt pachetul primit
				//adresa target ip este in format binar in headerul ip, asa ca o voi converti in char* (mai intai in integer, apoi din integer convertesc in char*)
				char target_ip_address[4];

				uint32_t ip_address = 0; //va memora adresa destinatie din headerul arp, adica adresa IP pentru care s-a facut Request
				//adresa arp_hdr->arp_tpa este convertita din u_char[] in integer
				int i;
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
   				sprintf(target_ip_address, "%s", inet_ntoa(ip_addr)); //target_ip_address retine adresa destinatie a pachetului in forma ascii text

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

					//inverseaza spa si tpa din pachetul request primit
					memcpy(tmp, arp_hdr->arp_spa, 4);
					memcpy(arp_hdr->arp_spa, arp_hdr->arp_tpa, 4);
					memcpy(arp_hdr->arp_tpa, tmp, 4);

					uint16_t new_code = htons(0x0002);
					memcpy(&(arp_hdr->arp_op), &new_code, 2);

					send_packet(m.interface, &m); //trimite ARP reply cu adresa MAC a interfetei pe care s-a primit un pachet
					continue;
				}
				
			}

			else if(arp_type == ARPOP_REPLY) { //routerul a primit Reply pentru una dintre adresele MAC 'cerute' anterior
				//adresa mac cautata este in campul shost
				//trebuie sa adaugam in tabela arp o noua intrare avand IP-ul la adresa arp_hdr->arp_spa si adresa mac salvata in arp_hdr->sha
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
				
				update_arp_table(ip_address, arp_hdr->arp_sha); //adauga a noua intrare in structura care reprezinta tabela ARP si completeza 'arp_table.txt
		
				if(queue_empty(packets) != 1) { //daca coada pachetelor care trebui trimise nu este goala

					void *pck = queue_deq(packets);
					packet *my_packet = (packet*)pck;

					//schimba adresa MAC destinatie a pachetului care trebuie trimis cu adresa MAC primita prin ARP reply
					struct ether_header *new_hdr = (struct ether_header *)(my_packet->payload);
				
					//adresa MAC a intrefetei routerului
					uint8_t mac;
					get_interface_mac(my_packet->interface, &mac);
					memcpy(new_hdr->ether_shost, &mac, 6);
					memcpy(new_hdr->ether_dhost, arp_hdr->arp_sha, 6);
					if(new_hdr->ether_type != 1544) { //am verificat inca o data ca pachetul extras din coada si care trebuie trimis sa nu fie un pachet arp
						send_packet(my_packet->interface, my_packet); //transmite un pachet extras din coada
					}
					
				}
			}
		}
				
	}

}				

