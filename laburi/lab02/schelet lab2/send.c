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

  //Send dummy message:
/*  printf("[send] Sending dummy...\n");
  sprintf(t.payload,"%s", "This is a dummy.");
  t.len = strlen(t.payload)+1;
  send_message(&t);
 */ 
  //trimite numele fisieului

  printf("[send] Sending name of file...\n");
  memset(t.payload, 0, MAX_LEN);
  sprintf(t.payload, "file.txt");
  t.len = strlen(t.payload)+1;
  send_message(&t);

  //verifica daca a fost primit numele fisierului
  if(recv_message(&t)<0) {
    perror("[recv] Receive error...\n");
    return -1;
  }
  else{
    printf("[%s] Got reply with payload: ACK( %s )\n ", argv[0], t.payload);
  }

  //se calculeaza si se trimite dimensiunea fisierului
  memset(t.payload, 0, MAX_LEN);
  int fd = open("file.txt", O_RDONLY);
  int size_of_file = lseek(fd, 0, SEEK_END);
  sprintf(t.payload,"%d",size_of_file);
  t.len = strlen(t.payload)+1;

  send_message(&t); //trimite dimensiunea fisierului
  lseek(fd, 0, SEEK_SET); //reseteaza pozitia FP la inceputul fisierului

  printf("[%s] Sending dimension of file...\n", argv[0]);
  //verifica daca s-a trimis dimensiunea fisierului
  if(recv_message(&t)<0) {
    perror("[recv] Receive error ...\n");
    return -1;
  }
  else {
    printf("[%s] Got reply with payload: ACK ( %s )\n", argv[0], t.payload);
  }
  
 //realizeaza citirea din fisier
 
  int size;
  memset(t.payload, 0, MAX_LEN);
  while((size = read(fd, t.payload, MAX_LEN-1))!=0) {
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
        printf("[%s] Got reply with payload: ACK ( %s )\n", argv[0], t.payload);
      }
     memset(t.payload, 0, MAX_LEN);
   }
  }

  close(fd);

/*
  // Check response:
  if (recv_message(&t)<0){
    perror("Receive error ...");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }
*/
  return 0;
}
