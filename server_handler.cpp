/***************headers start******************/
// #include <iostream>
// #include <vector>
// #include <string>
// #include <map>
// //#include <stdlib.h>
// #include <cstring>
//#include <bits/stdc++.h>
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
#include <time.h>
#include<string.h>
#include<cstring>
#include<iostream>
#include<map>

/***************headers end********************/


//define some global values
#define PORT 8080        //the server will be listening to PORT
#define PACKETSIZE 8        //data will be sent/recieved in a series of packets of size PACKETSIZE

//declare some global variables
std::map<std::string, std::pair<std::string, int>> ClientsList;           //stores ip address and port numbers for each connected clients
//std::map<std::pair<std::string, int>, std::string> AddrTable;
std::mutex mtx;                                       //for handeling critical sections



/****************************handler functions****************************/
/*
    function --> getip()
    return: the public ip as a string
*/
std::string getip(){
    FILE *f = popen("ip a | grep 'scope global' | grep -v ':' | awk '{print $2}' | cut -d '/' -f1", "r");
    char c; std::string s = "";
    while ((c = getc(f)) != EOF) 
         s += c;
    pclose(f);
    std::cout << s << std::endl;
    return s;
}


/*
    function --> Register_Client(int Sock_fd)
    return: void
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - fetches clients ip and port and add them to Clients
*/
void Register_Client(int Sock_fd){
    int datasize = 0;
    char Client_Username[100], Client_IP[32]; int Client_Port;
    memset(Client_Username, 0, sizeof(Client_Username));
    memset(Client_IP, 0, sizeof(Client_IP));

    //recieve Client's Username
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Client_Username, datasize, 0);

    //recieve Client's IP
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Client_IP, datasize, 0);

    //recieve Client's Port no
    recv(Sock_fd, &Client_Port, sizeof(Client_Port), 0);

    //Register Client and return it an id
    //WARNING: it is a critical section . mutex should be locked before doing anything
    mtx.lock();
    
    std::string username = (std::string)Client_Username;
    std::pair<std::string, int> clientinfo;
    clientinfo.first = (std::string)Client_IP;
    clientinfo.second = Client_Port;
    std::map<std::string, std::pair<std::string, int>> :: iterator it;
    it = ClientsList.find(username);
    bool res;
    if(it != ClientsList.end()){
        res = false;
        send(Sock_fd, &res, sizeof(res), 0);
        mtx.unlock();
        return;
    }

    ClientsList[username] = clientinfo;
    res = true;
    send(Sock_fd, &res, sizeof(res), 0);

    for(auto it : ClientsList){
        std::cout << it.first << "--" << it.second.first << ":" << it.second.second << std::endl;
    }

    mtx.unlock();                                     //critical section over , unlock mutex
}


/*
    function --> Handle_Download_Request(int Sock_fd)
    return: true on success
            false on failure
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - fetches the file name that the client wants to download, then forwards a request to each
        connected client if it has that file or not. if any client responds to the request, then 
        it sends that clients ip and port no to the first client , so that the client can connect 
        to that and download the file, else it searches for that file in itself . if it is available
        then sends its ip and port to the client .
*/
bool Handle_Download_Request(int Sock_fd){
    int datasize = 0;
    int Temp_Sockfd;
    struct sockaddr_in Client_Address;
    bool Response;
    memset(&Client_Address, 0, sizeof(Client_Address));
    
    //recieve client's id and filename
    //unsigned long int ClientID;
    char Filename[100], Username[100];
    memset(Username, 0, sizeof(Username));
    memset(Filename, 0, sizeof(Filename));
    //recv(Sock_fd, &ClientID, sizeof(ClientID), 0);
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Username, datasize, 0);
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Filename, datasize, 0);

    std::string username = (std::string)Username;
    std::map<std::string, std::pair<std::string, int>> :: iterator it;
    it = ClientsList.find(username);
    it++;
    while(it->first != username){
        (it == ClientsList.end() ? it = ClientsList.begin() : it = it);

        Client_Address.sin_family = AF_INET;
        Client_Address.sin_addr.s_addr = inet_addr(it->second.first.c_str());
        Client_Address.sin_port = htons(it->second.second);

        //create temporary socket for searching 
        if((Temp_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
              continue;
        }
        //connect
        if(connect(Temp_Sockfd, (struct sockaddr *)&Client_Address, sizeof(Client_Address)) < 0){
              it++;
              continue;
        }
            
        //send request
        int request = 0;
        send(Temp_Sockfd, &request, sizeof(request), 0);
        //send filename
        datasize = strlen(Filename);
        send(Temp_Sockfd, &datasize, sizeof(datasize), 0);
        send(Temp_Sockfd, Filename, datasize, 0);

        recv(Temp_Sockfd, &Response, sizeof(Response), 0);

        if(Response){                                               //file is present
                //forward response to the client
                send(Sock_fd, &Response, sizeof(Response), 0);

                //send ip and port no of the seeding client
                datasize = it->second.first.length();
                send(Sock_fd, &datasize, sizeof(datasize), 0);
                send(Sock_fd, it->second.first.c_str(), datasize, 0);
                send(Sock_fd, &(it->second.second), sizeof(it->second.second), 0);

                //all done. no need to check further
                close(Temp_Sockfd);
                return true;
        }
    
        close(Temp_Sockfd);
        it++;
    }

    return false;
}


