//
// Created by Lukado on 20. 10. 2016.
//

#include "server.h"

const int SERVER_PORT = 2222;

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
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
                                messenger::sendMsg(sd, badNick += +'\n');
                                break;
                            }
                        case msgtable::C_USR_READY:
                            setUsrReady(sd);
                            break;
                        case msgtable::C_PUT_CARD:
                            if (splittedMsg[1].length()==1) isOnTurn(sd, splittedMsg[1]);
                            break;
                        case msgtable::C_CHECK_CHEAT:
                            checkCheat(sd);
                            break;
                        case msgtable::EOS:
                            logoutUsr(sd);
                            clientSockets[i] = 0;
                            break;
                        case msgtable::ERR:
                            logoutUsr(sd);
                            clientSockets[i] = 0;
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

std::string server::receiveMsg(int socket) {
    char msg[128];
    memset(msg, '\0', 128);
    int ret = (int) read(socket, &msg, 127);
    if (ret < 0) {
        consoleOut("Chyba při příjmání zprávy od uživatele " + socket);
        return "ERR";
    } else if (ret == 0) {
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
            player.isOnline = true;

            connectedUsers++;
            if (connectedUsers >= MAX_CONNECTED) {
                serverFull = true;
            }

            FD_SET(socket, &socketSet);
            messenger::sendMsg(socket, ("S_LOGGED:" + name + "#" += '\n'));
            assignUsrToRoom(player);
            consoleOut("Přihlášen nový hráč " + name + " s id " + std::to_string(socket));
            return true;
        } else if(userIsDced(name)){
            for (int i = 0; i < gameRooms.size(); ++i){
                for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
                    if(gameRooms.at(i)->users.at(j).name==name){
                        gameRooms.at(i)->reconnect(socket, j);
                    }
                }
            }
        } else {
            messenger::sendMsg(socket, "S_NAME_EXISTS:" + name + "#" += '\n');
            FD_CLR(socket, &socketSet);
            close(socket);
            return false;
        }
    } else {
        messenger::sendMsg(socket, "S_SERVER_FULL#" + '\n');
        FD_CLR(socket, &socketSet);
        close(socket);
        return false;
    }
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
    messenger::sendMsg(socket, msg);
}

void server::assignUsrToRoom(players::User player) {
    int newRoomId = -1;
    for (int i = 0; i<gameRooms.size(); i++){
        if(!gameRooms.at(i)->isFull) {
            newRoomId = gameRooms.at(i)->addPlayer(player);
            if(newRoomId!=-1) break;
        }
    }

    if (newRoomId > -1) {
        consoleOut("[Místnost " + std::to_string(newRoomId) + "] Hráč s id " + std::to_string(player.uId) + " vstoupil do místnosti");
        std::string msg = "S_ROOM_INFO:"+std::to_string(gameRooms.at(newRoomId)->users.size())+":";
        for (int j = 0; j < gameRooms.at(newRoomId)->users.size(); ++j) {
            msg += std::to_string(gameRooms.at(newRoomId)->users.at(j).isReady)+":";
        }
        messenger::sendMsg(player.uId, msg+"#\n");

    } else {
        messenger::sendMsg(player.uId, "S_JOIN_ERR:" + std::to_string(newRoomId) + "#" += '\n');
    }
}

void server::setUsrReady(int playerId) {
    for (int i = 0; i < gameRooms.size(); ++i) {
        gameRooms.at(i)->setPlayerReady(playerId, true);
    }
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

void server::logoutUsr(int socket) {
    players::User player = getUserById(sd);
    if (player.uId != -1 && gameRooms.at(player.roomId)->roomStatus == gameRoom::ROOM_WAIT) {
        connectedUsers--;
        gameRooms.at(player.roomId)->removePlayer(player.uId);
        serverFull = false;
        FD_CLR(socket, &socketSet);
        close(socket);
        consoleOut("Hráč s id " + std::to_string(socket) + " se odpojil\n");
    } else if (player.uId != -1) {
        connectedUsers--;
        gameRooms.at(player.roomId)->setPlayerDc(player.uId);
        consoleOut("[Místnost " + std::to_string(player.roomId) + "] Čeká se na reconnect hráče s id " + std::to_string(socket) + "\n");
    } else{
        serverFull = false;
        FD_CLR(socket, &socketSet);
        close(socket);
        consoleOut("Hráč s id " + std::to_string(socket) + " se odpojil\n");
    }
}

void server::isOnTurn(int sd, std::string card) {
    players::User player = getUserById(sd);
    if(player.uId != -1) {
        gameRooms.at(player.roomId)->placeCard(sd, card);
    }
}

void server::checkCheat(int sd) {
    players::User player = getUserById(sd);
    if(player.uId != -1) {
        gameRooms.at(player.roomId)->checkTopCard(sd);
    }
}

players::User server::getUserById(int id) {
    for (int i = 0; i < gameRooms.size(); i++) {
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if (gameRooms.at(i)->users.at(j).uId == id) return gameRooms.at(i)->users.at(j);
        }
    }
    players::User user;
    user.uId = -1;
    return user;
}

bool server::userIsDced(std::string name) {
    for (int i = 0; i < gameRooms.size(); ++i){
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if(gameRooms.at(i)->users.at(j).name==name && !gameRooms.at(i)->users.at(j).isOnline){
                return true;
            }
        }
    }
    return false;
}
