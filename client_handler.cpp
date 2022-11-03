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
string ClientUsername;                         //MyID holdes Client's ID after connecting to the network
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
    string path = "./shared/public/";
    dir = opendir(path.c_str());
    bool res = false;
    if(dir != NULL){
        for(d = readdir(dir); d != NULL; d = readdir(dir)){
            stat((path+(string)d->d_name).c_str(), &dst);
            if(S_ISDIR(dst.st_mode)){
                continue;
            }
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


void send_file_list(int Sock_fd){
    
    int datasize;

    //open searching directory and start searching
    struct dirent *d;
    struct stat dst;
    DIR *dir;
    string path = "./shared/public/";
    dir = opendir(path.c_str());
    bool res = false;

    if(dir == NULL){
        send(Sock_fd, &res, sizeof(res), 0);
        return;
    }
    
    for(d = readdir(dir); d != NULL; d = readdir(dir)){
        stat((path+(string)d->d_name).c_str(), &dst);
            if(S_ISDIR(dst.st_mode)){
                continue;
            }

        res = true;
        send(Sock_fd, &res, sizeof(res), 0);
        datasize = strlen(d->d_name);
        send(Sock_fd, &datasize, sizeof(datasize), 0);
        send(Sock_fd, d->d_name, datasize, 0);
    }

    res = false;
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
void sharefile_public(int Sock_fd){

    //recieve the filename
    int datasize ;
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    char filename[100];
    memset(filename, 0, sizeof(filename));
    recv(Sock_fd, filename, datasize, 0);
    //cout << filename << " requested\n";

    //open and prepare the requested file
    fstream fin;
    string pathtofile = "./shared/public/" + (string)filename;
    fin.open(pathtofile, ios::out | ios::in);
    
    //calculate file size
    fin.seekg(0, ios::beg);
    unsigned long long int begin = fin.tellg();
    fin.seekg(0, ios::end);
    unsigned long long int end = fin.tellg();
    unsigned long long int filesize = end - begin;
    send(Sock_fd, &filesize, sizeof(filesize), 0);          //send the filesize to the client
    
    //start sending the file
    unsigned long long int cnt = filesize/PACKETSIZE, i;
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

void sharefile_private(string username, string filename){
    int sock_fd, datasize, req;
    char clientip[32]; int clientport;
    struct sockaddr_in clientaddress;
    memset(clientip, 0, sizeof(clientip));
    memset(&clientaddress, 0, sizeof(clientaddress));

    //gather info about user
    cout << "gathering info........." << endl;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "an unexpected error occured. try again later" << endl;
        return;
    }

    if(connect(sock_fd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        cout << "cannot connect to the server" << endl;
        return;
    }

    //send request to the server
    req = 4;
    send(sock_fd, &req, sizeof(req), 0);
    datasize = username.length();
    send(sock_fd, &datasize, sizeof(datasize), 0);
    send(sock_fd, username.c_str(), datasize, 0);
    bool res;
    recv(sock_fd, &res, sizeof(res), 0);
    if(!res){
        cout << "no client found with this username" << endl;
        close(sock_fd);
        return;
    }

    //fetch client's ip and port no
    recv(sock_fd, &datasize, sizeof(datasize), 0);
    recv(sock_fd, clientip, datasize, 0);
    recv(sock_fd, &clientport, sizeof(clientport), 0);
    close(sock_fd);

    cout << "connecting......." << endl;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "an unexpected error occured. try again later" << endl;
        return;
    }
    clientaddress.sin_family = AF_INET;
    clientaddress.sin_addr.s_addr = inet_addr(clientip);
    clientaddress.sin_port = htons(clientport);                   
    if(connect(sock_fd, (struct sockaddr *)&clientaddress, sizeof(clientaddress)) < 0){
        cout << "cannot connect to the client" << endl;
        return;
    }

    cout << "sending request........" << endl;
    req = 2;
    send(sock_fd, &req, sizeof(req), 0);
    datasize = username.length();
    send(sock_fd, &datasize, sizeof(datasize), 0);
    send(sock_fd, username.c_str(), datasize, 0);
    datasize = filename.length();
    send(sock_fd, &datasize, sizeof(datasize), 0);
    send(sock_fd, filename.c_str(), datasize, 0);
    recv(sock_fd, &res, sizeof(res), 0);
    if(!res){
        cout << "declined by " << username << endl;
        close(sock_fd);
        return;
    }

    //start sharing
    fstream fin;
    string pathtofile = "./shared/private/" + filename;
    fin.open(pathtofile, ios::out | ios::in);
    
    //calculate file size
    fin.seekg(0, ios::beg);
    unsigned long long int begin = fin.tellg();
    fin.seekg(0, ios::end);
    unsigned long long int end = fin.tellg();
    unsigned long long int filesize = end - begin;
    send(sock_fd, &filesize, sizeof(filesize), 0);          //send the filesize to the client
    
    //start sending the file
    unsigned long long int cnt = filesize/PACKETSIZE, i;
    char tmp[10] = {0};
    for(i=0; i<cnt; i++){
        fin.seekg((i*PACKETSIZE), ios::beg);
        fin.read(tmp, PACKETSIZE);
        //cout << tmp << endl;
        send(sock_fd, tmp, PACKETSIZE, 0);
        memset(tmp, '\0', sizeof(tmp));
    }

    cnt = filesize % PACKETSIZE;
    fin.seekg((i*PACKETSIZE), ios::beg);
    fin.read(tmp, cnt);
    send(sock_fd, tmp, cnt, 0);

    //close the file
    fin.close();

    cout << "file shared successfully......." << endl;
    close(sock_fd);
}

/*
    function : download(int Sock_fd, string filename)
    return: void
    description:
         - this function is called by try_download() if it has successfully found out the file in the 
           network, with the socket_fd with which the seeder is connected and the file name.
*/
void download(int Sock_fd, string filename){

    cout << "downloading.............";

    //recieve the filesize
    unsigned long long int filesize;
    recv(Sock_fd, &filesize, sizeof(filesize), 0);
    
    //prepare the output file
    fstream fout;
    fout.open(filename.c_str(), ios::trunc | ios::out | ios::in);

    
    //download file
    unsigned long long int cnt = filesize/PACKETSIZE, i;
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

    cout << "\ndownload completed......\n";
    cout << "filename: " << filename << "; size: " << filesize << " bytes\n";
}


void recievefile(int Sock_fd){
    int datasize; bool res;
    char username[100], filename[100];
    memset(username, 0, sizeof(username));
    memset(filename, 0, sizeof(filename));

    //recieve username and filename
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, username, datasize, 0);
    recv(Sock_fd, &datasize, sizeof(datasize), 0);
    recv(Sock_fd, filename, datasize, 0);

    //ask user
    fprintf(stdin, "%s", "n\n");
    cout << "\n[>] " << username << " wants to share " << filename << " with you" << endl;
    cout << "  do you agree ?(y / n): ";
    char choice;
    cin >> choice;
    if(choice == 'n'){
        res = false;
        send(Sock_fd, &res, sizeof(res), 0);
        return;
    }

    res = true;
    send(Sock_fd, &res, sizeof(res), 0);
    download(Sock_fd, (string)filename);
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

    //send username
    int datasize = ClientUsername.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, ClientUsername.c_str(), datasize, 0);

    cout << "searching for file.......\n";
    //send filename
    datasize = filename.length();
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

    cout << "file found\n";
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

    //download the file
    //send download request to the seeder(request = 1)
    request = 1;
    send(Sock_fd, &request, sizeof(request), 0);

    //send filename to the seeder
    datasize = filename.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, filename.c_str(), datasize, 0);
    download(Sock_fd, filename); 
    close(Sock_fd);
}


