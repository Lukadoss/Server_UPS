//
// Created by Lukado on 27/10/16.
//

#include "gameRoom.h"

gameRoom::gameRoom() {
    roomStatus = RoomStatus::ROOM_WAIT;
    info.isOver = false;
}

int gameRoom::addPlayer(players::User player) {
    if (!playerInOtherRoom(player)) {
        if (!playerAlreadyJoined(player.uId)) {
            if (isRoomWaiting()) {
                player.roomId = roomId;
                player.isOnline = true;
                users.push_back(player);

                numPlaying++;
                if (numPlaying == maxPlaying) {
                    isFull = true;
                }
                messenger::sendMsg(player.uId, ("S_LOGGED:" + player.name + "#" += '\n'));
                messenger::sendMsgAllOthers(player.uId, users,
                                            "S_USR_JOINED:" + std::to_string(users.size() - 1) + ":" + player.name +
                                            "#\n");

                return roomId;
            } else return -1;
        } else return -1;
    } else return -1;
}

bool gameRoom::removePlayer(int uId) {
    for (unsigned int i = 0; i < users.size(); i++) {
        if (users.at(i).uId == uId) {
            info.isOver = true;
            std::string name = users.at(i).name;
            users.erase(users.begin() + i);
            numPlaying--;
            isFull = false;
            if (roomStatus == RoomStatus::ROOM_WAIT) messenger::sendMsgAll(users, "S_USR_LEFT:" + name + "#\n");
            return true;
        }
    }
    return false;
}

bool gameRoom::isRoomWaiting() {
    return roomStatus == RoomStatus::ROOM_WAIT;
}

bool gameRoom::playerInOtherRoom(players::User player) {
    return player.roomId > -1;
}

bool gameRoom::playerAlreadyJoined(int id) {
    for (int i = 0; i < numPlaying; i++) {
        if (id == users.at(i).uId && users.at(i).isOnline) return true;
    }
    return false;
}

void gameRoom::setPlayerReady(int playerId, bool ready) {
    if (roomStatus == RoomStatus::ROOM_WAIT) {
        for (int i = 0; i < numPlaying; i++) {
            if (users.at(i).uId == playerId) {
                if (users.at(i).isReady == ready) {
                    messenger::sendMsg(playerId, "S_CONSOLE_INFO:Uživatel již připraven#\n");
                    break;
                } else {
                    users.at(i).isReady = ready;
                    messenger::sendMsgAllOthers(playerId, users, "S_USR_READY:" + users.at(i).name + "#\n");
                    messenger::sendMsg(playerId, "S_USR_READY_ACK#\n");
                    break;
                }
            }
        }
        allPlayersReady();
    } else {
        messenger::sendMsg(playerId, "S_MSG_NOT_VALID#\n");
    }
}

void gameRoom::setPlayerDc(int playerId) {
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).uId == playerId) users.at(i).isOnline = false;
    }
}

void gameRoom::allPlayersReady() {
    int numReady = 0;
    if (numPlaying >= 2) {
        for (int i = 0; i < numPlaying; i++) {
            if (users.at(i).isReady) numReady++;
        }
        if (numReady == numPlaying) {
            consoleOut("Všichni hráči připraveni, hra v místnosti[" + std::to_string(roomId) + "] brzy začne");
            createNewGame();
            roomStatus = RoomStatus::GAME_IN_PROGRESS;
        }
    }
}

void gameRoom::createNewGame() {
    init();
    info.onTurnId = 0;
    info.lastTurnId = 0;
    info.isOver = false;
    gameThread = std::thread(loop, this);
    gameThread.detach();
}

