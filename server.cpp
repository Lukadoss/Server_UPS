//
// Created by Seda on 20. 10. 2016.
//

#include "server.h"

const int SERVER_PORT = 44444;

server::server() {
    serverPort = SERVER_PORT;
}

void server::start() {
    connectedUsers = 0;
    serverFull = false;
    sockfd = -1;

    for (int i = 0; i < MAX_CONNECTED; i++) {
        clientSockets[i] = 0;
    }
    //vytvoření socketu
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sockfd < 0) {
        consoleOut("Chyba při vytvoření socketu");
        exit(1);
    }


    int optionVal = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optionVal, sizeof(optionVal));

    //struktura sockAddr
    memset(&sockAddr, '\0', sizeof(sockAddr));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = INADDR_ANY; //inet_addr("127.0.0.1"); nebo INADDR_ANY;
    sockAddr.sin_port = htons((uint16_t) serverPort);

    std::string serverAddress = inet_ntoa(sockAddr.sin_addr);
    std::string serverPort = std::to_string(ntohs(sockAddr.sin_port));
    consoleOut("Server address: " + serverAddress);
    consoleOut("Server port: " + serverPort);

    //Bind socketu
    if (bind(sockfd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        consoleOut("Bindování socketu se nezdařilo");
        exit(1);
    }

    //listen
    if (listen(sockfd, MAX_CONNECTED + CONNECT_QUEUE) < 0) {
        consoleOut("Chyba při naslouchání");
        exit(1);
    }

    consoleOut("Inicializace herních místností");
    gameRooms = std::vector<gameRoom *>(MAX_SMALL_ROOMS);

    for (int j = 0; j < MAX_SMALL_ROOMS; ++j) {
        gameRooms.at(j) = new gameRoom();
    }

    for (int i = 0; i < gameRooms.size(); ++i) {
        gameRooms.at(i)->numPlaying = 0;
        gameRooms.at(i)->maxPlaying = 7;
        gameRooms.at(i)->roomName = "Game room " + std::to_string(i);
        gameRooms.at(i)->isFull = false;
        gameRooms.at(i)->users = std::vector<players::User>();
        gameRooms.at(i)->roomId = i;
    }

    sockaddr_in clientSocketAddr;
    int clientSocketAddrSize = sizeof(clientSocketAddr);
    int clientSocket;

    consoleOut("Server spuštěn, čeká na příchozí připojení");
    while (true) {
        FD_ZERO(&socketSet);

        FD_SET(sockfd, &socketSet); //přidání server socketu do setu (Selector)
        max_socketDesc = sockfd;

        for (int i = 0; i < (MAX_CONNECTED); i++) {
            sd = clientSockets[i];
            if (sd > 0) {
                FD_SET(sd, &socketSet);
            }

            if (sd > max_socketDesc) {
                max_socketDesc = sd;
            }
        }

        activity = select(max_socketDesc + 1, &socketSet, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &socketSet)) {
            if ((clientSocket = accept(sockfd, (struct sockaddr *) &clientSocketAddr,
                                       (socklen_t *) &clientSocketAddrSize)) < 0) {
                consoleOut("Chyba při acceptu");
                close(sockfd);
                exit(1);
            }
            for (int i = 0; i < MAX_CONNECTED; i++) {
                if (clientSockets[i] == 0) {
                    clientSockets[i] = clientSocket;
                    //cout << "Přidávám nový socket " << clientSocket << " do setu" << endl;
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CONNECTED; i++) {
            sd = clientSockets[i];
            if (FD_ISSET(sd, &socketSet)) {
                std::string incMsg = receiveMsg(sd);
                if (incMsg.size() > 0) {
                    std::vector<std::string> splittedMsg = stl::splitMsg(incMsg);
                    switch (msgtable::getType(splittedMsg[0])) {
                        case msgtable::C_LOGIN:
                            if (splittedMsg[1].length() >= 3 && splittedMsg[1].length() <= 15) {
                                if (!loginUsr(sd, splittedMsg[1])) {
                                    clientSockets[i] = 0;
                                }
                                break;
                            } else {
                                std::string badNick = "S_NICK_LEN#";
                                sendMsg(sd, badNick += +'\n');
                                break;
                            }
                        case msgtable::C_LOGOUT:
                            if(waitForPlayer()) clientSockets[i] = 0;
                            break;
                        case msgtable::C_GET_TABLE:
                            sendAllRooms(sd);
                            break;
                        case msgtable::C_USR_READY:
                            setUsrReady(sd);
                            break;
                        case msgtable::C_ROOM_USERS:
                            sendRoomUsers(sd, stoi(splittedMsg[1]));
                            break;
                        case msgtable::C_PUT_CARD:
                            gameRooms.at(stoi(splittedMsg[1]))->addTurned();
                            break;
                        case msgtable::C_CHECK_CHEAT:
                            gameRooms.at(stoi(splittedMsg[1]))->turnCard(sd, stoi(splittedMsg[2]), stoi(splittedMsg[3]));
                            break;
                        case msgtable::EOS:
                            if(waitForPlayer()) clientSockets[i] = 0;
                            break;
                        case msgtable::ERR:
                            if(waitForPlayer()) clientSockets[i] = 0;
                            break;
                        case msgtable::NO_CODE:
                            break;
                        default:
                            break;
                    }
                }
            }
        }

    }
}

void server::consoleOut(std::string msg) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "[%d-%m-%Y %H:%M:%S] ", timeinfo);
    std::string str(buffer);

    std::cout << str << msg << std::endl;
}

