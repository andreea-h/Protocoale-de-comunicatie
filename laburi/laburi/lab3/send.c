#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

//paritatea unui octet
int byte_parity(unsigned char x) {
	int rez = 0;
	int i;
	for(i=0; i < 8; i++) {
		if(x& (1<<i)) {
			rez ^= 1;
		}
	}
	return rez;
}

int main(int argc, char *argv[])
{
	msg t, r;
	int i, res;
	int corrupt = 0;
	int correct = 0;
	
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	int BDP = atoi(argv[1]);
	printf("[SENDER]: BDP=%d\n", atoi(argv[1])); 

	int window_size = (BDP*1000)/(sizeof(msg)*8); //dimensiunea ferestrei
	printf("[sender] Nr frames: %d\n", window_size);

    //trimite window_size cadre fara a astepta ACK pentru acestea
	for(i = 0; i < window_size ; i++) {

		memset(&t, 0, sizeof(msg));
	    sprintf(t.payload, "%s", "This is the message");
		t.len = MSGSIZE;
		//aplica xor pe mesaj
		t.check_sum = byte_parity(t.payload[0]);

		int j;
		for(j = 1; j < MSGSIZE; j++) {
			t.check_sum = t.check_sum ^ byte_parity(t.payload[j]);
		}
		res = send_message(&t); 
		if(res < 0) {
			printf("[sender] Sending message error\n");
			return -1;
		}

	}


	//trimite restul cadrelor (adica COUNT - window_size cadre) care trebuie trimise
	for (i = 0; i < COUNT - window_size; i++){
		res = recv_message(&r);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}

		memset(&t, 0, sizeof(msg));

		t.len = MSGSIZE;
		strcpy(t.payload, "This is the message");
		//trimite un cadru
		int sum = byte_parity(t.payload[0]);

		for(int j=1; j<MSGSIZE; j++){
			sum = sum ^ byte_parity(t.payload[j]);
		}

		t.check_sum = byte_parity(t.payload[0]);

		for(int j = 1; j < MSGSIZE; j++) {
			t.check_sum = t.check_sum ^ byte_parity(t.payload[j]);
		}

		if(sum == t.check_sum){
			correct++;
		}
		else {
			corrupt++;
		}
		res = send_message(&t);

		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}



	//mai sunt de primit window_size ACK-uri de primit, corespunzatoare primelor window_size care nu au priimit ACK
	for(i = 0; i < window_size; i++) {
		res = recv_message(&t);
		if(res < 0) {
			printf("[sender] Receive ack error\n");
			return -1;
		}
	}


/*	
	for (i = 0; i < COUNT; i++) {
		// cleanup msg 
		memset(&t, 0, sizeof(msg));
		
		//gonna send an empty msg 
		t.len = MSGSIZE;
		
		// send msg 
		res = send_message(&t);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
		
		 wait for ACK 
		res = recv_message(&t);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
	}
*/
	printf("[SENDER] Job done, all sent.\n");
	printf("Correct: %d\nCorrupt: %d\n", correct, corrupt);	
	return 0;
}
