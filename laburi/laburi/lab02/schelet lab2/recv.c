#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001


int main(int argc,char** argv){
  msg r; //receiver
  msg t; //transmit

  //prin t se trimite validarea primirii unui mesaj, construit cu acelasi payload ca mesajul initial
  init(HOST,PORT);

  memset(r.payload, 0, MAX_LEN);

  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }
  printf("[%s] Got msg with payload: <%s>, sending ACK...\n", argv[0], r.payload);

  //trimite numele fisierului .out
  strcat(r.payload, ".out"); //concateneaza la numele fisierului '.out'
  int fd = open(r.payload, O_WRONLY | O_CREAT, 0777);

  //transmite validarea primirii mesajului (trimite in msg chiar mesajul care tocmai a fost primit)

  sprintf(t.payload, "%s", r.payload);
  t.len = strlen(t.payload);
  send_message(&t); 
  
  //primeste dimensiunea fisierului
  if (recv_message(&r) < 0){
        perror("Receive message");
        return -1;
  }
  printf("[%s] Got message with payload: %s\n", argv[0], r.payload);
  
  //trimite validarea primirii dimensiunii
  sprintf(t.payload,"%s", r.payload);
  t.len = strlen(t.payload)+1;
  send_message(&t);

  int dimension = atoi(r.payload);
  printf("Size of file :%d\n", dimension);

  while (dimension>0) {
      //primeste treptat bucati din fisierul fragmentat in send.c
        if (recv_message(&r) < 0) {
            perror("Receive message");
            return -1;
        }
        //continutul efectiv este in 'r.payload'
        printf("[%s] Got message with payload: %s\n",argv[0], r.payload);
        write(fd, r.payload, r.len);
        send_message(&r); //trimite validarea 
        dimension -= r.len;
        memset(t.payload, 0, MAX_LEN);
    }

    close(fd);
/*  // Send ACK:
  sprintf(r.payload,"%s", "ACK");
  r.len = strlen(r.payload) + 1;
  send_message(&r);
  printf("[recv] ACK sent\n");
*/

  return 0;
}
