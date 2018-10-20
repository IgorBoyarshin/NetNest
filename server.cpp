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
    std::cout << "----- START -----" << std::endl;

    int socketHandle;
    socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle < 0) {
        std::cout << "Failed" << std::endl;
        return -1;
    }

    // Structure for handling internet addresses
    struct sockaddr_in socketInfo;
    memset(&socketInfo, 0, sizeof(sockaddr_in));
    socketInfo.sin_family = AF_INET;
    // socketInfo.sin_addr.s_addr = htonl(INADDR_ANY);
    /* inet_aton("10.18.51.1", &socketInfo.sin_addr); */
    inet_aton("127.0.0.1", &socketInfo.sin_addr);
    socketInfo.sin_port = htons(2345);

    if (bind(socketHandle, reinterpret_cast<sockaddr*>(&socketInfo), sizeof(socketInfo)) < 0) {
        close(socketHandle);
        std::cout << "Unable to bind" << std::endl;
        return -1;
    }

    listen(socketHandle, 1);

    int socketConnection;
    socketConnection = accept(socketHandle, NULL, NULL);
    if (socketConnection < 0) {
        std::cout << "Unable to accept connection" << std::endl;
        return -1;
    }

    close(socketHandle);

    int rc = 0;
    char buff[512];

    rc = recv(socketConnection, buff, 512, 0);
    buff[rc] = '\0';

    std::cout << "Read " << rc << " bytes" << std::endl;
    std::cout << "Received: " << buff << std::endl;

    std::cout << "-----  END  -----" << std::endl;
    return 0;
}