void getfilelist(){

    //connect to the server
    int sock_fd;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << "error occured while connecting to the server" << endl;
        return;
    }

    if(connect(sock_fd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        cout << "cannot connect to the server" << endl;
        close(sock_fd);
        return;
    }

    //send request
    cout << "sending request......" << endl;
    int req = 3, datasize;
    send(sock_fd, &req, sizeof(req), 0);

    //send username
    datasize = ClientUsername.length();
    send(sock_fd, &datasize, sizeof(datasize), 0);
    send(sock_fd, ClientUsername.c_str(), datasize, 0);
 
    cout << "fetching files list........." << endl;
    bool next_user = false, next_file = false;
    char seedername[100], filename[100];
    recv(sock_fd, &next_user, sizeof(next_user), 0);
    if(!next_user){
        cout << "no file found.." << endl;
        close(sock_fd);
        return;
    }

    cout << "files available in the network publicly: " << endl << endl;;
    while(next_user){

        memset(seedername, 0, sizeof(seedername));
        recv(sock_fd, &datasize, sizeof(datasize), 0);
        recv(sock_fd, seedername, datasize, 0);

        cout << "////////// seeder: " << seedername << " \\\\\\\\\\" << endl; 
        for(recv(sock_fd, &next_file, sizeof(next_file), 0); next_file; recv(sock_fd, &next_file, sizeof(next_file), 0)){
            recv(sock_fd, &datasize, sizeof(datasize), 0);
            recv(sock_fd, filename, datasize, 0);
            filename[datasize] = '\0';
            cout << "  --" << filename << endl;
        }

        cout << endl;
        recv(sock_fd, &next_user, sizeof(next_user), 0);
    }

    close(sock_fd);
}

