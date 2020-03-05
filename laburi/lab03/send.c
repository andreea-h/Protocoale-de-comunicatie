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


//aplicarea longitudinal redundancy check (LRC)
int parity_bit(unsigned char x) {
	int result = 0, i;
	for(i=0; i<8; i++) {
		result ^=(1<<i)&x;
	}
	return result;
}

int main(int argc, char *argv[])
{
	msg t, r;
	int i, result, sum;

	int correct, corrupt;
	
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	int BDP = atoi(argv[1]); //latimea de banda

	printf("[SENDER]: BDP=%d\n", atoi(argv[1])); 
	
    int window_size = (BDP *1000)/(8*MSGSIZE); //dimeniunea ferestrei

    //trimite window_size cadre fara a astepta ACK pentru acestea

    for(i = 0 ;i < window_size; i++) {
    	memset(&t, 0, sizeof(msg));
    	strcpy(t.payload, "This is the message");
    	t.len = MSGSIZE;

        //inainte de a trimite mesajul
        int j;
        t.check_sum = parity_bit(t.payload[0]);
        //se face calculul sumei de control
        for(j = 1; j < MSGSIZE; j++) {
        	t.check_sum = t.check_sum ^ parity_bit(t.payload[j]);
        }

    	result = send_message(&t);
    	if(result < 0) {
    		perror("[SENDER] Send error. Exiting.\n");
			return -1;
    	}
    }


    //trimite restul de cadre altfel: pentru fiecare ACK primit trimite mai departe inca un cadru
    for (i = 0; i < COUNT - window_size; i++){
		result = recv_message(&r);
		if (result < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}

		memset(&t, 0, sizeof(msg));

		t.len = MSGSIZE;
		strcpy(t.payload, "This is the message");
		//trimite un cadru
		int sum = parity_bit(t.payload[0]);

		for(int j=1; j<MSGSIZE; j++){
			sum = sum ^ parity_bit(t.payload[j]);
		}

		t.check_sum = parity_bit(t.payload[0]);

		for(int j = 1; j < MSGSIZE; j++) {
			sum = sum ^ parity_bit(t.payload[j]);
		}

		if(sum == t.check_sum){
			correct++;
		}
		else {
			corrupt++;
		}
		result = send_message(&t);

		if (result < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}


	//mai avem de primit ACK pentru window_size cadre
	for(i = 0; i < window_size; i++){
		result = recv_message(&r);
		if (result < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
		t.check_sum = parity_bit(t.payload[0]);

		for(int j = 1; j < MSGSIZE; j++) {
			sum = sum ^ parity_bit(t.payload[j]);
		}
        
		if(sum == t.check_sum){
			correct++;
		}
		else {
			corrupt++;
		}
			
	}

	/*
	for (i = 0; i < COUNT; i++) {
		// cleanup msg 
		memset(&t, 0, sizeof(msg));
		
		// gonna send an empty msg 
		t.len = MSGSIZE;
		
		// send msg 
		res = send_message(&t);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
		
		// wait for ACK 
		res = recv_message(&t);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
	}
    */
	printf("[SENDER] Job done, all sent.\n");
    
    printf("correct: %d\ncorrupt: %d\n", correct, corrupt);

	return 0;
}
