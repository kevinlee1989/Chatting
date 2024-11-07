all: chat-server

chat-server: main.c http-server.c
	 gcc -std=c11 -Wall -Wno-unused-variable -g main.c http-server.c -o chat-server


clean:
	 rm -f chat-server