//
// Created by Lukado on 20. 10. 2016.
//
#ifndef UPS_SERVER_SERVER_H
#define UPS_SERVER_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "gameRoom.h"
#include "server.h"
#include "msgTable.h"

#define QUEUE 5

class server {
    int serverPort;
    int sockfd;
    int connectedUsers;
    bool serverFull;
    int max_socketDesc;
    int sd;
    int *clientSockets;
    int activity;

    fd_set socketSet;

    struct sockaddr_in sockAddr;

    std::vector<gameRoom *> gameRooms;
public:
    server();

    server(std::string ipAddr, int port, int maxrooms, int maxconn);

    void start();

    static void consoleOut(std::string msg);

    std::string receiveMsg(int socket);

    bool loginUsr(int socket, std::string name);

    bool nameAvailable(std::string name);

    void logoutUsr(int socket, int i);

    void sendRoomInfo(int socket);

    void setUsrReady(int playerId);

    void assignUsrToRoom(players::User player);

    void isOnTurn(int sd, std::string card);

    void checkCheat(int sd);

    players::User getUserById(int id);

    bool userIsDced(std::string name);

    bool checkPlayer(int sd);
};

#endif //UPS_SERVER_SERVER_H
