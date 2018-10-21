#include <iostream>

#include<cstring> // memset()

#include <unistd.h> // close socket
#include <arpa/inet.h> // inet_aton()
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>

#define PORT_SELF 2343
#define PORT_REMOTE 2344


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


int client(const std::string& userName) {
    /* std::cout << "----- START CLIENT of " << userName << "-----" << std::endl; */

    for (int i=0; i < 100; i++) {
        /* std::cout << "=============== " << i << std::endl; */
        int socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socketHandle < 0) {
            std::cout << ":> Failed to create socket" << std::endl;
            return -1;
        }

        sockaddr_in destinationSocketInfo;
        memset(&destinationSocketInfo, 0, sizeof(sockaddr_in));
        destinationSocketInfo.sin_family = AF_INET;
        destinationSocketInfo.sin_port = htons(PORT_REMOTE);
        inet_aton("192.168.1.3", &destinationSocketInfo.sin_addr);

        while (connect(socketHandle, reinterpret_cast<sockaddr*>(&destinationSocketInfo), sizeof(destinationSocketInfo)) < 0) {
            /* std::cout << "-- Could not establish connection..." << std::endl; */
            const unsigned int seconds = 1;
            sleep(seconds);
        }
        /* std::cout << "-- Connection to the server established!" << std::endl; */

        /* std::cout << "**** send 1" << std::endl; */
        char buff[512] = "Hedgehog loves Masik more!\0";
        std::cout << userName << " says: " << buff << std::endl;
        send(socketHandle, buff, strlen(buff) + 1, 0);
        sleep(4);

        close(socketHandle);
    }

    /* std::cout << "-----  END CLIENT of " << userName << " -----" << std::endl; */
    return 0;
}


int server(const std::string& userName) {
    /* std::cout << "----- START SERVER of " << userName << "-----" << std::endl; */

    signal(SIGCHLD, handleSignal); // TODO: check if needed

    int socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle < 0) {
        std::cout << ":> Failed to create socket" << std::endl;
        return -1;
    }
    {
        int option = 1; // TODO: confirm that the variable may be disposed of
        setsockopt(socketHandle, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    }

    // Structure for handling internet addresses
    sockaddr_in selfSocketInfo;
    memset(&selfSocketInfo, 0, sizeof(sockaddr_in));
    selfSocketInfo.sin_family = AF_INET;
    selfSocketInfo.sin_port = htons(PORT_SELF);
    inet_aton("192.168.1.3", &selfSocketInfo.sin_addr);

    if (bind(socketHandle, reinterpret_cast<sockaddr*>(&selfSocketInfo), sizeof(selfSocketInfo)) < 0) {
        std::cout << ":> Unable to bind: " << strerror(errno) << std::endl;
        close(socketHandle);
        return -1;
    }

    const unsigned int MAX_IN_QUEUE = 5;
    listen(socketHandle, MAX_IN_QUEUE);

    for (unsigned int counter = 0; counter < 10;) {
        if (!isReadyToRead(socketHandle)) {
            /* std::cout << "-- No incoming packets..." << std::endl; */
            const unsigned int seconds = 1;
            sleep(seconds);
            continue;
        }
        /* std::cout << "++ Packet arrived" << std::endl; */

        sockaddr_in incomingSocketInfo;
        socklen_t incomingSocketSize = sizeof(incomingSocketInfo);
        int socketConnection = accept(
                socketHandle,
                reinterpret_cast<sockaddr*>(&incomingSocketInfo),
                &incomingSocketSize);
        if (socketConnection < 0) {
            std::cout << ":> Unable to accept connection" << std::endl;
            close(socketHandle);
            if (errno == EINTR) { std::cout << "******* in \"needed??\"" << std::endl; continue; } // TODO: needed??
            return -1;
        }

        /* std::cout << "++ Reveiced packet from: " */
        /*     << inet_ntoa(incomingSocketInfo.sin_addr) << std::endl; */

        switch(fork()) {
            case -1: // fork error
                std::cout << ":> Fork errorred -1" << std::endl;
                close(socketConnection);
                close(socketHandle);
                return -1;
            case 0: // child
                close(socketHandle);
                {
                    /* std::cout << "== Processing packet..." << std::endl; */
                    char buff[512];
                    int rc = recv(socketConnection, buff, 512, 0);
                    buff[rc] = '\0';

                    /* std::cout << "== Read " << rc << " bytes" << std::endl; */
                    /* std::cout << "== Received: " << buff << std::endl; */
                    std::cout << userName << " hears: " << buff << std::endl;
                }
                exit(0);
            default: // parent
                /* std::cout << "++ Packet being processed in parallel" << std::endl; */
                close(socketConnection);
                counter++;
        }
    }

    // Wait for all children that process packets to finish
    while (waitpid(-1, NULL, 0)) if (errno == ECHILD) break;

    close(socketHandle);
    /* std::cout << "-----  END SERVER of " << userName << " -----" << std::endl; */
    return 0;
}


int main() {
    const std::string username("Hedgehog");
    std::cout << "----- " << username << " START -----" << std::endl;

    switch(fork()) {
        case 0:
            return client(username);
        default:
            server(username);
    }

    std::cout << "-----  " << username << " END  -----" << std::endl;
    return 0;
}