void gameRoom::init() {
    std::string card = "";
    info.cards = std::vector<std::string>();

    for (int i = 0; i < 4; i++) {
        switch (i) {
            case 0:
                card = "R";
                break;
            case 1:
                card = "B";
                break;
            case 2:
                card = "G";
                break;
            case 3:
                card = "K";
                break;
            default:
                break;
        }
        for (int j = 0; j < 7; ++j) {
            info.cards.push_back(card);
        }
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    shuffle(info.cards.begin(), info.cards.end(), std::default_random_engine(seed));
}

void gameRoom::loop(gameRoom *r) {
    int previousStackNum;
    r->giveCardsToPlayers();
    messenger::sendMsgAll(r->users, "S_CONSOLE_INFO:---GAME_STARTED---#\n");
    previousStackNum = r->info.cards.size();

    messenger::sendMsgAll(r->users, "S_ON_TURN:" + r->users.at(r->info.onTurnId).name + ":" +
                                    r->users.at(r->info.lastTurnId).name + "#\n");
    while (!r->info.isOver) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (previousStackNum < r->info.cards.size()) {
            r->nextPlayer();
            messenger::sendMsgAll(r->users, "S_ON_TURN:" + r->users.at(r->info.onTurnId).name + ":" +
                                            r->users.at(r->info.lastTurnId).name + "#\n");
            previousStackNum = r->info.cards.size();
        }
        if (r->info.cards.size() == 0 && previousStackNum != 0) previousStackNum = 0;

        r->checkOnlinePlayers();

    }
    r->roomStatus = RoomStatus::GAME_END;
    r->clearRoom(r);
}

void gameRoom::clearRoom(gameRoom *r) {
    r->isFull = false;
    r->info.isOver = false;
    for (int i = 0; i < users.size(); ++i) {
        users.at(i).isReady = false;
    }
    r->roomStatus = RoomStatus::ROOM_WAIT;
}

void gameRoom::giveCardsToPlayers() {
    double cardsforplayer = floor(info.cards.size() / numPlaying);
    for (int i = 0; i < numPlaying; ++i) {
        users.at(i).cards = std::vector<std::string>();
    }
    for (int i = 0; i < cardsforplayer; ++i) {
        for (int i = 0; i < numPlaying; ++i) {
            std::string karta = info.cards.back();
            users.at(i).cards.push_back(karta);
            info.cards.pop_back();
        }
    }
    for (int i = 0; i < users.size(); ++i) {
        messenger::sendMsg(users.at(i).uId, "S_CARDS_OWNED:" + getPlayerCards(i));
    }
    messenger::sendMsgAll(users, "S_STACK_CARDS:" + std::to_string(info.cards.size()) += "#\n");
}

void gameRoom::placeCard(int id, std::string card) {
    if(roomStatus==RoomStatus::GAME_WAITING){
        messenger::sendMsg(id, "S_CONSOLE_INFO:Čeká se na reconnect hráče#\n");
        return;
    }else if(roomStatus==RoomStatus::ROOM_WAIT || roomStatus==RoomStatus::GAME_END){
        messenger::sendMsg(id, "S_MSG_NOT_VALID#\n");
        return;
    }
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).uId == id && users.at(i).cards.size() == 0) {
            info.isOver = true;
            info.winner = id;
            messenger::sendMsgAll(users, "S_GAME_WINNER:" + users.at(info.winner).name + "#\n");
            return;
        } else if (users.at(i).uId == id && info.onTurnId == i) {
            for (int j = 0; j < users.at(i).cards.size(); ++j) {
                if (users.at(i).cards.at(j) == card) {
                    info.cards.push_back(users.at(i).cards.at(j));
                    users.at(i).cards.erase(users.at(i).cards.begin() + j);

                    if (users.at(info.lastTurnId).cards.size() == 0) {
                        info.isOver = true;
                        info.winner = info.lastTurnId;
                        messenger::sendMsgAll(users, "S_GAME_WINNER:" + users.at(info.winner).name + "#\n");
                        return;
                    } else if (users.at(i).cards.size() == 0 && info.cards.size() == 1) {
                        info.isOver = true;
                        info.winner = id;
                        messenger::sendMsgAll(users, "S_GAME_WINNER:" + users.at(info.winner).name + "#\n");
                        return;
                    }
                    info.lastTurnId = i;
                    messenger::sendMsg(users.at(info.onTurnId).uId, "S_CARD_ACK:" + info.cards.back() + "#\n");
                    return;
                }
            }
            messenger::sendMsg(users.at(info.onTurnId).uId, "S_NOT_VALID_CARD#\n");
            return;
        }
    }
    messenger::sendMsg(id, "S_MSG_NOT_VALID#\n");
}

