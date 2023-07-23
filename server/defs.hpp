//server header 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <netdb.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <string.h>
#include <iostream>
#include <map>

#define PORT 8080           //the server will be listening to PORT
#define PACKETSIZE 8        //data will be sent/recieved in a series of packets of size PACKETSIZE

//initialisation for the server
void RunServer();

//handler functions
void Register_Client(int);   
bool Handle_Download_Request(int);
void Handle_List_Request(int);
void Provide_UserInfo(int);

//common
std::string getip();