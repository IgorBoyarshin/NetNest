#include <iostream>

/* #include<stdio.h> //printf */
#include<cstring> // memset()
/* #include<stdlib.h> //exit(0); */

#include <unistd.h> // close socket
#include <arpa/inet.h> // inet_aton()
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>


void handleSignal(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

bool isReadyToRead(int socketHandle) {
    fd_set checkReadSet;
    timeval timeout = {0,0}; // results in immediate return if not ready (for polling)

    FD_ZERO(&checkReadSet);
    FD_SET(socketHandle, &checkReadSet);

    if (const int readyDescriptorsAmount = select(socketHandle + 1, &checkReadSet,
                static_cast<fd_set*>(0), static_cast<fd_set*>(0), &timeout);
            readyDescriptorsAmount == 0) {
        return false; // no fitting descriptors => nothing ready yet
    } else if (readyDescriptorsAmount == -1) { // error
        std::cout << ":> isReadyToRead::select() returned -1:" << strerror(errno) << std::endl;
        close(socketHandle); // do this??
        return false;
    } else {
        return FD_ISSET(socketHandle, &checkReadSet);
    }

    return false;
}

int main() {
    std::cout << "----- START -----" << std::endl;

    signal(SIGCHLD, handleSignal);

    int socketHandle;
    socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle < 0) {
        std::cout << "Failed to create socket" << std::endl;
        return -1;
    }
    int option = 1;
    setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // Structure for handling internet addresses
    struct sockaddr_in socketInfo;
    memset(&socketInfo, 0, sizeof(sockaddr_in));
    socketInfo.sin_family = AF_INET;
    // socketInfo.sin_addr.s_addr = htonl(INADDR_ANY);
    /* inet_aton("10.18.51.1", &socketInfo.sin_addr); */
    /* inet_aton("127.0.0.1", &socketInfo.sin_addr); */
    inet_aton("192.168.1.3", &socketInfo.sin_addr);
    socketInfo.sin_port = htons(2343);

    if (bind(socketHandle, reinterpret_cast<sockaddr*>(&socketInfo), sizeof(socketInfo)) < 0) {
        std::cout << strerror(errno) << std::endl;
        close(socketHandle);
        std::cout << "Unable to bind" << std::endl;
        return -1;
    }

    const unsigned int MAX_IN_QUEUE = 5;
    listen(socketHandle, MAX_IN_QUEUE);

    int socketConnection;
    unsigned int counter = 0;
    for(;;) {
        if (counter >= 10) break;

        if (!isReadyToRead(socketHandle)) {
            std::cout << "Parent waitinig..." << std::endl;
            sleep(1);
            continue;
        }
        std::cout << "Gotcha!!" << std::endl;
        struct sockaddr_in otherSocketInfo;
        socklen_t actualSize = sizeof(otherSocketInfo);
        socketConnection = accept(
                socketHandle,
                reinterpret_cast<sockaddr*>(&otherSocketInfo),
                &actualSize);
                // reinterpret_cast<socklen_t*>(&otherSocketInfo));
        if (socketConnection < 0) {
            std::cout << "Unable to accept connection" << std::endl;
            close(socketHandle);
            if(errno == EINTR) continue; // needed??
            return -1;
        }

        std::cout << "Reveiced from: " << inet_ntoa(otherSocketInfo.sin_addr) << std::endl;

        switch(fork()) {
            case -1:
                close(socketHandle);
                close(socketConnection);
                std::cout << "Forked -1" << std::endl;
                return -1;
            case 0: // child
                close(socketHandle);
                {
                    std::cout << "Child working" << std::endl;
                    char buff[512];
                    int rc = recv(socketConnection, buff, 512, 0);
                    buff[rc] = '\0';

                    std::cout << "Read " << rc << " bytes" << std::endl;
                    std::cout << "Received: " << buff << std::endl;
                }
                exit(0);
            default: // parent
                std::cout << "Parent got connection" << std::endl;
                close(socketConnection);
                counter++;
        }
    }

    while (waitpid(-1, NULL, 0)) {
       if (errno == ECHILD) {
          break;
       }
    }

    /* int socketConnection = accept(socketHandle, NULL, NULL); */
    /* if (socketConnection < 0) { */
    /*     std::cout << "Unable to accept connection" << std::endl; */
    /*     close(socketHandle); */
    /*     return -1; */
    /* } */

    close(socketHandle);

    /* int rc = 0; */
    /* char buff[512]; */
    /*  */
    /* rc = recv(socketConnection, buff, 512, 0); */
    /* buff[rc] = '\0'; */
    /*  */
    /* #<{(| std::cout << "Read " << rc << " bytes" << std::endl; |)}># */
    /* #<{(| std::cout << "Received: " << buff << std::endl; |)}># */
    /* std::cout << buff << std::endl; */
    /* close(socketConnection); */

    std::cout << "-----  END  -----" << std::endl;
    return 0;
}
