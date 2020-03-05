#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc,char** argv){
  init(HOST,PORT);
  msg t;

  //trimite numele fisierului
  printf("[send] Sending name of file...\n");
  sprintf(t.payload, "%s", "file.bin");
  t.len = strlen(t.payload)+1;
  send_message(&t);

  //verifica daca a fost primit numele fisierului
  if(recv_message(&t)<0) {
    perror("Receive error");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }

  //trimite dimensiunea fisierului
  printf("[send] Sending dimension of file...\n");
  memset(t.payload, 0, MAX_LEN);
  int fd = open("file.bin",0,O_RDONLY);
  int size_of_file = lseek(fd, 0, SEEK_END);
//  printf("%d\n",size_of_file );
  sprintf(t.payload, "%d", size_of_file);
  t.len = strlen(t.payload)+1;
  send_message(&t);

  //verifica daca s-a primit dimensiunea fisieului
  if(recv_message(&t)<0) { //vezi daca se primeste ACK
    perror("Receive error");
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }
  
  lseek(fd, 0, SEEK_SET); //repozitioneaza fp la inceputul fisierului
  memset(t.payload, 0, MAX_LEN);
  //trimite continutul fisierului
  printf("[send] Sending content of file...\n");
  
  int size;
  while((size = read(fd, t.payload, MAX_LEN))>0) {
    if(size<0) {
      perror("Reading error");
    }
    else {
      t.len = size;
      send_message(&t); //trimite 'bucata' din fisier 
      if(recv_message(&t)<0) {
        perror("Receive error");
        return -1;
      }
      else {
        printf("[send] Got reply with payload: ACK - message (%s)\n",t.payload);
      }
     memset(t.payload, 0, MAX_LEN);
   }
  }


  return 0;
}
