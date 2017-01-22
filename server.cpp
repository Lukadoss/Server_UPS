#include "server.h"

int MAX_CONNECTED;
int MAX_ROOMS;
int MAX_PLAYING;
std::string IP;

server::server() {
    IP = "INADDR_ANY";
    MAX_ROOMS = 2;
    MAX_PLAYING = 7;
    MAX_CONNECTED = MAX_ROOMS * MAX_PLAYING;
    serverPort = 2222;
    clientSockets = new int[MAX_CONNECTED];
}

server::server(std::string ipAddr, int port, int maxrooms, int maxplaying) {
    IP = ipAddr;
    MAX_ROOMS = maxrooms;
    MAX_PLAYING = maxplaying;
    MAX_CONNECTED = MAX_ROOMS * MAX_PLAYING;
    serverPort = port;
    clientSockets = new int[MAX_CONNECTED];
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
    if (IP.compare("INADDR_ANY") == 0) {
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (IP.compare("localhost") == 0) {
        sockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else {
        in_addr_t adr = inet_addr(IP.c_str());
        if(adr==-1) {
            std::cout<<"IP adresa není validní"<<std::endl;
            exit(1);
        }
        sockAddr.sin_addr.s_addr = inet_addr(IP.c_str());
    }
    sockAddr.sin_port = htons((uint16_t) serverPort);

    std::string serverAddress = inet_ntoa(sockAddr.sin_addr);
    std::string serverPort = std::to_string(ntohs(sockAddr.sin_port));
    consoleOut("Server address: " + serverAddress);
    consoleOut("Server port: " + serverPort);
    //Bind socketu
    if (bind(sockfd, (struct sockaddr *) &sockAddr, sizeof(sockAddr)) < 0) {
        consoleOut("Bind socketu se nezdařil");
        exit(1);
    }

    //listen
    if (listen(sockfd, MAX_CONNECTED + QUEUE) < 0) {
        consoleOut("Chyba při naslouchání");
        exit(1);
    }

    consoleOut("Počet herních místností: " + std::to_string(MAX_ROOMS));
    consoleOut("Maximální počet hráčů v místnosti: " + std::to_string(MAX_PLAYING));
    gameRooms = std::vector<gameRoom *>(MAX_ROOMS);

    for (int j = 0; j < MAX_ROOMS; ++j) {
        gameRooms.at(j) = new gameRoom();
    }

    for (int i = 0; i < gameRooms.size(); ++i) {
        gameRooms.at(i)->numPlaying = 0;
        gameRooms.at(i)->maxPlaying = MAX_PLAYING;
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
                    std::vector<std::string> splittedMsg = messenger::splitMsg(incMsg);
                    switch (msgtable::getType(splittedMsg[0])) {
                        case msgtable::C_LOGIN:
                            if (checkPlayer(sd)) break;
                            if (splittedMsg[1].length() >= 3 && splittedMsg[1].length() <= 15) {
                                if (!loginUsr(sd, splittedMsg[1])) {
                                    clientSockets[i] = 0;
                                }
                                break;
                            } else {
                                if (getUserById(sd).roomId != -1) messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
                                else messenger::sendMsg(sd, "S_NICK_LEN#\n");
                                break;
                            }
                        case msgtable::C_ROOM_INFO:
                            sendRoomInfo(sd);
                            break;
                        case msgtable::C_USR_READY:
                            setUsrReady(sd);
                            break;
                        case msgtable::C_PUT_CARD:
                            if (splittedMsg[1].length() == 1) isOnTurn(sd, splittedMsg[1]);
                            else messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
                            break;
                        case msgtable::C_CHECK_CHEAT:
                            checkCheat(sd);
                            break;
                        case msgtable::EOS:
                            logoutUsr(sd, i);
                            break;
                        case msgtable::ERR:
                            logoutUsr(sd, i);
                            break;
                        case msgtable::NO_CODE:
                            messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
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
    if (!serverFull) {
        if (nameAvailable(name)) {
            players::User player;
            player.uId = socket;
            player.name = name;
            player.roomId = -1;
            player.isReady = false;
            player.isOnline = false;

            if(!assignUsrToRoom(player)) return true;

            FD_SET(socket, &socketSet);
            connectedUsers = 0;
            for (int i = 0; i < gameRooms.size(); i++) connectedUsers += gameRooms.at(i)->users.size();
            if (connectedUsers >= MAX_CONNECTED) {
                serverFull = true;
            }
            return true;
        } else if (userIsDced(name)) {
            for (int i = 0; i < gameRooms.size(); ++i) {
                for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
                    if (gameRooms.at(i)->users.at(j).name == name) {
                        gameRooms.at(i)->reconnect(socket, j);
                        consoleOut("Uživatel " + name + " se znovu připojil do hry\n");
                        return true;
                    }
                }
            }
        } else {
            messenger::sendMsg(socket, "S_NAME_EXISTS:" + name + "#\n");
            return true;
        }
    } else {
        messenger::sendMsg(socket, "S_SERVER_FULL#\n");
        FD_CLR(socket, &socketSet);
        close(socket);
        return false;
    }
}

bool server::assignUsrToRoom(players::User player) {
    int newRoomId = -1;
    for (int i = 0; i < gameRooms.size(); i++) {
        if (!gameRooms.at(i)->isFull) {
            newRoomId = gameRooms.at(i)->addPlayer(player);
            if (newRoomId != -1) break;
        }
    }

    if (newRoomId > -1) {
        consoleOut("[Místnost " + std::to_string(newRoomId) + "] Hráč s id " + std::to_string(player.uId) +
                   " vstoupil do místnosti");
        return true;
    } else {
        messenger::sendMsg(player.uId, "S_JOIN_ERR:#\n");
        return false;
    }
}

void server::setUsrReady(int playerId) {
    if (getUserById(playerId).uId != -1) {
        for (int i = 0; i < gameRooms.size(); ++i) {
            gameRooms.at(i)->setPlayerReady(playerId, true);
        }
    } else {
        messenger::sendMsg(playerId, "S_MSG_NOT_VALID#\n");
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

void server::logoutUsr(int socket, int x) {
    players::User player = getUserById(sd);
    if (player.uId != -1 && gameRooms.at(player.roomId)->roomStatus == gameRoom::ROOM_WAIT) {
        gameRooms.at(player.roomId)->removePlayer(player.uId);
        consoleOut("Hráč s id " + std::to_string(socket) + " se odpojil\n");
    } else if (player.uId != -1) {
        gameRooms.at(player.roomId)->setPlayerDc(player.uId);
        consoleOut("[Místnost " + std::to_string(player.roomId) + "] Čeká se na reconnect hráče s id " +
                   std::to_string(socket) + "\n");
    } else {
        consoleOut("Hráč s id " + std::to_string(socket) + " se odpojil\n");
    }
    clientSockets[x] = 0;
    FD_CLR(socket, &socketSet);
    close(socket);

    connectedUsers = 0;
    for (int i = 0; i < gameRooms.size(); i++) connectedUsers += gameRooms.at(i)->users.size();
    if (connectedUsers < MAX_CONNECTED) {
        serverFull = false;
    }

}

void server::isOnTurn(int sd, std::string card) {
    players::User player = getUserById(sd);
    if (player.uId != -1) {
        gameRooms.at(player.roomId)->placeCard(sd, card);
    } else {
        messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
    }
}

void server::checkCheat(int sd) {
    players::User player = getUserById(sd);
    if (player.uId != -1) {
        gameRooms.at(player.roomId)->checkTopCard(sd);
    } else {
        messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
    }
}

players::User server::getUserById(int id) {
    for (int i = 0; i < gameRooms.size(); i++) {
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if (gameRooms.at(i)->users.at(j).uId == id && gameRooms.at(i)->users.at(j).isOnline) return gameRooms.at(i)->users.at(j);
        }//TODO: pokud vypadne nějaká chyba, je tady. Upravil sem to pred 5min
    }
    players::User user;
    user.uId = -1;
    user.roomId = -1;
    return user;
}

bool server::userIsDced(std::string name) {
    for (int i = 0; i < gameRooms.size(); ++i) {
        for (int j = 0; j < gameRooms.at(i)->users.size(); ++j) {
            if (gameRooms.at(i)->users.at(j).name == name && !gameRooms.at(i)->users.at(j).isOnline) {
                return true;
            }
        }
    }
    return false;
}

void server::sendRoomInfo(int socket) {
    players::User player = getUserById(socket);
    if (player.uId != -1 && player.isOnline) {
        if (gameRooms.at(player.roomId)->users.size() > 1) {
            for (int j = 0; j < gameRooms.at(player.roomId)->users.size(); ++j) {
                if (player.name.compare(gameRooms.at(player.roomId)->users.at(j).name)!=0) {
                    messenger::sendMsg(player.uId, "S_ROOM_INFO:" + gameRooms.at(player.roomId)->users.at(j).name + ":" +
                                                   std::to_string(gameRooms.at(player.roomId)->users.at(j).isReady) +
                                                   ":" + std::to_string(
                            gameRooms.at(player.roomId)->users.at(j).cards.size()) + "#\n");
                } else {
                    gameRooms.at(player.roomId)->sendReconnectInfo(socket, j);
                }
            }
            if (gameRooms.at(player.roomId)->users.at(0).cards.size() > 0) {
                messenger::sendMsg(player.uId, "S_PLAYER_RECONNECTED#\n");
            }
        } else {
            messenger::sendMsg(sd, "S_CONSOLE_INFO:Jsi zde sám#\n");
        }
    } else {
        messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
    }
}

bool server::checkPlayer(int sd) {
    for (int i = 0; i < gameRooms.size(); ++i) {
        if (gameRooms.at(i)->playerAlreadyJoined(sd)) {
            messenger::sendMsg(sd, "S_MSG_NOT_VALID#\n");
            return true;
        }
    }
    return false;
}
