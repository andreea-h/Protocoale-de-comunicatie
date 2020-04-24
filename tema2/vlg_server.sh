#!/bin/bash


valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
	 --tool=memcheck \
         --log-file=valgrind-server-out.txt \
         ./server 8080
