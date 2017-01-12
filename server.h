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

#define MAX_CONNECTED 14
#define CONNECT_QUEUE 5
#define MAX_SMALL_ROOMS 2

class server {
    int serverPort;
    int sockfd;
    int connectedUsers;
    bool serverFull;
    int max_socketDesc;
    int sd;
    int clientSockets[MAX_CONNECTED];
    int activity;

    fd_set socketSet;

    struct sockaddr_in sockAddr;

    std::vector<gameRoom *> gameRooms;
public:
    server();

    void start();

    static void consoleOut(std::string msg);

    void sendMsg(int socket, std::string msg);

    std::string receiveMsg(int socket);

    bool loginUsr(int socket, std::string name);

    bool nameAvailable(std::string name);

    void logoutUsr(int socket);

    void sendAllRooms(int socket);

    void sendRoomInfo(int socket, int roomId);

    void sendTimeMsg(gameRoom *r, int id);

    void sendRoomUsers(int socket, int roomId);

    void sendRoomUserInfo(int socket, int roomId, int user);

    void setUsrReady(int playerId);

    void assignUsrToRoom(players::User player);

    bool removeUsrFromRoom(int roomId, int socket);

    bool waitForPlayer();

    void isOnTurn(int sd, std::string card);

    void checkCheat(int sd);

    players::User getUserById(int id);
};

#endif //UPS_SERVER_SERVER_H
