# target: server
server: common.o server_main.o server_handler.o server_handler_funcs.o 
	g++ -o serverex getip.o server_main.o server_handler.o handler_funcs.o && rm *.o

server_main.o:
	g++ -c ./server/server_main.cpp

server_handler.o:
	g++ -c ./server/server_handler.cpp

server_handler_funcs.o:
	g++ -c ./server/handler_funcs.cpp


# target: client
client: common.o client_main.o client_handler.o client_handler_funcs.o 
	g++ -o clientex getip.o client_main.o client_handler.o handler_funcs.o && rm *.o

client_main.o:
	g++ -c ./client/client_main.cpp

client_handler.o:
	g++ -c ./client/client_handler.cpp

client_handler_funcs.o:
	g++ -c ./client/handler_funcs.cpp



# common:
common.o:
	g++ -c ./common/getip.cpp