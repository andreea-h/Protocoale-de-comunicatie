#!/bin/bash

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-client-out.txt \
         ./subscriber s1 127.0.0.1 8080~               
