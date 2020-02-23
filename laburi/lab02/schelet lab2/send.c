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
  sprintf(t.payload, "%s", "file.txt");
  t.len = strlen(t.payload)+1;
  send_message(&t);

  //verifica daca a fost primit numele fisierului
  if(recv_message(&t)<0) {
    perror("[recv] Receive error...\n");
    return -1;
  }

  //se calculeaza si se trimite dimensiunea fisierului
  memset(t.payload, 0, MAX_LEN);
  int fd = open("file.txt", O_RDONLY);
  int size_of_file = lseek(fd, 0, SEEK_END);
  sprintf(t.payload,"%d",size_of_file);
  t.len = strlen(t.payload)+1;
  send_message(&t);
  
  printf("[send] Sending dimension of file...\n");
  //verifica daca s-a trimis dimeniunea fisierului
/*  if(recv_message(&t)<0) {
    perror("[recv] Receive error ...\n");
    return -1;
  }
  else {
    printf("[send] Got reply with payload: %s\n", t.payload);
  }
  */
  lseek(fd, 0, SEEK_SET);
  //realizeaza citirea din fisier
  int size;
  memset(t.payload, 0, MAX_LEN);
  while((size = read(fd, t.payload, MAX_LEN-1))!=0) {
    if(size<0) {
      perror("Reading error");
    }
    else {
      t.len = size;
      send_message(&t);
      if(recv_message(&t)<0) {
        perror("Receive error");
        return -1;
      }
      else {
        printf("[recv] Got reply with payload %s \n", t.payload );
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
