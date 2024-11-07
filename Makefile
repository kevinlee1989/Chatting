all: chat-server

chat-server: main.c http-server.c
	 gcc -std=c11 -Wall -Wno-unused-variable -fsanitize=address -g main.c http-server.c -o chat-server
	 ./chat-server 3001

clean:
	 rm -f chat-server