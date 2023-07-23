#include "defs.hpp"

/*
    function : user_interface()
    return: void
    description:
        - it interacts with the user and takes the filename as input to download
*/
void user_interface(std::string ClientUsername, struct sockaddr_in ServerAddress){
    while(true){
        int op;
        std:: string filename;
        std:: cout << "[>] what do you want to perform ?" << std:: endl;
        std:: cout << "1) download\n" << "2) list files\n";
        std:: cin >> op;
        
        if(op == 1){
            std:: cout << "[>] enter filename to download : ";
            std:: cin >> filename;
            try_download(filename, ClientUsername, ServerAddress);
        }else if(op == 2){
            getfilelist(ServerAddress);
        }
        else{
            std:: cout << "[-] Invalid choice. " << std:: endl;
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
               
        //  case 2:
        //        recievefile(Sock_fd);
        //        break;

         case 3:
               send_file_list(Sock_fd);
               break;
               
         default:
               std:: cout << "Invalid request" << std:: endl;
    }

    close(Sock_fd);
}


//main driver code for client
void RunClient(){
    int Client_Listen_Sockfd, Client_Request_Sockfd;
    struct sockaddr_in ClientAddress, ServerAddress;
    std::string ClientUsername;
    memset(&ServerAddress, 0, sizeof(ServerAddress));
    memset(&ClientAddress, 0, sizeof(ClientAddress));

    //create client's listening socket
    if((Client_Listen_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std:: cout << "client listen socket could not be created\n";
        exit(1);
    }

    //fillup ClientAddress to bind
    std:: string myip; int myport;
    std:: cout << "Enter ur ip: ";
    std:: cin >> myip;
    std:: cout << "Enter port number to listen: ";
    std:: cin >> myport;
    ClientAddress.sin_family = AF_INET;
    ClientAddress.sin_addr.s_addr = inet_addr(myip.c_str());
    ClientAddress.sin_port = htons(myport);    
    //bind
    if(bind(Client_Listen_Sockfd, (struct sockaddr *)&ClientAddress, sizeof(ClientAddress)) < 0){
        std:: cout << "cannot bind\n";
        exit(1);
    }
    
    std:: cout << "Listening to " << myip << ": " << myport << std:: endl;

    //create clients request socket & connect to the server to register
    if((Client_Request_Sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std:: cout << "client request socket could not be created\n";
        exit(1);
    }

    std:: string serverip; int serverport;
    std:: cout << "enter server ip: ";
    std:: cin >> serverip;
    std:: cout << "enter server port: ";
    std:: cin >> serverport;
    ServerAddress.sin_family = AF_INET;
    ServerAddress.sin_port = htons(serverport);
    ServerAddress.sin_addr.s_addr = inet_addr(serverip.c_str());
    if(connect(Client_Request_Sockfd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        std:: cout << "cannot connect to the server\n";
        exit(1);
    }

    std:: cout << "Connected successfully.......\n";
    //send register request and store ID in MyID
    int requestid = 1;
    send(Client_Request_Sockfd, &requestid, sizeof(requestid), 0);
    std:: cout << "Enter username for registration : ";
    std:: cin >> ClientUsername;
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
        std:: cout << "registration failed. try another username" << std:: endl;
        close(Client_Listen_Sockfd);
        exit(1);
    }

    std:: cout << "registration successful" << std:: endl;

    //start listening
    if(listen(Client_Listen_Sockfd, 6) < 0){
        std:: cout << "error in listening\n";
        exit(1);
    }
    std:: cout << "listening...." << std:: endl;

    //start user interface in separate thread
    std::thread thr_UI(user_interface, ClientUsername, ServerAddress);
    thr_UI.detach();


    //start accepting requests
    int tmp_sockfd;
    struct sockaddr_in tmpaddress; socklen_t addrsize;
    while(true){
        std:: cout << "\nhere.." << std:: endl;
        if((tmp_sockfd = accept(Client_Listen_Sockfd, (struct sockaddr *)&tmpaddress, &addrsize)) < 0){
            std:: cout << "cannot accept request\n";
            exit(1);
        }

        //recieve requestid
        recv(tmp_sockfd, &requestid, sizeof(requestid), 0);

        //handle request
        std:: thread thr_handlerequest(handle_request, tmp_sockfd, requestid);
        thr_handlerequest.detach();
    }

    return;
}
