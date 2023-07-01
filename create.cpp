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
    std::string szField, field, szData, data, action, idClient, idpacket;

    std::string comb_create()
    {
        return idpacket + action + szField + field + szData + data;
    }

    std::string comb_read()
    {
        return idpacket + action + szField + field;
    }
};

int id_counter = 0;
int NA_num = 0;

std::hash<std::string> hshr;
std::vector<std::thread> thrds;
std::map<int, std::queue<protocol>> queues;
std::vector<int> NAsocket;

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
    std::cout << "Before while \n" << queues[socket].size() << std::endl;
    while (true) {
        mtx.lock();
        if (!queues[socket].empty()) {
            protocol Data = queues[socket].front();
            queues[socket].pop();

            std::cout << "About to find id" << std::endl;
            int id = NAsocket[hshr(Data.field) % NAsocket.size()];
            std::cout << "Sending to " << NAsocket[hshr(Data.field) % NAsocket.size()] << std::endl;
            write(id, Data.comb_create().c_str(), Data.comb_create().size());
            std::cout << "Sent!\n";
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
    char buf[1024]; // buffer for client protocol
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
                    if ((nbytes = read(i, buf, 4)) <= 0)
                    {
                        // got error or connection closed by client
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
                        buf[4] = '\0';

                        nbytes = read(i, buf, 1);
                        // std::string strid=std::to_string(id_counter++);
                        // strid.insert(0,4-strid.length(),'0');
                        Data.idpacket = "0000";
                        buf[1] = '\0';
                        Data.action = buf;

                        switch(Data.action[0]) {
                            case '0':
                                NAsocket.push_back(i);
                                printf("NA creado, socket %d\n", i);
                                break;
                            case '1':
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
                                
                                std::cout<<"before pushing"<<std::endl;
                                queues[i].push(Data);

                                mtx.unlock();
                                break;
                        }

                    }
                } // END handle protocol from client
            }     // END got new incoming connection
        }         // END looping through file descriptors
    }             // END for(;;)--and you thought it would never end!
    return 0;
}
