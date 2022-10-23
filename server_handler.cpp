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
#include <cstdlib>
#include <time.h>

/***************headers end********************/

using namespace std;

//define some global values
#define PORT 8080           //the server will be listening to PORT
#define PACKETSIZE 8        //data will be sent/recieved in a series of packets of size PACKETSIZE

//declare some global variables
vector<pair<string, int>> ClientsList;    //stores ip address and port numbers for each connected clients
mutex mtx;                                //for handeling critical sections



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
    return: void
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - fetches clients ip and port and add them to Clients
*/
void Register_Client(int Sock_fd){
    int datasize = 0;
    char Client_IP[32]; int Client_Port;
    memset(Client_IP, 0, sizeof(Client_IP));

    //recieve Client's IP
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Client_IP, datasize, 0);

    //recieve Client's Port no
    recv(Sock_fd, &Client_Port, sizeof(Client_Port), 0);

    //Register Client and return it an id
    //WARNING: it is a critical section . mutex should be locked before doing anything
    mtx.lock();
    ClientsList.push_back({(string)Client_IP, Client_Port});
    unsigned long int ClientID = ClientsList.size() - 1;
    send(Sock_fd, &ClientID, sizeof(ClientID), 0);
    cout << "Client connected: ID: " << ClientID << " IP: " << Client_IP << " Port: " << Client_Port << endl;
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
    unsigned long int ClientID;
    char Filename[1000];
    memset(Filename, 0, sizeof(Filename));
    recv(Sock_fd, &ClientID, sizeof(ClientID), 0);
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, Filename, datasize, 0);

    //create temporary socket for searching 
    if((Temp_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return false;
    }

    //initiate a random starting point for searching
    unsigned long int StartingIndex;
    srand(time(NULL));
    StartingIndex = rand();
    StartingIndex = StartingIndex % ClientsList.size();

    //start searching
    for(unsigned long int Offset = 0; Offset < ClientsList.size(); Offset++){
        unsigned long int Index = (StartingIndex + Offset) % ClientsList.size();
        if(Index != ClientID){                                                  //don't search in same client
            //fill-up Client_Address for connecting
            Client_Address.sin_family = AF_INET;
            Client_Address.sin_addr.s_addr = inet_addr(ClientsList[Index].first.c_str());
            Client_Address.sin_port = htons(ClientsList[Index].second);

            //connect
            if(connect(Temp_Sockfd, (struct sockaddr *)&Client_Address, sizeof(Client_Address)) < 0){
                  continue;
            }

            //send filename
            datasize = strlen(Filename);
            send(Temp_Sockfd, &datasize, sizeof(datasize), 0);
            send(Temp_Sockfd, Filename, datasize, 0);

            //get response
            recv(Temp_Sockfd, &Response, sizeof(Response), 0);
            if(Response){                                               //file is present
                //forward response to the client
                send(Sock_fd, &Response, sizeof(Response), 0);

                //send ip and port no of the seeding client
                datasize = ClientsList[Index].first.length();
                send(Sock_fd, &datasize, sizeof(datasize), 0);
                send(Sock_fd, ClientsList[Index].first.c_str(), datasize, 0);
                send(Sock_fd, &ClientsList[Index].second, sizeof(ClientsList[Index].second), 0);

                //all done. no need to check further
                close(Temp_Sockfd);
                return true;
            }
        }
    }

    /* rest part yet to be written */
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