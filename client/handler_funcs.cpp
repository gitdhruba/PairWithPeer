#include "defs.hpp"


/****************************handler functions****************************/

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
    std:: string path = "./shared/public/";
    dir = opendir(path.c_str());
    bool res = false;
    if(dir != NULL){
        for(d = readdir(dir); d != NULL; d = readdir(dir)){
            stat((path+(std:: string)d->d_name).c_str(), &dst);
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

/*
    function : send_file_list(int Sock_fd)
    return: void
    description:
        - this function sends a list of all files shared by this client
*/
void send_file_list(int Sock_fd){
    
    int datasize;

    //open searching directory and start searching
    struct dirent *d;
    struct stat dst;
    DIR *dir;
    std:: string path = "./shared/public/";
    dir = opendir(path.c_str());
    bool res = false;

    if(dir == NULL){
        send(Sock_fd, &res, sizeof(res), 0);
        return;
    }
    
    for(d = readdir(dir); d != NULL; d = readdir(dir)){
        stat((path+(std:: string)d->d_name).c_str(), &dst);
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
    std:: fstream fin;
    std:: string pathtofile = "./shared/public/" + (std:: string)filename;
    fin.open(pathtofile, std:: ios::out | std::ios::in);
    
    //calculate file size
    fin.seekg(0, std:: ios::beg);
    unsigned long long int begin = fin.tellg();
    fin.seekg(0, std:: ios::end);
    unsigned long long int end = fin.tellg();
    unsigned long long int filesize = end - begin;
    send(Sock_fd, &filesize, sizeof(filesize), 0);          //send the filesize to the client
    
    //start sending the file
    unsigned long long int cnt = filesize/PACKETSIZE, i;
    char tmp[10] = {0};
    for(i=0; i<cnt; i++){
        fin.seekg((i*PACKETSIZE), std:: ios::beg);
        fin.read(tmp, PACKETSIZE);
        //cout << tmp << endl;
        send(Sock_fd, tmp, PACKETSIZE, 0);
        memset(tmp, '\0', sizeof(tmp));
    }

    cnt = filesize % PACKETSIZE;
    fin.seekg((i*PACKETSIZE), std:: ios::beg);
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
void download(int Sock_fd, std:: string filename){

    std:: cout << "downloading.............";

    //recieve the filesize
    unsigned long long int filesize;
    recv(Sock_fd, &filesize, sizeof(filesize), 0);
    
    //prepare the output file
    std:: fstream fout;
    fout.open(filename.c_str(), std:: ios::trunc | std:: ios::out | std:: ios::in);

    
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

    std:: cout << "\ndownload completed......\n";
    std:: cout << "filename: " << filename << "; size: " << filesize << " bytes\n";
}


/*
    function : try_download(string filename)
    return: void
    description:
        - this function sends a request to the server to download a file. if server returns a response true, 
          then it fetches the seeder ip and port no, then connect with that and then calls the download()
          function.
*/    
void try_download(std:: string filename, std::string ClientUsername, struct sockaddr_in ServerAddress){

    //create a socket to communicate with the server
    int Sock_fd;
    if((Sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std:: cout << "client request socket could not be created\n";
        exit(1);
    }

    if(connect(Sock_fd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        std:: cout << "cannot connect to server\n";
        return;
    }
    
    //send request to download a file
    int request = 2;
    send(Sock_fd, &request, sizeof(request), 0);

    //send username
    int datasize = ClientUsername.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, ClientUsername.c_str(), datasize, 0);

    std:: cout << "searching for file.......\n";
    //send filename
    datasize = filename.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0);
    send(Sock_fd, filename.c_str(), datasize, 0);

    //recieve response
    bool res;
    recv(Sock_fd, &res, sizeof(res), 0);
    if(!res){
        std:: cout << "file not found\n";
        close(Sock_fd);
        return;
    }

    std:: cout << "file found\n";
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
        std:: cout << "client request socket could not be created\n";
        exit(1);
    }
    connect(Sock_fd, (struct sockaddr *)&peeraddr, sizeof(peeraddr));

    std:: cout << "connected with: " << peerip << ":"     << peerport << std:: endl;

    //download the file
    //send download request to the seeder(request = 1)
    request = 1;
    send(Sock_fd, &request, sizeof(request), 0);

    //send filename to the seeder
    datasize = filename.length();
    send(Sock_fd, &datasize, sizeof(datasize), 0    );
    send(Sock_fd, filename.c_str(), datasize, 0);
    download(Sock_fd, filename); 
    close(Sock_fd);
}


/*
    function : getfilelist()
    return: void
    description:
        - gathers list of all available files
*/  
void getfilelist(struct sockaddr_in ServerAddress){

    //connect to the server
    int sock_fd;
    if((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        std:: cout << "error occured while connecting to the server" << std:: endl;
        return;
    }

    if(connect(sock_fd, (struct sockaddr *)&ServerAddress, sizeof(ServerAddress)) < 0){
        std:: cout << "cannot connect to the server" << std:: endl;
        close(sock_fd);
        return;
    }

    //send request
    std:: cout << "sending request......" << std:: endl;
    int req = 3, datasize;
    send(sock_fd, &req, sizeof(req), 0);

    //send username
    // datasize = ClientUsername.length();    
    // send(sock_fd, &datasize, sizeof(datasize), 0);
    // send(sock_fd, ClientUsername.c_str(), datasize, 0);
 
    std:: cout << "fetching files list........." << std:: endl;
    bool next_user = false, next_file = false;
    char seedername[100], filename[100];
    recv(sock_fd, &next_user, sizeof(next_user), 0);
    if(!next_user){
        std:: cout << "no file found.." << std:: endl;
        close(sock_fd);
        return;
    }

    std:: cout << "files available in the network publicly: " << std:: endl << std:: endl;;
    while(next_user){

        memset(seedername, 0, sizeof(seedername));
        recv(sock_fd, &datasize, sizeof(datasize), 0);
        recv(sock_fd, seedername, datasize, 0);

        std:: cout << "////////// seeder: " << seedername << " \\\\\\\\\\" << std:: endl; 
        for(recv(sock_fd, &next_file, sizeof(next_file), 0); next_file; recv(sock_fd, &next_file, sizeof(next_file), 0)){
            recv(sock_fd, &datasize, sizeof(datasize), 0);
            recv(sock_fd, filename, datasize, 0);
            filename[datasize] = '\0';    
            std:: cout << "  --" << filename << std:: endl;
        }

        std:: cout << "\n";
        recv(sock_fd, &next_user, sizeof(next_user), 0);
    }

    close(sock_fd);
}