#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <string>

void
processLine(const char* word, const char* definition, int socket) 
{
    size_t wordLen = strlen(word);
    size_t definitionLen = strlen(definition);

    char* szWord = (char*)malloc(4 * sizeof(char));
    char* szDef = (char*)malloc(4 * sizeof(char));
    sprintf(szWord, "%04lu", wordLen);
    sprintf(szDef, "%04lu", definitionLen);
    //sprintf(insert, "%04d%01d%04lu%s%04lu%s", 0, 1, wordLen, word, definitionLen, definition);
    //printf("%04d%1d%04lu%s%04lu%s", 0, 1, wordLen, word, definitionLen, definition);

    write(socket, "0000", 4);
    write(socket, "+", 1);
    write(socket, szWord, 4);
    write(socket, word, wordLen);
    write(socket, szDef, 4);
    write(socket, definition, definitionLen);

    //printf("00001%s%s%s%s\n", szWord, word, szDef, definition);
}

int main(int argc, char *argv[]) 
{
    const char* serverIP = "0.0.0.0";
    int serverPort = 9034;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Failed to create socket");
        return 1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0) {
        perror("Invalid server address");
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Failed to connect to the server");
        return 1;
    }

    if (argc == 1) {
        FILE* file = fopen("WordWord.csv", "r");
        if (!file) {
            perror("Failed to open file");
            return 1;
        }

        char line[1024];
        int counterLines = 0;
        while (fgets(line, sizeof(line), file)) {
            char* word = strtok(line, ",");
            char* definition = strtok(NULL, "\n");
            
            if (word && definition) {
                processLine(word, definition, sock);

                if((counterLines++ % 1000) == 0){
                    close(sock);

                    sock = socket(AF_INET, SOCK_STREAM, 0);
                    if (sock == -1) {
                        perror("Failed to create socket");
                        return 1;
                    }

                    struct sockaddr_in serverAddress;
                    serverAddress.sin_family = AF_INET;
                    serverAddress.sin_port = htons(serverPort);

                    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0) {
                        perror("Invalid server address");
                        return 1;
                    }

                    if (connect(sock, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
                        perror("Failed to connect to the server");
                        return 1;
                    }
                }
                //if (write(sock, insert, strlen(insert)) == -1) {
                //    perror("Failed to send data to the server");
                //    return 1;
                //}
            }
        }

        fclose(file);
    } else {
        if (!strcmp(argv[1], "read")) {
            char sizeF[256];
            sprintf(sizeF, "%04d", strlen(argv[2]));

            write(sock, "0000", 4);
            write(sock, "&", 1);
            write(sock, sizeF, 4);
            write(sock, argv[2], strlen(argv[2]));

            int nbytes;
            char buf[32768];
            while (true) {
                if (!(nbytes = read(sock, buf, 4)) <= 0) {
                    buf[4] = '\0';

                    nbytes = read(sock, buf, 1);
                    buf[1] = '\0';

                    nbytes = read(sock, buf, 6);
                    buf[6] = '\0';
                    char *szData = buf;
                    
                    printf("SZDATA: %s\n", szData);

                    int szD = std::stoi(szData);
                    nbytes = read(sock, buf, szD);
                    buf[szD] = '\0';
                    char *data = buf;
                    printf("%s\n", data);
                }
            }
        }
    }

    close(sock);

    return 0;
}

