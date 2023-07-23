#include "defs.hpp"


//declare some global variables
std::map<std::string, std::pair<std::string, int>> ClientsList;           //stores ip address and port numbers for each connected clients
std::mutex mtx;                                       //for handeling critical sections


/****************************handler functions****************************/


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


/*
    function --> Handle_List_Request(int Sock_fd)
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - sends list of all files shared publicly in the network
*/
void Handle_List_Request(int Sock_fd){
    int datasize = 0;
    int temp_sockfd;
    bool next_user, next_file;
    char filename[100];
    struct sockaddr_in seeder_addr;
    
    //get username
    // recv(Sock_fd, &datasize, sizeof(datasize), 0);
    // recv(Sock_fd, username, datasize, 0);

    std::map<std::string, std::pair<std::string, int>> :: iterator itr;
    for(itr = ClientsList.begin(); itr != ClientsList.end(); itr++, next_user = false){

        // if(itr->first == (std::string)username){
        //     continue;
        // }

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


/*
    function --> Provide_UserInfo(int Sock_fd)
    description:
         param-1: Sock_fd = new client socket file-descriptor
      - provides socket address of the resquested user
*/
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