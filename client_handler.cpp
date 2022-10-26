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
#define SERVERPORT 8080           //the server will be listening to SERVERPORT
#define PACKETSIZE 8              //data will be sent/recieved in a series of packets of size PACKETSIZE

//declare some global variables
unsigned long int MyID;                         //MyID holdes Client's ID after connecting to the network
struct sockaddr_in ServerAddress;               //ServerAddress holds Server's socket address




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
    function : searchfile(int Sock_fd)
    return: void
    description:
       - this function is called when server asks for if a file is present or not. this function recieves
         the filename from the server and searches in its current directory. if it is present , function 
         sends response as true, otherwise false.
*/
void searchfile(int Sock_fd){

    //recieve filename
    int datasize;
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    char filename[100];
    memset(filename, 0, sizeof(filename));
    recv(Sock_fd, filename, datasize, 0);

    //open searching directory and start searching
    struct dirent *d;
    struct stat dst;
    DIR *dir;
    string path = "./";
    dir = opendir(path.c_str());
    
    bool res = false;
    if(dir != NULL){
        for(d = readdir(dir); d != NULL; d = readdir(dir)){
            if(!strcmp(d->d_name, filename)){
                res = true;
                break;
            }
        }
    }

    //searching complete . send response to the server
    send(Sock_fd, &res, sizeof(res), 0);
    closedir(dir);
}


/*
    function : sharefile(int Sock_fd)
    return: void
    description:
        - this function is called when a client requests for a file to download. it shares the requested
          file in a series of packets of size PACKETSIZE.
*/
void sharefile(int Sock_fd){

    //recieve the filename
    int datasize ;
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    char filename[100];
    memset(filename, 0, sizeof(filename));
    recv(Sock_fd, filename, datasize, 0);
    //cout << filename << " requested\n";

    //open and prepare the requested file
    fstream fin;
    fin.open(filename, ios::out | ios::in);
    
    //calculate file size
    fin.seekg(0, ios::beg);
    int begin = fin.tellg();
    fin.seekg(0, ios::end);
    int end = fin.tellg();
    int filesize = end - begin;
    send(Sock_fd, &filesize, sizeof(filesize), 0);          //send the filesize to the client
    
    //start sending the file
    int cnt = filesize/PACKETSIZE, i;
    char tmp[10] = {0};
    for(i=0; i<cnt; i++){
        fin.seekg((i*PACKETSIZE), ios::beg);
        fin.read(tmp, PACKETSIZE);
        //cout << tmp << endl;
        send(Sock_fd, tmp, PACKETSIZE, 0);
        memset(tmp, '\0', sizeof(tmp));
    }

    cnt = filesize % PACKETSIZE;
    fin.seekg((i*PACKETSIZE), ios::beg);
    fin.read(tmp, cnt);
    send(Sock_fd, tmp, cnt, 0);

    //close the file
    fin.close();
}


/*
    function : download(int Sock_fd, string filename)
    return: void
    description:
         - this function is called by try_download() if it has successfully found out the file in the 
           network, with the socket_fd with which the seeder is connected and the file name.
*/
void download(int Sock_fd, string filename){

    //send download request to the seeder(request = 1)
    int request = 1;
    send(Sock_fd, &request, sizeof(request), 0);

    //send filename to the seeder
    int datasize = filename.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, filename.c_str(), datasize, 0);

    //recieve the filesize
    int filesize;
    recv(Sock_fd, &filesize, sizeof(filesize), 0);
    cout << filename << endl;
    cout << "filesize: " << filesize << endl;

    //prepare the output file
    fstream fout;
    fout.open(filename.c_str(), ios::trunc | ios::out | ios::in);

    //download file
    int cnt = filesize/PACKETSIZE, i;
    char tmp[10] = {0};
    for(i=0; i<cnt; i++){
        recv(Sock_fd, tmp, PACKETSIZE, 0);
        fout.write(tmp, PACKETSIZE);
        memset(tmp, 0, sizeof(tmp));
    }

    cnt = filesize % PACKETSIZE;
    recv(Sock_fd, tmp, cnt, 0);
    fout.write(tmp, cnt);
    fout.close();                               //close the file
}