void server::sendMsg(int socket, std::string msg) {
    const char *msgChar = msg.c_str();
    send(socket, (void *) msgChar, msg.length(), 0);
}

std::string server::receiveMsg(int socket) {
    char msg[128];
    memset(msg, '\0', 128);
    int ret = (int) read(socket, &msg, 127);
    if (ret < 0) {
        consoleOut("Chyba při příjmání zprávy od uživatele " + socket);
        logoutUsr(socket);
        return "ERR";
    } else if (ret == 0) {
        logoutUsr(socket);
        return "EOS";
    } else {
        int i = 0;
        std::string msgRet = "";
        while (msg[i] != '#' && i < 127 && msg[i] != '\0' && msg[i] != '\n') {
            msgRet += msg[i];
            i++;
        }
        return msgRet;
    }
}

bool server::loginUsr(int socket, std::string name) {
    for (int i = 0; i < gameRooms.size(); ++i) {
        if(gameRooms.at(i)->playerAlreadyJoined(socket)) return true;
    }
    if (!serverFull) {
        if (nameAvailable(name)) {
            players::User player;
            player.uId = socket;
            player.name = name;
            player.roomId = -1;
            player.isReady = false;

            connectedUsers++;
            if (connectedUsers >= MAX_CONNECTED) {
                serverFull = true;
            }

            FD_SET(socket, &socketSet);
            sendMsg(socket, ("S_LOGGED:" + name + "#" += '\n'));
            consoleOut("Přihlášen nový hráč " + name + " s id " + std::to_string(socket));

            for (int i = 0; i<gameRooms.size(); i++){
                std::cout<<gameRooms.at(i)->isFull<<std::endl;
                if(!gameRooms.at(i)->isFull) {
                    assignUsrToRoom(gameRooms.at(i)->roomId, player);
                    break;
                }
            }
            return true;
        } else {
            sendMsg(socket, "S_NAME_EXISTS:" + name + "#" += '\n');
            FD_CLR(socket, &socketSet);
            close(socket);
            return false;
        }
    } else {
        sendMsg(socket, "S_SERVER_FULL#" + '\n');
        FD_CLR(socket, &socketSet);
        close(socket);
        return false;
    }
}

void server::sendAllRooms(int socket) {
    for (int i = 0; i < gameRooms.size(); i++) {
        sendRoomInfo(socket, i);
        std::string incMsg = receiveMsg(socket);
        std::vector<std::string> splittedMsg = stl::splitMsg(incMsg);
        if (splittedMsg[0] == "C_ROW_UPDATE") continue;
        else break;
    }
}