void Handle_List_Request(int Sock_fd){
    int datasize = 0;
    int temp_sockfd;
    bool next_user, next_file;
    char username[100], filename[100];
    struct sockaddr_in seeder_addr;
    memset(username, 0, sizeof(username));
    
    //get username
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, username, datasize, 0);

    std::map<std::string, std::pair<std::string, int>> :: iterator itr;
    for(itr = ClientsList.begin(); itr != ClientsList.end(); itr++, next_user = false){

        if(itr->first == (std::string)username){
            continue;
        }

        memset(&seeder_addr, 0, sizeof(seeder_addr));
        if((temp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            next_user = false;
            send(Sock_fd, &next_user, sizeof(next_user), 0);
            std::cout << "an unexpexted error occured.." << std::endl;
            return;
        }

        seeder_addr.sin_family = AF_INET;
        seeder_addr.sin_addr.s_addr = inet_addr((itr->second.first).c_str());
        seeder_addr.sin_port = htons(itr->second.second);

        if(connect(temp_sockfd, (struct sockaddr *)&seeder_addr, sizeof(seeder_addr)) < 0){
            next_user = false;
            send(Sock_fd, &next_user, sizeof(next_user), 0);
            std::cout << "an unexpexted error occured.." << std::endl;
            return;
        }

        next_user = true;
        send(Sock_fd, &next_user, sizeof(next_user), 0);

        datasize = itr->first.size();
        send(Sock_fd, &datasize, sizeof(datasize), 0);
        send(Sock_fd, (itr->first).c_str(), datasize, 0);
        
        int req = 3;
        send(temp_sockfd, &req, sizeof(req), 0);
        datasize = 100;
        for(recv(temp_sockfd, &next_file, sizeof(next_file), 0); next_file; recv(temp_sockfd, &next_file, sizeof(next_file), 0)){
            memset(filename, 0, datasize);
            recv(temp_sockfd, &datasize, sizeof(datasize), 0);
            recv(temp_sockfd, filename, datasize, 0);
            send(Sock_fd, &next_file, sizeof(next_file), 0);
            send(Sock_fd, &datasize, sizeof(datasize), 0);
            send(Sock_fd, filename, datasize, 0);
        }

        send(Sock_fd, &next_file, sizeof(next_file), 0);
        close(temp_sockfd);
    }

    send(Sock_fd, &next_user, sizeof(next_user), 0);
}


void Provide_UserInfo(int Sock_fd){
    int datasize;
    char username[100];
    memset(username, 0, sizeof(username));
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, username, datasize, 0);

    bool res = true;
    std::map<std::string, std::pair<std::string, int>> :: iterator itr;
    itr = ClientsList.find((std::string)username);
    if(itr == ClientsList.end()){
        res = false;
        send(Sock_fd, &res, sizeof(res), 0);
        return;
    }

    send(Sock_fd, &res, sizeof(res), 0);
    std::string ip = itr->second.first;
    int port = itr->second.second;
    datasize = ip.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, ip.c_str(), datasize, 0);
    send(Sock_fd, &port, sizeof(port), 0);
}

/*
    function --> Handle_Client_Request(int Sock_fd, int RequestID)
    return: void
    description:
         param-1: Sock_fd = new client socket file-descriptor
         param-2: RequestID = 1 -> new connection request
                  RequestID = 2 -> request to download a file
*/
void Handle_Client_Request(int Sock_fd, int RequestID){
    switch(RequestID){
        case 1:             //new connection request
              Register_Client(Sock_fd);
              break;

        case 2:             //request to download a file
              if(!Handle_Download_Request(Sock_fd)){
                 bool status = false;
                 send(Sock_fd, &status, sizeof(status), 0);
              }
              break;

        case 3:
               Handle_List_Request(Sock_fd);
               break;
        
         case 4:
               Provide_UserInfo(Sock_fd);
               break;

        default:
              std::cout << "Invalid request\n";
    }

    //close socket
    close(Sock_fd);
    return;
}



/*driver code for the server*/
void RunServer(){
    int Server_Sockfd, New_Client_Sockfd;
    struct sockaddr_in Server_Address, New_Client_Address;
    socklen_t addr_size;

    memset(&Server_Address, 0, sizeof(Server_Address));     //clear Server_Address

    //create socket
    if((Server_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std::cout << "server socket could not be created\n";
        exit(1);
    }

    //fill-up Server_Address for binding
    std::string ipaddr = getip();
    Server_Address.sin_family = AF_INET;
    Server_Address.sin_port = htons(PORT);
    Server_Address.sin_addr.s_addr = inet_addr(ipaddr.c_str());

    //bind
    if(bind(Server_Sockfd, (struct sockaddr *)&Server_Address, sizeof(Server_Address)) < 0){
        std::cout << "cannot bind\n";
        exit(1);
    }

    //start listening
    if(listen(Server_Sockfd, 6) < 0){
        std::cout << "cannot listen\n";
        exit(1);
    }
    std::cout << "Listening to " << ipaddr << ": " << PORT << std::endl;
    //accept requests from clients and make separate threads to handle them
    while(true){
        if((New_Client_Sockfd = accept(Server_Sockfd, (struct sockaddr *)&New_Client_Address, &addr_size)) < 0){
            std::cout << "client connection could not be established\n";
            exit(1);
        }

        int RequestID;
        recv(New_Client_Sockfd, &RequestID, sizeof(RequestID), 0);
        std::cout << RequestID << std::endl;
        //create thread to handle request
        std::thread thr(Handle_Client_Request, New_Client_Sockfd, RequestID);
        thr.detach();
    }

    return;

}