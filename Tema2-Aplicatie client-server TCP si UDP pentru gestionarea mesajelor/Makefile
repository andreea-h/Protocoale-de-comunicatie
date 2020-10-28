PORT=8080
IP_ADDR=127.0.0.1

build:
	gcc server.c -o server -lm -g
	gcc subscriber.c -o subscriber -g
run_server:
	./server ${PORT}
run_subscriber:
	./subscriber $(id_client) ${IP_ADDR} ${PORT}