/*
    function : try_download(string filename)
    return: void
    description:
        - this function sends a request to the server to download a file. if server returns a response true, 
          then it fetches the seeder ip and port no, then connect with that and then calls the download()
          function.
*/
void try_download(string filename){

    //create a socket to communicate with the server
    int Sock_fd;
    if((Sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "client request socket could not be created\n";
        exit(1);
    }

    if(connect(Sock_fd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        cout << "cannot connect to server\n";
        return;
    }
    
    //send request to download a file
    int request = 2;
    send(Sock_fd, &request, sizeof(request), 0);

    //send client ID
    send(Sock_fd, &MyID, sizeof(MyID), 0);

    //send filename
    int datasize = filename.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, filename.c_str(), datasize, 0);

    //recieve response
    bool res;
    recv(Sock_fd, &res, sizeof(res), 0);
    if(!res){
        cout << "file not found\n";
        close(Sock_fd);
        return;
    }

    //recieve seeder's ip and port no
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    char peerip[32]; int peerport;
    memset(peerip, 0, sizeof(peerip));
    recv(Sock_fd, peerip, datasize, 0);
    recv(Sock_fd, &peerport, sizeof(peerport), 0);
    close(Sock_fd);

    //connect with the seeder
    struct sockaddr_in peeraddr;
    memset(&peeraddr, 0, sizeof(peeraddr));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(peerport);
    peeraddr.sin_addr.s_addr = inet_addr(peerip);
    if((Sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "client request socket could not be created\n";
        exit(1);
    }
    connect(Sock_fd, (struct sockaddr *)&peeraddr, sizeof(peeraddr));

    cout << "connected with: " << peerip << ":" << peerport << endl;
    cout << "Downloading.......\n";

    //download the file
    download(Sock_fd, filename); 
    close(Sock_fd);
}


/*
    function : user_interface()
    return: void
    description:
        - it interacts with the user and takes the filename as input to download
*/
void user_interface(){
    while(true){
        string filename;
        //cin.ignore(numeric_limits<streamsize>::max(),'\n');
        cout << "enter filename to download: ";
        //cin.ignore(numeric_limits<streamsize>::max(),'\n');
        cin >> filename;
        try_download(filename);
    }
}

/*
    function : handle_request(int Sock_fd, int requestid)
    return: void
    description:
        - it handles several requests from server and other clients
*/
void handle_request(int Sock_fd, int requestid){
    switch (requestid){
         case 0:                    //it is a server request(server wants to search a file)
               searchfile(Sock_fd);
               break;
         
         case 1:                    //it is a client request to download a file
               sharefile(Sock_fd);
               break;
    }

    close(Sock_fd);
}


//main driver code for client
void RunClient(){
    int Client_Listen_Sockfd, Client_Request_Sockfd;
    struct sockaddr_in ClientAddress;
    memset(&ServerAddress, 0, sizeof(ServerAddress));
    memset(&ClientAddress, 0, sizeof(ClientAddress));

    //create client's listening socket
    if((Client_Listen_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "client listen socket could not be created\n";
        exit(1);
    }

    //fillup ClientAddress to bind
    string myip = getip(); int myport;
    cout << "Enter port number to listen: ";
    cin >> myport;
    ClientAddress.sin_family = AF_INET;
    ClientAddress.sin_addr.s_addr = inet_addr(myip.c_str());
    ClientAddress.sin_port = htons(9001);
    //bind
    if(bind(Client_Listen_Sockfd, (struct sockaddr *)&ClientAddress, sizeof(ClientAddress)) < 0){
        cout << "cannot bind\n";
        exit(1);
    }
    
    cout << "Listening to " << myip << ": " << myport << endl;

    //create clients request socket & connect to the server to register
    if((Client_Request_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "client request socket could not be created\n";
        exit(1);
    }

    string serverip;
    cout << "enter server ip: ";
    cin >> serverip;
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(SERVERPORT);
    ServerAddress.sin_addr.s_addr = inet_addr(serverip.c_str());
    if(connect(Client_Request_Sockfd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        cout << "cannot connect to the server\n";
        exit(1);
    }

    cout << "Connected successfully.......\n";
    //send register request and store ID in MyID
    int requestid = 1;
    send(Client_Request_Sockfd, &requestid, sizeof(requestid), 0);
    int datasize = myip.length();
    send(Client_Request_Sockfd, &datasize, sizeof(datasize), 0);
    send(Client_Request_Sockfd, myip.c_str(), datasize, 0);
    send(Client_Request_Sockfd, &myport, sizeof(myport), 0);
    recv(Client_Request_Sockfd, &MyID, sizeof(MyID), 0);
    close(Client_Request_Sockfd);
    cout << "connected with ID: " << MyID << endl;

    //start user interface
    thread thr_UI(user_interface);
    thr_UI.detach();

    //start listening
    if(listen(Client_Listen_Sockfd, 6) < 0){
        cout << "error in listening\n";
        exit(1);
    }

    //start accepting requests
    int tmp_sockfd;
    struct sockaddr_in tmpaddress; socklen_t addrsize;
    while(true){
        if((tmp_sockfd = accept(Client_Listen_Sockfd, (struct sockaddr *)&tmpaddress, &addrsize)) < 0){
            cout << "cannot accept request\n";
            exit(1);
        }

        //recieve requestid
        recv(tmp_sockfd, &requestid, sizeof(requestid), 0);

        //handle request
        thread thr_handlerequest(handle_request, tmp_sockfd, requestid);
        thr_handlerequest.detach();
    }

    return;
}