void server::sendRoomInfo(int socket, int roomId) {
    std::string msg = "S_ROOM_INFO:" + std::to_string(gameRooms.at(roomId)->roomId) + ":" +
                              gameRooms.at(roomId)->roomName + ":" +
                              std::to_string(gameRooms.at(roomId)->numPlaying) + ":" +
                              std::to_string(gameRooms.at(roomId)->maxPlaying) + ":" +
                              gameRoom::getString(gameRooms.at(roomId)->roomStatus) +
                              "#" += '\n';
    sendMsg(socket, msg);
}

void server::sendRoomUsers(int socket, int roomId) {
    for (int i = 0; i < gameRooms.at(roomId)->numPlaying; i++) {
        sendRoomUserInfo(socket, roomId, i);
        std::string incMsg = receiveMsg(socket);
        std::vector<std::string> splittedMsg = stl::splitMsg(incMsg);
        if (splittedMsg[0] == "C_USER_UPDATE") continue;
        else break;
    }
}

void server::sendRoomUserInfo(int socket, int roomId, int user) {
    std::string ready = "0";
    if (gameRooms.at(roomId)->users.at(user).isReady) {
        ready = "1";
    }
    std::string msg =
            "S_ROOM_USER_INFO:" + std::to_string(user) + ":" + gameRooms.at(roomId)->users.at(user).name + ":" +
            ready + "#" += '\n';
    sendMsg(socket, msg);
}

void server::assignUsrToRoom(int roomId, players::User player) {
    int newRoomId = gameRooms.at(roomId)->addPlayer(player);

    if (newRoomId > -1) {
        consoleOut("[Místnost " + std::to_string(roomId) + "] Hráč s id " + std::to_string(player.uId) + " vstoupil do místnosti");
        sendMsg(player.uId, "S_USR_JOINED:" + std::to_string(roomId) + ":" +
                std::to_string(gameRooms.at(roomId)->numPlaying) + ":" +
                std::to_string(gameRooms.at(roomId)->maxPlaying) + ":" +
                gameRoom::getString(gameRooms.at(roomId)->roomStatus) +
                "#" += '\n');

    } else {
        sendMsg(player.uId, "S_JOIN_ERR:" + std::to_string(roomId) + "#" += '\n');
    }
}

void server::setUsrReady(int playerId) {
    for (int i = 0; i < gameRooms.size(); ++i) {
        gameRooms.at(i)->setPlayerReady(playerId, true);
    }
}

void server::removeUsrFromRoom(int roomId, int socket) {
    if(gameRooms.at(roomId)->removePlayer(socket)){
        consoleOut("[Místnost " + std::to_string(roomId) + "] Hráč s id " + std::to_string(socket) + " byl vyhozen z místnosti");
    };
}

bool server::nameAvailable(std::string name) {
    for (int i = 0; i < gameRooms.size(); i++) {
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if (!name.compare(gameRooms.at(i)->users.at(j).name))
                return false;
        }
    }
    return true;
}

void server::sendTimeMsg(gameRoom *r, int id) {
    for (int i = 0; i < r->numPlaying; i++) {
        sendMsg(r->users.at(i).uId, "S_TIME_NOTIFY:" + std::to_string(id) + "#" += '\n');
    }
}

void server::logoutUsr(int socket) {
    for (int i = 0; i < gameRooms.size(); i++) {
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if (gameRooms.at(i)->users.at(j).uId == socket) {
                removeUsrFromRoom(gameRooms.at(i)->roomId, socket);
                connectedUsers--;
                serverFull = false;
                FD_CLR(socket, &socketSet);
                close(socket);
                consoleOut("Hráč s id " + std::to_string(socket) + " se odpojil");
                break;
            }
        }
    }
}

bool server::waitForPlayer() {
    logoutUsr(sd);
    return true;
}
