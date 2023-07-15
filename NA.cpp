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
    std::string szField, field, szData, data, action, idClient, idpacket, dof;

    std::string comb_create()
    {
        return idpacket + action + dof + szField + field + szData + data;
    }

    std::string comb_read()
    {
        return idpacket + action + dof + szField + field + szData;
    }
};


void
processLine(const char* word, const char* definition, int socket, std::string bracket) 
{
    size_t wordLen = strlen(word);
    size_t definitionLen = strlen(definition);

    char* szWord = (char*)malloc(4 * sizeof(char));
    char* szDef = (char*)malloc(4 * sizeof(char));
    sprintf(szWord, "%04lu", wordLen);
    sprintf(szDef, "%04lu", definitionLen);
    //sprintf(insert, "%04d%01d%04lu%s%04lu%s", 0, 1, wordLen, word, definitionLen, definition);
    //printf("%04d%1d%04lu%s%04lu%s", 0, 1, wordLen, word, definitionLen, definition);

    write(socket, "00000000", 8);
    write(socket, "+", 1);
    write(socket, bracket.c_str(), 1);
    write(socket, szWord, 4);
    write(socket, word, wordLen);
    write(socket, szDef, 4);
    write(socket, definition, definitionLen);

    //printf("00001%s%s%s%s\n", szWord, word, szDef, definition);
}

char buf[10000];
int nbytes;
std::map<std::string, std::vector<std::string>> storageWord;
std::map<std::string, std::vector<std::string>> storageGlosa;

