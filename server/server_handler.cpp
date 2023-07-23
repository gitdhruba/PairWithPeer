#include "defs.hpp"


/*
    function --> Handle_Client_Request(int Sock_fd, int RequestID)
    return: void
    description:
         param-1: Sock_fd = new client socket file-descriptor
         param-2: RequestID = 1 -> new connection request
                  RequestID = 2 -> request to download a file

     -  handles all client requests
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

        case 3:             //request for listing all files
               Handle_List_Request(Sock_fd);
               break;
        
         case 4:            //request for socket addr of other users
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