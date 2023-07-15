/*
** selectserver.c -- a cheezy multiperson chat server
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <queue>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>

#define PORT "9034" // port we're listening on
// get sockaddr, IPv4 or IPv6:

std::mutex mtx;

struct protocol
{
    std::string szField, field, szData, data, action, idClient, idpacket, dof;

    std::string comb_create()
    {
        return idpacket + action + dof + szField + field + szData + data;
    }

    std::string comb_readF()
    {
        return idpacket + action + dof + szField + field + szData;
    }

    std::string comb_readD()
    {
        return idpacket + action + dof + szData + data + szField;
    }

    std::string comb_updateF()
    {
        return idpacket + action + dof + szField + field + szData + data + idClient;
    }
};

int id_counter = 0;
int NA_num = 0;

std::hash<std::string> hshr;
std::map<int, std::queue<protocol>> queues;
std::map<int, std::queue<std::string>> DataRU;
std::vector<int> NAsocket;
std::map<int,bool> NAstatus;
int totalWord, totalGlosa;

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void 
write_(int socket)
{
    while (true) {
        mtx.lock();
        if (!queues[socket].empty()) {
            protocol Data = queues[socket].front();
            queues[socket].pop();
            int id;
            if (Data.action == "+") {
                std::cout << "CREATE About to find id" << std::endl;
                id = NAsocket[hshr(Data.field) % NAsocket.size()];
                std::cout << "Sending to " << NAsocket[hshr(Data.field) % NAsocket.size()] << std::endl;
                write(id, Data.comb_create().c_str(), Data.comb_create().size());
                id = NAsocket[(hshr(Data.field)+1) % NAsocket.size()];
                write(id, Data.comb_create().c_str(), Data.comb_create().size());
                std::cout << "Sent!\n";
            } else if (Data.action == "&") {
                std::cout << "READ About to find id" << std::endl;
                if(NAstatus[socket]){ id = NAsocket[hshr(Data.field) % NAsocket.size()];}
                else { id = NAsocket[(hshr(Data.field) + 1) % NAsocket.size()];}
                std::cout << "Sending to " << id << std::endl;
                write(id, Data.comb_readF().c_str(), Data.comb_readF().size());
                std::cout << "Sent!\n";
            } else if (Data.action=="*") {
                for(int i=0; i<NAsocket.size(); i++){
                    std::cout << "Sending to " << NAsocket[i] << std::endl;
                    write(NAsocket[i], Data.comb_readD().c_str(), Data.comb_readD().size());
                    std::cout << "Sent!\n";
                }
            } else if (Data.action==")"){
                std::cout << "UPDATE About to find id" << std::endl;
                id = NAsocket[hshr(Data.data) % NAsocket.size()];
                std::cout << "Sending to " << id << std::endl;
                write(id, Data.comb_updateF().c_str(), Data.comb_updateF().size());
                id = NAsocket[(hshr(Data.data) + 1) % NAsocket.size()];
                std::cout << "And sending to " << id << std::endl;
                write(id, Data.comb_updateF().c_str(), Data.comb_updateF().size());
                std::cout << "Sent!\n";
            } else if (Data.action=="#"){
                std::cout << "Delete About to find id" << std::endl;
                if(NAstatus[socket]){ id = NAsocket[hshr(Data.field) % NAsocket.size()];}
                else {id = NAsocket[(hshr(Data.field) + 1) % NAsocket.size()];}
                std::cout << "Sending to " << id << std::endl;
                write(id, Data.comb_updateF().c_str(), Data.comb_updateF().size());
                std::cout << "Sent!\n";
            } else if (Data.action=="|"){
                for(int i=0; i<NAsocket.size(); i++){
                    std::cout << "Sending to " << NAsocket[i] << std::endl;
                    write(NAsocket[i], Data.comb_updateF().c_str(), Data.comb_updateF().size());
                    std::cout << "Sent!\n";
                }
            }
        }
        mtx.unlock();
    }
}

int main(void)
{
    fd_set master;                      // master file descriptor list
    fd_set read_fds;                    // temp file descriptor list for select()
    int fdmax;                          // maximum file descriptor number
    int listener;                       // listening socket descriptor
    int newfd;                          // newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    char buf[999999]; // buffer for client protocol
    int nbytes;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    struct addrinfo hints, *ai, *p;
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0)
    {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }

    for (p = ai; p != NULL; p = p->ai_next)
    {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0)
        {
            continue;
        }
        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
        {
            close(listener);
            continue;
        }
        break;
    }
    // if we got here, it means we didn't get bound
    if (p == NULL)
    {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    freeaddrinfo(ai); // all done with this
    // listen
    if (listen(listener, 10) == -1)
    {
        perror("listen");
        exit(3);
    }
    // add the listener to the master set
    FD_SET(listener, &master);
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one

    std::vector<std::thread> thrds;

    // main loop

    for (;;)
    {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(4);
        }
        // run through the existing connections looking for protocol to read
        for (i = 0; i <= fdmax; i++)
        {
            if (FD_ISSET(i, &read_fds))
            { // we got one!!
                if (i == listener)
                {
                    // handle new connections
                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                                   (struct sockaddr *)&remoteaddr,
                                   &addrlen);
                    if (newfd == -1)
                    {
                        perror("accept");
                    }
                    else
                    {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax)
                        { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on "
                               "socket %d\n",
                               inet_ntop(remoteaddr.ss_family,
                                         get_in_addr((struct sockaddr *)&remoteaddr),
                                         remoteIP, INET6_ADDRSTRLEN),
                               newfd);

                        // thread aquí
                        // queues[newfd].push("");
                        //queues[newfd].push(protocol());
                        //queues[newfd].pop();
                        thrds.push_back(std::thread(write_, newfd));
                    }
                }
                else
                {
                    // handle protocol from a client
                    if ((nbytes = read(i, buf, 8)) <= 0)
                    {
                        // got error or connection closed by client

                        for(int j = 0; j<NAsocket.size(); j++){
                            if(i == NAsocket[j]){
                                NAstatus[i] = false;
                                std::cout << "ERASING NA: " << i << std::endl;
                                break;
                            }
                        }    
                        
                        if (nbytes == 0)
                        {
                        }
                        else
                        {
                            perror("recv");
                        }
                        close(i);           // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else
                    {
                        protocol Data;
                        buf[8] = '\0';

                        nbytes = read(i, buf, 1);
                        // std::string strid=std::to_string(id_counter++);
                        // strid.insert(0,4-strid.length(),'0');
                        buf[1] = '\0';
                        Data.action = buf;

                        nbytes = read(i, buf, 1);
                        buf[1] = '\0';
                        Data.dof = buf;

                        Data.idpacket ="00000000";

                        std::cout << "Entering action: " << Data.action << std::endl;

                        switch(Data.action[0]) {
                            case '@': {
                                NAsocket.push_back(i);
                                NAstatus[i] = true;
                                printf("NA creado, socket %d\n", i);
                                break;
                            }
                            case '+': {
                                // std::cout<<"Entrando a case 1"<<std::endl;
                                //Data.action = "1";

                                mtx.lock();

                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "Field: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Size data: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout<<"Data: " << buf << std::endl;
                                
                                if (Data.dof == "[") {
                                    char szId[10];
                                    sprintf(szId, "%08d", totalWord++);
                                    Data.idpacket = szId;
                                } else if (Data.dof == "]") {
                                    char szId[10];
                                    sprintf(szId, "%08d", totalGlosa++);
                                    Data.idpacket = szId;
                                }
                                std::cout<<"before pushing"<<std::endl;
                                queues[i].push(Data);

                                mtx.unlock();
                                break;
                            }
                            case '&': {
                                mtx.lock();
                                std::cout << "READ FROM CLIENT: " << std::endl;

                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "Field: " << buf << std::endl;
                                
                                // nbytes = read(i, buf, 4);
                                sprintf(buf, "%04d", i);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Socket: " << buf << std::endl;
                                
                                std::cout<<"before pushing"<<std::endl;
                                queues[i].push(Data);

                                mtx.unlock();
                                break;
                            }
                            case '*': {
                                mtx.lock();

                                //READ DATA
                                Data.action="*";
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Size data: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout<<"Data: " << buf << std::endl;

                                // nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                sprintf(buf, "%04d", i);
                                Data.szField = buf;
                                std::cout << "Socket: " << buf << std::endl;
                                
                                std::cout<<"before pushing"<<std::endl;
                                queues[i].push(Data);

                                mtx.unlock();
                                break;
                            }
                            case '%': {
                                mtx.lock();
                                std::cout << "READ FIELD FROM NA: " << std::endl;

                                nbytes = read(i, buf, 6);
                                buf[6] = '\0';
                                Data.szField = buf;
                                //std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                std::cout << "Size field: " << szF << std::endl;

                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "Field: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Socket: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                
                                //std::cout << "before pushing" << std::endl;
                                //queues[i].push(Data);

                                write(szD, "0000", 4);
                                write(szD, "%", 1);
                                write(szD, Data.szField.c_str(), 6);
                                write(szD, Data.field.c_str(), stoi(Data.szField));
                                    
                                std::cout << Data.data << std::endl;

                                mtx.unlock();
                                break;
                            }
                            case '(': {
                                mtx.lock();
                                std::cout << "READ FROM NA: " << std::endl;

                                nbytes = read(i, buf, 6);
                                buf[6] = '\0';
                                Data.szData = buf;
                                //std::cout << "Size field: " << buf << std::endl;

                                int szD = stoi(Data.szData);
                                std::cout << "Size data: " << szD << std::endl;

                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout << "Data: " << buf << std::endl;
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Socket: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szF = stoi(Data.szField);
                                
                                //std::cout << "before pushing" << std::endl;
                                //queues[i].push(Data);
                                DataRU[szF].push(Data.data);
                                if(DataRU[szF].size()==NAsocket.size()){
                                    
                                    std::string answer;
                                    while(!DataRU[szF].empty()){
                                        answer+=DataRU[szF].front();
                                        DataRU[szF].pop();
                                    }
                                    
                                    char szAnswer[10];
                                    sprintf(szAnswer, "%06d", answer.length());
                                    write(szF, "00000000", 8);
                                    write(szF, "%", 1);
                                    write(szF, szAnswer, 6);
                                    write(szF, answer.c_str(), answer.length());
                                }
                                    

                                mtx.unlock();
                                break;
                            }
                            case ')': {
                                mtx.lock();
                                
                                //UPDATE 
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Size Newfield: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "NewField: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Size Field: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout<<"Field: " << buf << std::endl;
                                
                                sprintf(buf, "%04d", i);
                                Data.idClient = buf;
                                
                                std::cout<<"before pushing"<<std::endl;
                                std::cout<<"Data.Field "<<Data.field<<std::endl;
                                std::cout<<"Data.action "<<Data.action<<std::endl;
                                queues[i].push(Data);
                                mtx.unlock();
                                break;
                            }
                            case '$':{
                                mtx.lock();
                                std::cout << "Update FROM NA: " << std::endl;

                                nbytes = read(i, buf, 6);
                                buf[6] = '\0';
                                Data.szField = buf;
                                //std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                std::cout << "Size Newfield: " << szF << std::endl;

                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "NewField: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Socket: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                
                                //std::cout << "before pushing" << std::endl;
                                //queues[i].push(Data);

                                write(szD, "00000000", 8);
                                write(szD, "$", 1);
                                write(szD, Data.szField.c_str(), 6);
                                write(szD, Data.field.c_str(), stoi(Data.szField));
                                    
                                std::cout << " updated field: "<<Data.field << std::endl;

                                mtx.unlock();
                                break;
                            }
                            case '|': {
                                mtx.lock();
                                
                                //UPDATE 
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Size NewData: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "NewData: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Size Data: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout<<"Data: " << buf << std::endl;
                                
                                sprintf(buf, "%04d", i);
                                Data.idClient = buf;
                                
                                std::cout<<"before pushing"<<std::endl;

                                queues[i].push(Data);
                                mtx.unlock();
                                break;
                            }
                            case '#': {
                                mtx.lock();
                                
                                //DELETE
                                std::cout<<"DELETING"<<std::endl;
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szField = buf;
                                std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "Field: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Size Data: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                nbytes = read(i, buf, szD);
                                buf[szD] = '\0';
                                Data.data = buf;
                                std::cout<<"Data: " << buf << std::endl;
                                
                                sprintf(buf, "%04d", i);
                                Data.idClient = buf;
                                
                                std::cout<<"before pushing"<<std::endl;
                                queues[i].push(Data);

                                mtx.unlock();
                                break;
                            }
                            
                            case '{': {
                               mtx.lock();
                                std::cout << "Delete FROM NA: " << std::endl;

                                nbytes = read(i, buf, 6);
                                buf[6] = '\0';
                                Data.szField = buf;
                                //std::cout << "Size field: " << buf << std::endl;

                                int szF = stoi(Data.szField);
                                std::cout << "Size field: " << szF << std::endl;

                                nbytes = read(i, buf, szF);
                                buf[szF] = '\0';
                                Data.field = buf;
                                std::cout << "Field: " << buf << std::endl;
                                
                                nbytes = read(i, buf, 4);
                                buf[4] = '\0';
                                Data.szData = buf;
                                std::cout << "Socket: " << buf << std::endl;

                                //std::cout << "SZDATA: " << Data.szData << std::endl;
                                int szD = stoi(Data.szData);
                                
                                //std::cout << "before pushing" << std::endl;
                                //queues[i].push(Data);

                                write(szD, "00000000", 8);
                                write(szD, "{", 1);
                                write(szD, Data.szField.c_str(), 6);
                                write(szD, Data.field.c_str(), stoi(Data.szField));
                                    
                                std::cout << " deleted field: "<<Data.field << std::endl;

                                mtx.unlock();
                                break;
                            }

                        }

                    }
                } // END handle protocol from client
            }     // END got new incoming connection
        }         // END looping through file descriptors
    }             // END for(;;)--and you thought it would never end!
    return 0;
}
