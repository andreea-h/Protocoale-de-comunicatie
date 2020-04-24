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

int byte_parity(unsigned char x) {
	int rez = 0;
	for(int i=0; i<8; i++)
		if(x & (1 << i))
			rez ^= 1;
	return rez;
}

int main(int argc, char *argv[])
{
	msg t, r;
	int i, res, correct, corrupt;

	correct = corrupt = 0;
	
	printf("[SENDER] Starting.\n");	
	init(HOST, PORT);

	printf("[SENDER]: BDP=%d\n", atoi(argv[1]));

	int w = atoi(argv[1])*1000/(MSGSIZE*8);
	//printf("%d\n", w);

	for (i = 0; i < w; i++){
		memset(&t, 0, sizeof(msg));
		strcpy(t.payload, "This is the message");

		t.len = MSGSIZE;

		t.c_sum = byte_parity(t.payload[0]);

		for(int j = 1; j < MSGSIZE; j++) {
			t.c_sum = t.c_sum ^ byte_parity(t.payload[j]);
		}

		res = send_message(&t);

		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}

	}

	for (i = 0; i < COUNT - w; i++){
		res = recv_message(&r);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}

		int sum = byte_parity(r.payload[0]);

		for(int j=1; j<MSGSIZE; j++){
			sum = sum ^ byte_parity(r.payload[j]);
		}
		memset(&t, 0, sizeof(msg));

		t.len = MSGSIZE;
		strcpy(t.payload, "This is the message");

		t.c_sum = byte_parity(t.payload[0]);

		for(int j = 1; j < MSGSIZE; j++) {
			t.c_sum = t.c_sum ^ byte_parity(t.payload[j]);
		}

		if(sum == t.c_sum){
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

	for(i = 0; i < w; i++){
		res = recv_message(&r);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}

		int sum = byte_parity(r.payload[0]);

		for(int j=1; j<MSGSIZE; j++){
			sum = sum ^ byte_parity(r.payload[j]);
		}

		if(sum == t.c_sum){
			correct++;
		}
		else {
			corrupt++;
		}
			
	}
		
	
	/*	//WAIT STOP
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
		}`
		
		// wait for ACK
		res = recv_message(&t);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
	}*/

	printf("[SENDER] Job done, all sent.\n");

	printf("correct: %d\ncorrupt: %d\n", correct, corrupt);
		
	return 0;
}
