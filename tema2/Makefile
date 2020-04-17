PORT=8080
IP_ADDR=127.0.0.1

build:
	gcc publisher.c -o publisher
	gcc server.c -o server
	gcc subscriber.c -o subscriber
run_publisher:
	./publisher ${IP_ADDR} ${PORT}
run_server:
	./server ${PORT}
run_subscriber:
	./subscriber s1 ${IP_ADDR} ${PORT}
