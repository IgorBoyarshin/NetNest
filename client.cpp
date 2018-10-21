#include <iostream>

/* #include<stdio.h> //printf */
#include<cstring> // memset()
/* #include<stdlib.h> //exit(0); */

#include <unistd.h> // close socket
#include <arpa/inet.h> // inet_aton()
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


int main() {
    std::cout << "----- START CLIENT -----" << std::endl;

    int socketHandle;
    socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle < 0) {
        std::cout << "Failed" << std::endl;
        return -1;
    }

    struct sockaddr_in socketInfo;
    memset(&socketInfo, 0, sizeof(sockaddr_in));
    socketInfo.sin_family = AF_INET;
    /* inet_aton("127.0.0.1", &socketInfo.sin_addr); */
    inet_aton("192.168.1.3", &socketInfo.sin_addr);
    socketInfo.sin_port = htons(2343);

    /* struct hostent* hPtr; */
    if (connect(socketHandle, reinterpret_cast<sockaddr*>(&socketInfo), sizeof(socketInfo)) < 0) {
        close(socketHandle);
        std::cout << "Unable to connect" << std::endl;
        return -1;
    }

    char buff[512] = "hello from Igorek\0";

    send(socketHandle, buff, strlen(buff) + 1, 0);

    std::cout << "-----  END CLIENT  -----" << std::endl;
    return 0;
}
