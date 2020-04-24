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
  init(HOST,PORT);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }
  printf("[recv] Got msg with payload: <%s>, sending ACK...\n", r.payload);

  // Send ACK:
  sprintf(r.payload,"%s", r.payload);
  r.len = strlen(r.payload) + 1;
  send_message(&r);
  printf("[recv] ACK sent\n");

  //deschide fisierul in care va fi scris textul primit
  strcpy(r.payload, "primit.bin"); //numele fisierului in care se scrie textul primit
  int fd = open(r.payload, O_WRONLY | O_CREAT, 0777);

  //primeste dimensiunea fisierului
  if(recv_message(&r)<0) {
    perror("Receive message");
    return -1;
  }
  printf("[recv] Got msg with payload: <%s>, sending ACK...\n", r.payload);
  
  //trimite ACK pentru primirea dimeniunii fisierului  
  msg t;
  sprintf(t.payload, "%s", r.payload);
  t.len = strlen(t.payload)+1;
  send_message(&t); //trimite validarea primirii dimensiunii
  
  int dimension_of_file = atoi(t.payload);
  
  while(dimension_of_file>0) { //atat timp cat mai ramane de citit text din fisier
    if(recv_message(&r)<0) {
      perror("Receive error\n");
    }
    else {
      printf("[recv] Got message with payload: %s\n",r.payload);
      write(fd, r.payload, r.len); //scrie mesajul in fisier
      send_message(&r); //trimite validarea 
      dimension_of_file = dimension_of_file - r.len;
      memset(t.payload, 0, MAX_LEN);
    }
  }

  close(fd);

  return 0;
}
