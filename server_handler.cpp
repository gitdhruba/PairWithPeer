/***************headers start******************/
#include <bits/stdc++.h>
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

/***************headers end********************/

using namespace std;

//define some global values
#define PORT 8080           //the server will be listening to PORT
#define PACKETSIZE 8        //data will be sent/recieved in a series of packets of size PACKETSIZE

//declare some global variables
vector<pair<string, int>> Clients;    //stores ip address and port numbers for each connected clients
mutex mtx;                            //for handeling critical sections



/****************************handler functions****************************/

/*
    function --> getip()
    return: the public ip as a string
*/
string getip(){
    FILE *f = popen("ip a | grep 'scope global' | grep -v ':' | awk '{print $2}' | cut -d '/' -f1", "r");
    char c; string s = "";
    while ((c = getc(f)) != EOF) 
         s += c;
    pclose(f);
    cout << s << endl;
    return s;
}

/*
    function --> Register_Client(int Sock_fd)
    return: true on success
            false on failure
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - fetches clients ip and port and add them to Clients
*/
bool Register_Client(int Sock_fd){
    /*yet to be written*/
}


/*
    function --> Handle_Download_Request(int Sock_fd)
    return: void
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - fetches the file name that the client wants to download, then forwards a request to each
        connected client if it has that file or not. if any client responds to the request, then 
        it sends that clients ip and port no to the first client , so that the client can connect 
        to that and download the file. 
*/
void Handle_Download_Request(int Sock_fd){
    /*yet to be written*/
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
              if(!(Register_Client(Sock_fd))){
                 cout << "cannot register client\n";
              }
              break;

        case 2:             //request to download a file
              Handle_Download_Request(Sock_fd);
              break;

        default:
              cout << "Invalid request\n";
    }
}



/*driver code for the server*/
void RunServer(){
    int Server_Sockfd, New_Client_Sockfd;
    struct sockaddr_in Server_Address, New_Client_Address;
    socklen_t addr_size;

    memset(&Server_Address, 0, sizeof(Server_Address));     //clear Server_Address

    //create socket
    if((Server_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "server socket could not be created\n";
        exit(1);
    }

    //fill-up Server_Address for binding
    string ipaddr = getip();
    Server_Address.sin_family = AF_INET;
    Server_Address.sin_port = htons(PORT);
    Server_Address.sin_addr.s_addr = inet_addr(ipaddr.c_str());

    //bind
    if(bind(Server_Sockfd, (struct sockaddr *)&Server_Address, sizeof(Server_Address)) < 0){
        cout << "cannot bind\n";
        exit(1);
    }

    //start listening
    if(listen(Server_Sockfd, 6) < 0){
        cout << "cannot listen\n";
        exit(1);
    }

    //accept requests from clients and make separate threads to handle them
    while(true){
        if((New_Client_Sockfd = accept(Server_Sockfd, (struct sockaddr *)&New_Client_Address, &addr_size)) < 0){
            cout << "client connection could not be established\n";
            exit(1);
        }

        int RequestID;
        recv(New_Client_Sockfd, &RequestID, sizeof(RequestID), 0);

        //create thread to handle request
        thread thr(Handle_Client_Request, New_Client_Sockfd, RequestID);
        thr.detach();
    }

    return;

}