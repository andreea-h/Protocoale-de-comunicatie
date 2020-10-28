#ifndef LIB
#define LIB

#define MSGSIZE		1400  //dimesiunea maxima a unui cadru trimis
#define COUNT       1000 //numarul de cadre trimise

typedef struct {
  int len;
  int check_sum;
  char payload[MSGSIZE];
  
} msg;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);

#endif

