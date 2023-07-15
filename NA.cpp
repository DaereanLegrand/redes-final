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
#include <algorithm>

using namespace std;

//std::map<std::string, std::vector<std::string>> DATA;

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

void 
thread_read(int i)
{
    for(;;){
        protocol Data;
        std::cout<<"Entering NA"<<std::endl;
        nbytes = read(i, buf, 4);
        buf[4] = '\0';
        Data.idPackets=buf;
        nbytes = read(i, buf, 1);
        buf[1] = '\0';
        Data.action = buf;

        std::cout<<"ID NA: "<<Data.idPackets<<std::endl;

        switch(Data.action[0]) {
            case '+':{
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
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;

                std::cout<<"NA about to push"<<std::endl;

                storage[Data.field].push_back(Data.data);
                std::cout << "Guardando: " << Data.comb_create() << std::endl;
            
                break;
            }
            
            case '&': {
                std::cout<<"NA read field"<<std::endl;
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.szField = buf;

                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;

                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.szData = buf;

                std::cout << "NA about to push" << std::endl;

                std::string answer;
                
                for(int i = 0; i < storage[Data.field].size(); i++){
                    answer += storage[Data.field][i] + ",";
                }
                
                char szAnswer[10];
                sprintf(szAnswer, "%06d", answer.length());
                std::cout << "SIZE OF DATA: " << szAnswer << std::endl;
                std::cout << "DATA: " << answer << std::endl;
                std::cout << "SOCKET: " << Data.szData << std::endl;
                
                nbytes = write(i, "0000", 4);
                nbytes = write(i, "%", 1);
                nbytes = write(i, szAnswer, 6);
                nbytes = write(i, answer.c_str(), answer.length());
                nbytes = write(i, Data.szData.c_str(), 4);
                break;
            }
            
            case '*': {
                std::cout<<"NA read data"<<std::endl;
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.szData = buf;

                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;

                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.szField = buf;

                std::cout << "NA about to push" << std::endl;
                std::cout<<"INItiAL DATA: "<<Data.data<<" -----------------------"<<std::endl;
                std::string answer;
                for (auto i : storage){
                    for (int j=0; j<i.second.size(); j++){
                        std::cout<<"Current data: "<<i.second[j]<<std::endl;
                        std::string a1=i.second[j];
                        std::string a2=Data.data;
                        std::cout<<a1<<" -------- "<<a2<<std::endl;
                        if("city"==a2){
                            answer+=a1+",";
                            std::cout<<"Current answer: "<<answer<<std::endl;
                        }        
                    }      
                }
                
                char szAnswer[10];
                sprintf(szAnswer, "%06d", answer.length());
                std::cout << "SIZE OF DATA: " << szAnswer << std::endl;
                std::cout << "DATA: " << answer << std::endl;
                std::cout << "SOCKET: " << Data.szData << std::endl;
                
                nbytes = write(i, "0000", 4);
                nbytes = write(i, "(", 1);
                nbytes = write(i, szAnswer, 6);
                nbytes = write(i, answer.c_str(), answer.length());
                nbytes = write(i, Data.szField.c_str(), 4);
                break;
            }

            case ')': {
                std::cout<<"NA Update field"<<std::endl;

                //SzNewfield
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szField = buf;
                
                //NewField
                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;
                
                //SzField
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szData = buf;

                //Field
                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;
                
                //idClient
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.idClient = buf;
                std::cout<<"NA about to push"<<std::endl;

                storage[Data.field]=storage[Data.data];
                storage.erase(Data.data);
                std::cout << "Actualizando: " << Data.comb_create() << std::endl;

                char szAnswer[10];
                sprintf(szAnswer, "%06d", Data.field.length());
                nbytes = write(i, "0000", 4);
                nbytes = write(i, "$", 1);
                nbytes = write(i, szAnswer, 6);
                nbytes = write(i, Data.field.c_str(), Data.field.length());
                nbytes = write(i, Data.idClient.c_str(), 4);
                break;
            }

            case '#' : {
                std::cout<<"NA Delete"<<std::endl;

                //SzField
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szField = buf;

                //Field
                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;

                //SzData
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.szData = buf;

                //Data 
                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;

                //idClient
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.idClient = buf;
                std::cout<<"NA about to push"<<std::endl;

                storage[Data.field].erase(std::remove_if(storage[Data.field].begin(), storage[Data.field].end(), [&](std::string s){return s==Data.data;}), storage[Data.field].end());
                std::cout << "Borrando: " << Data.comb_create() << std::endl;

                char szAnswer[10];
                sprintf(szAnswer, "%06d", Data.field.length());
                nbytes = write(i, "0000", 4);
                nbytes = write(i, "{", 1);
                nbytes = write(i, szAnswer, 6);
                nbytes = write(i, Data.field.c_str(), Data.field.length());
                nbytes = write(i, Data.idClient.c_str(), 4);
                break;
            }
        }
    }
}

int 
main()
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
    write(SocketFD, "@", 1);

    std::cout << "NA iniciado" << std::endl;

    for (;;)
    {
        thread_read(SocketFD);
    }

    shutdown(SocketFD, SHUT_RDWR);

    close(SocketFD);
    return 0;
}
