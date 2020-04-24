#ifndef LIB
#define LIB

#define MSGSIZE		1400  //dimesiunea maxima a unui cadru trimis
#define COUNT       100 //numarul de cadre trimise

typedef struct {
  int len;
  int c_sum;
  char payload[MSGSIZE];
  
} msg;

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);

#endif