void 
thread_read(int i)
{
    for(;;){
        protocol Data;
        std::cout << "Entering NA" << std::endl;
        //ID
        nbytes = read(i, buf, 8);
        buf[8] = '\0';
        Data.idpacket = buf;

        //ACTION
        nbytes = read(i, buf, 1);
        buf[1] = '\0';
        Data.action = buf;
        
        //DOF
        nbytes=read(i,buf,1);
        buf[1]='\0';
        Data.dof=buf;

        std::cout<<"ID NA: "<< Data.idpacket <<std::endl;
        std::cout<<"ID Action: "<< Data.action <<std::endl;
        std::cout<<"ID Dof: "<< Data.dof <<std::endl;

        switch(Data.action[0]) {
            case '+':{
                //DATA CREATE

                std::cout<<"NA create"<<std::endl;
                //SZFIELD
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szField = buf;

                //FIELD
                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;
                
                //SZDATA
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szData = buf;

                //DATA
                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;

                std::cout<<"NA about to push"<<std::endl;

                if (Data.dof == "[") {
                    storageWord[Data.field].push_back(Data.data);
                }
                else if (Data.dof == "]") {
                    storageGlosa[Data.field].push_back(Data.data);
                }
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
                std::cout << "Field " << Data.field<<std::endl;
                std::cout << "SZField "<< Data.szField<<std::endl;
                std::string answer;
                if (Data.dof == "[") {
                    for(int i = 0; i < storageWord[Data.field].size(); i++){
                        answer += storageWord[Data.field][i] + ",";
                        std::cout << "Answer: "<<answer<<std::endl;
                    }
                } else if (Data.dof == "]") {
                    for(int i = 0; i < storageGlosa[Data.field].size(); i++){
                        answer += storageGlosa[Data.field][i] + ",";
                    }
                }
                
                
                char szAnswer[10];
                sprintf(szAnswer, "%06d", answer.length());
                std::cout << "SIZE OF DATA: " << szAnswer << std::endl;
                std::cout << "DATA: " << answer << std::endl;
                std::cout << "SOCKET: " << Data.szData << std::endl;
                
                nbytes = write(i, "000000000", 8);
                nbytes = write(i, "%", 1);
                nbytes = write(i, Data.dof.c_str(), 1);
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
                if(Data.dof == "[") {
                    for (auto i: storageWord){
                        for (int j = 0; j < i.second.size(); j++){
                            std::string a1 = i.second[j];
                            std::string a2 = Data.data;
                            if (a1 == a2) {
                                answer += "{" + i.first + ":" + a1 + "}" + ",";
                            }        
                        }      
                    }
                } else if (Data.dof == "]") {
                        for (auto i: storageGlosa){
                        for (int j = 0; j < i.second.size(); j++){
                            std::string a1 = i.second[j];
                            std::string a2 = Data.data;
                            if (a1 == a2) {
                                answer+= "{" + i.first + ":" + a1 + "}" +","; 
                            }        
                        }      
                    }
                }
                
                char szAnswer[10];
                sprintf(szAnswer, "%06d", answer.length());
                std::cout << "SIZE OF DATA: " << szAnswer << std::endl;
                std::cout << "DATA: " << answer << std::endl;
                std::cout << "SOCKET: " << Data.szData << std::endl;
                
                nbytes = write(i, "000000000", 8);
                nbytes = write(i, "(", 1);
                nbytes = write(i, Data.dof.c_str(), 1);
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
                std::cout << "New szField: "<<Data.szField<<std::endl;
                std::cout << "New Field: "<<Data.field<<std::endl;
                std::cout << "szField: "<<Data.szData<<std::endl;
                std::cout << "Field: "<<Data.data<<std::endl;
                std::cout << "NA about to push"<<std::endl;
                std::cout << "Actualizando: " << Data.comb_create() << std::endl;
                auto aux = storageWord[Data.data];


                if (Data.dof == "[") {
                    storageWord.erase(Data.data);
                } else if(Data.dof == "]"){
                    aux = storageGlosa[Data.data];
                    storageGlosa.erase(Data.data);
                }
                for (auto j: aux) {
                    processLine(Data.field.c_str(), j.c_str(), i,Data.dof);
                }
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

                if(Data.dof=="["){
                    storageWord[Data.field].erase(std::remove_if(storageWord[Data.field].begin(), storageWord[Data.field].end(), [&](std::string s){return s==Data.data;}), storageWord[Data.field].end());
                    std::cout << "Borrando: " << Data.comb_create() << std::endl;
                } else if (Data.dof=="]"){
                    storageGlosa[Data.field].erase(std::remove_if(storageGlosa[Data.field].begin(), storageGlosa[Data.field].end(), [&](std::string s){return s==Data.data;}), storageGlosa[Data.field].end()   );
                    std::cout << "Borrando: " << Data.comb_create() << std::endl;
                }

                char szAnswer[10];
                sprintf(szAnswer, "%06d", Data.field.length());
                nbytes = write(i, "000000000", 8);
                nbytes = write(i, "{", 1);
                nbytes = write(i, Data.dof.c_str(), 1);
                nbytes = write(i, szAnswer, 6);
                nbytes = write(i, Data.field.c_str(), Data.field.length());
                nbytes = write(i, Data.idClient.c_str(), 4);
                break;
            }
            case '|': {
                std::cout<<"NA update data"<<std::endl;

               //SzNewData
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szField = buf;
                std::cout << "Size NewData: " << buf << std::endl;
                
                //NewData
                nbytes = read(i, buf, stoi(Data.szField));
                buf[stoi(Data.szField)] = '\0';
                Data.field = buf;
                std::cout << "NewData: " << buf << std::endl;
                
                //SzData
                nbytes = read(i,buf,4);
                buf[4] = '\0';
                Data.szData = buf;
                std::cout << "Size Data: " << buf << std::endl;

                //Data
                nbytes = read(i, buf, stoi(Data.szData));
                buf[stoi(Data.szData)] = '\0';
                Data.data = buf;
                std::cout<<"Data: " << buf << std::endl;

                //idClient
                nbytes = read(i, buf, 4);
                buf[4] = '\0';
                Data.idClient = buf;
                std::cout<<"idClient: " << buf << std::endl;
    
                // std::string answer;
                if(Data.dof == "[") {
                    for (auto k = storageWord.begin(); k != storageWord.end(); k++) {
                        for (auto l = (*k).second.begin(); l != (*k).second.end(); l++) {
                            if (Data.data == *l) {
                                (*k).second.erase(l);
                                (*k).second.push_back(Data.field);
                            }
                        }
                    }
                } else if (Data.dof == "]") {
                    for (auto k = storageGlosa.begin(); k != storageGlosa.end(); k++) {
                        for (auto l = (*k).second.begin(); l != (*k).second.end(); l++) {
                            if (Data.data == *l) {
                                (*k).second.erase(l);
                                (*k).second.push_back(Data.field);
                            }
                        }
                    }
                }

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

    write(SocketFD, "00000000", 8);
    write(SocketFD, "@", 1);
    write(SocketFD, "[", 1);

    std::cout << "NA iniciado" << std::endl;

    for (;;)
    {
        thread_read(SocketFD);
    }

    shutdown(SocketFD, SHUT_RDWR);

    close(SocketFD);
    return 0;
}
