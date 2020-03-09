#!/bin/bash

cd $(dirname $0)../

valgrind --log-file=./valgrind_report.log --leak-check=full --show-leak-kinds=all --show-reachable=yes \
--track-origins=yes ./bin/Server -n namesrv:9876 -h 127.0.0.1 -p 8080