void gameRoom::checkTopCard(int id) {
    if(roomStatus==RoomStatus::GAME_IN_PROGRESS) {
        if (info.cards.size() < 1) {
            messenger::sendMsg(id, "S_CONSOLE_INFO:Balíček je prázdný#\n");
            return;
        }else {
            for (int i = 0; i < users.size(); ++i) {
                if (users.at(i).uId == id) {
                    if (info.cards.front() == info.cards.back()) takePack(i);
                    else givePackToLast(i);
                    break;
                }
            }
        }
    }else{
        messenger::sendMsg(id, "S_MSG_NOT_VALID#\n");
    }
}

void gameRoom::givePackToLast(int pos) {
    int size = info.cards.size();
    for (int i = 0; i < size; ++i) {
        users.at(info.lastTurnId).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = pos;
    messenger::sendMsgAll(users, "S_ON_TURN:" + users.at(info.onTurnId).name + ":" + users.at(info.lastTurnId).name + "#\n");
    messenger::sendMsgAll(users, "S_CONSOLE_INFO:Hráč " + users.at(info.lastTurnId).name +
                                 " podváděl a bere balíček. Na tahu je hráč " + users.at(info.onTurnId).name + "#\n");
}

void gameRoom::takePack(int pos) {
    int size = info.cards.size();
    for (int i = 0; i < size; ++i) {
        users.at(pos).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = info.lastTurnId;
    messenger::sendMsgAll(users, "S_ON_TURN:" + users.at(info.onTurnId).name + ":" + users.at(info.lastTurnId).name + "#\n");
    messenger::sendMsgAll(users, "S_CONSOLE_INFO:Hráč " + users.at(info.lastTurnId).name +
                                 " nepodváděl a je na tahu. Balíček bere hráč " + users.at(pos).name + "#\n");
}

void gameRoom::nextPlayer() {
    if (users.size() - 1 != info.onTurnId) info.onTurnId++;
    else info.onTurnId = 0;
}

void gameRoom::checkOnlinePlayers() {
    timer disconnectTime;
    const int MAX_DISC_TIME = 45;

    int dcPlayer = getDcPlayer();

    if (dcPlayer != -1) {
        if (roomStatus != RoomStatus::ROOM_WAIT) {
            roomStatus = RoomStatus::GAME_WAITING;
            disconnectTime.start();
            messenger::sendMsgAllOthers(users.at(dcPlayer).uId, users, "S_DISCONNECT:"+users.at(dcPlayer).name + "#\n");
            while (disconnectTime.elapsedTime() < MAX_DISC_TIME) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (roomStatus == RoomStatus::GAME_IN_PROGRESS) {
                    messenger::sendMsgAllOthers(users.at(dcPlayer).uId, users, "S_RECONNECT:"+ users.at(dcPlayer).name+"#\n");
                    return;
                }
            }
        }
        for (int i = 0; i < users.size(); ++i) {
            if (!users.at(i).isOnline) {
                removePlayer(users.at(i).uId);
                i--;
            }
        }
        messenger::sendMsgAll(users, "S_GAME_END#\n");
    }
}

int gameRoom::getDcPlayer() {
    for (int i = 0; i < users.size(); ++i)
        if (!users.at(i).isOnline) return i;
    return -1;
}

void gameRoom::reconnect(int socket, int pos) {
    users.at(pos).isOnline = true;
    users.at(pos).uId = socket;
    roomStatus = RoomStatus::GAME_IN_PROGRESS;
    messenger::sendMsg(socket, "S_CARDS_OWNED:" + getPlayerCards(pos));
    messenger::sendMsg(socket, "S_STACK_CARDS:" + std::to_string(info.cards.size()) + "#\n");
    messenger::sendMsg(socket, "S_ON_TURN:" + users.at(info.onTurnId).name + ":" +
                                    users.at(info.lastTurnId).name + "#\n");}

std::string gameRoom::getPlayerCards(int i) {
    std::string cards = "";
    for (int j = 0; j < users.at(i).cards.size(); ++j) {
        cards += (users.at(i).cards.at(j) + ":");
    }
    cards.pop_back();
    return cards + "#\n";
}

void gameRoom::consoleOut(std::string msg) {
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "[%d-%m-%Y %H:%M:%S] ", timeinfo);
    std::string str(buffer);

    std::cout << str << msg << std::endl;
}
