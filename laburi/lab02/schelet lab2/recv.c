#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc,char** argv){
  msg r;
  msg t;

  init(HOST,PORT);
  memset(r.payload, 0, MAX_LEN);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }
  printf("[recv] Got msg with payload: <%s>, sending ACK...\n", r.payload);

  strcat(r.payload, "-out");
  int fd = open(r.payload, O_WRONLY | O_CREAT, 0777);


  sprintf(t.payload,"%s", r.payload);
  t.len = strlen(t.payload)+1;
  send_message(&t);

  if (recv_message(&r) < 0){
        perror("Receive message");
        return -1;
    }
  printf("[recv] Got message with payload: %s\n", r.payload);


  sprintf(t.payload, "%s", r.payload);
  t.len = strlen(t.payload);
  send_message(&t);

  int dimension = atoi(r.payload);
  printf("Size of file :%d\n", dimension);
  while (dimension>0) {
        if (recv_message(&r) < 0) {
            perror("Receive message");
            return -1;
        }

        printf("[recv] Got message with payload: %s\n", r.payload);
        write(fd, r.payload, r.len);
        send_message(&r);
        dimension -= r.len;
        memset(t.payload, 0, MAX_LEN);
    }


/*  // Send ACK:
  sprintf(r.payload,"%s", "ACK");
  r.len = strlen(r.payload) + 1;
  send_message(&r);
  printf("[recv] ACK sent\n");
*/

  return 0;
}
