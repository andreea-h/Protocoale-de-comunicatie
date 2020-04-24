#include "skel.h"

int interfaces[ROUTER_NUM_INTERFACES];
struct route_table_entry *rtable; //structura care retine tabela de rutare
int rtable_size; //nr de intrari din tabela de rutare

struct arp_entry *arp_table; //tabela arp 
int arp_table_len; //nr de intrari din tabela arp

/*
 Returns a pointer (eg. &rtable[i]) to the best matching route
 for the given dest_ip. Or NULL if there is no matching route.
*/

//rtable este un vector de structuri de tipul route_table_entry
struct route_table_entry *get_best_route(__u32 dest_ip) {
	/* TODO 1: Implement the function */
    int i;
    int max_len = 0;
 
    struct table_entry* candidate = NULL;

    //adresa care se potiveste si are cei mai muti biti de 1
    for(i = 0; i < rtable_size; i++) {
		if(((rtable[i].mask & rtable[i].prefix) == (dest_ip & rtable[i].mask)) && (__builtin_popcount(rtable[i].mask) > max_len)) {
    		candidate = &rtable[i];
    		max_len = __builtin_popcount(rtable[i].mask);
    	}
    }
	return candidate;
}

/*
 Returns a pointer (eg. &arp_table[i]) to the best matching ARP entry.
 for the given dest_ip or NULL if there is no matching entry.
*/
struct arp_entry *get_arp_entry(__u32 ip) {
    /* TODO 2: Implement */
    int i;
    for(i=0 ; i < arp_table_len; i++) {
    	if(arp_table[i].ip == ip) {
    		return &arp_table[i];
    	}
    }
    return NULL;
}

int main(int argc, char *argv[])
{
	msg m;
	int rc;

	init();
	rtable = malloc(sizeof(struct route_table_entry) * 100);
	arp_table = malloc(sizeof(struct  arp_entry) * 100);
	DIE(rtable == NULL, "memory");
	rtable_size = read_rtable(rtable);
	parse_arp_table();
	/* Students will write code here */

	while (1) {
		rc = get_packet(&m); //routerul primeste un pachet
		DIE(rc < 0, "get_message");
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		
		/* TODO 3: Check the checksum */

		/* TODO 4: Check TTL >= 1 */

		/* TODO 5: Find best matching route (using the function you wrote at TODO 1) */

		/* TODO 6: Update TTL and recalculate the checksum */

		/* TODO 7: Find matching ARP entry and update Ethernet addresses */

		/* TODO 8: Forward the pachet to best_route->interface */

		struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
		if(best_route == NULL) //nu s-a gasit un best_route -> fac discard la pachet
  			continue;

  		//se foloseste ca sa determine exact device-ul destinatie
  		//o sa primesc acea intrare din tabela arp care care adresa ip egala cu adresa ip a destinatiei 
  		//adica adresa ip asociata statiei catre care trimit packetul
		struct arp_entry *arp = get_arp_entry(best_route->next_hop); //get matching arp entry 
 		if(arp == NULL)
			continue;

		//se completeaza header-ul ethernet al pachetului receptat
		
		//update source map address for packet
		get_interface_mac(best_route->interface, &eth_hdr->ether_shost);
		//update Ethernet addresses for destination in eth header 
		//update it with arp->mac
		memcpy(eth_hdr->ether_dhost, arp->mac, 6);

		if(ip_checksum(ip_hdr, sizeof(struct iphdr)) != 0)
			continue;	

		//update TTL
		ip_hdr->ttl -= 1;

		ip_hdr->check = 0;
		//recalculate the checksum
		if(ip_hdr->ttl >= 1) {
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));
		}
		else {
			continue;
		}

		//forward the packet to best_route->interface
		send_packet (best_route->interface, &m);

		

	}
}
