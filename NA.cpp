#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <fstream>
#include <map>
#include <vector>

using namespace std;

std::map<std::string, std::vector<std::string>> DATA;

int NAnumber;
string fileName;

struct protocol
{
    std::string szField, field, szData, data, action, idClient, idPackets;

    std::string comb_create()
    {
        return idPackets + action + szField + field + szData + data;
    }

    std::string comb_read()
    {
        return idPackets + action + szField + field;
    }
};

char buf[10000];
int nbytes;
std::map<std::string, std::vector<std::string>> storage;

void thread_read(int i)
{
    for(;;){
        // std::cout<<"NA ABOUT TO READ"<<std::endl;
        protocol Data;
        nbytes = read(i, buf, 4);
        buf[4] = '\0';
        Data.idPackets=buf;
        nbytes = read(i, buf, 1);
        buf[1] = '\0';
        Data.action = buf;

        switch(Data.action[0]) {
            case '1':
                std::cout<<"NA create"<<std::endl;
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szField = buf;

                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;
                
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szData = buf;

                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)]='\0';
                Data.data = buf;

                std::cout<<"NA about to push"<<std::endl;

                storage[Data.field].push_back(Data.data);
                std::cout << "Guardando: " << Data.comb_create() << std::endl;

                break;
        }
    }
}

int main()
{
    struct sockaddr_in stSockAddr;
    int Res;
    int SocketFD = socket(AF_INET, SOCK_STREAM, 0);
    int n;
    char buffer[256];

    if (-1 == SocketFD)
    {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(9034);
    Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);
    // Res = inet_pton(AF_INET, "172.17.1.58", &stSockAddr.sin_addr);

    if (0 > Res)
    {
        perror("error: first parameter is not a valid address family");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
        perror("char string (second parameter does not contain valid ipaddress");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    int socketClient = connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));
    // Here get connect ID, then pass it to the thread
    if (-1 == socketClient)
    {
        perror("connect failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    write(SocketFD, "0000", 4);
    write(SocketFD, "0", 1);

    std::cout << "NA iniciado" << std::endl;

    for (;;)
    {
        thread_read(SocketFD);
    }

    shutdown(SocketFD, SHUT_RDWR);

    close(SocketFD);
    return 0;
}