/*
    function : user_interface()
    return: void
    description:
        - it interacts with the user and takes the filename as input to download
*/
void user_interface(){
    while(true){
        int op;
        string filename;
        cout << "[>] what do you want to perform ?" << endl;
        cout << "1) share\n" << "2) download\n" << "3) list files\n" << "4) refresh screen\n";
        cin >> op;
        if(op == 1){
            cout << "[>] enter filename to share : ";
            cin >> filename;
            string uname;
            cout << "[>] enter username for sharing with : ";
            cin >> uname;
            sharefile_private(uname, filename);
        }else if(op == 2){
            cout << "[>] enter filename to download : ";
            cin >> filename;
            try_download(filename);
        }else if(op == 3){
            getfilelist();
        }else if(op == 4){
            continue;
        }
        else{
            cout << "[-] Invalid choice. " << endl;
        }
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
               sharefile_public(Sock_fd);
               break;
               
         case 2:
               recievefile(Sock_fd);
               break;

         case 3:
               send_file_list(Sock_fd);
               break;
               
         default:
               cout << "Invalid request" << endl;
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
    string myip; int myport;
    cout << "Enter ur ip: ";
    cin >> myip;
    cout << "Enter port number to listen: ";
    cin >> myport;
    ClientAddress.sin_family = AF_INET;
    ClientAddress.sin_addr.s_addr = inet_addr(myip.c_str());
    ClientAddress.sin_port = htons(myport);
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
    cout << "Enter username for registration : ";
    cin >> ClientUsername;
    int datasize = ClientUsername.length();
    send(Client_Request_Sockfd, &datasize, sizeof(datasize), 0);
    send(Client_Request_Sockfd, ClientUsername.c_str(), datasize, 0);
    datasize = myip.length();
    send(Client_Request_Sockfd, &datasize, sizeof(datasize), 0);
    send(Client_Request_Sockfd, myip.c_str(), datasize, 0);
    send(Client_Request_Sockfd, &myport, sizeof(myport), 0);
    //recv(Client_Request_Sockfd, &MyID, sizeof(MyID), 0);
    bool res;
    recv(Client_Request_Sockfd, &res, sizeof(res), 0);
    close(Client_Request_Sockfd);
    //cout << "connected with ID: " << MyID << endl;
    if(!res){
        cout << "registration failed. try another username" << endl;
        close(Client_Listen_Sockfd);
        exit(1);
    }

    cout << "registration successful" << endl;

    //start listening
    if(listen(Client_Listen_Sockfd, 6) < 0){
        cout << "error in listening\n";
        exit(1);
    }
    cout << "listening...." << endl;
    //start user interface
    thread thr_UI(user_interface);
    thr_UI.detach();


    //start accepting requests
    int tmp_sockfd;
    struct sockaddr_in tmpaddress; socklen_t addrsize;
    while(true){
        cout << "\nhere.." << endl;
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
