//client header
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

#define PACKETSIZE 8        //data will be sent/recieved in a series of packets of size PACKETSIZE

//initialisation for the client
void RunClient();


//handlers
void getfilelist(struct sockaddr_in);
void try_download(std::string, std::string,  struct sockaddr_in);
void searchfile(int);
void sharefile_public(int);
void send_file_list(int);

//common
std::string getip();