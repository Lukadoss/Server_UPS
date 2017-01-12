//
// Created by Lukado on 27/10/16.
//

#include <iostream>
#include <unistd.h>
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
                users.push_back(player);

                numPlaying++;
                if (numPlaying == maxPlaying) {
                    isFull = true;
                }
                return roomId;
            } else return -1;
        } else return -1;
    } else return -1;
}

bool gameRoom::removePlayer(int uId) {
    for (unsigned int i = 0; i < users.size(); i++) {
        if (users.at(i).uId == uId) {
            info.isOver = true;
            users.erase(users.begin() + i);
            numPlaying--;
            isFull = false;
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

bool gameRoom::playerAlreadyJoined(int uId) {
    for (int i = 0; i < numPlaying; i++) {
        if (uId == users.at(i).uId) return true;
    }
    return false;
}

void gameRoom::setPlayerReady(int playerId, bool ready) {
    if (roomStatus == RoomStatus::ROOM_WAIT) {
        for (int i = 0; i < numPlaying; i++) {
            if (users.at(i).uId == playerId) {
                if (users.at(i).isReady == ready) {
                    messenger::sendMsg(playerId, "Uživatel již připraven");
                    break;
                } else {
                    users.at(i).isReady = ready;
                    messenger::sendMsgAll(users, "S_USR_READY:" + std::to_string(i) + "#" += '\n');
                    break;
                }
            }
        }
        allPlayersReady();
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
            createNewGame();
            roomStatus = RoomStatus::GAME_IN_PROGRESS;
        }
    }
}

void gameRoom::createNewGame() {
    init();
    info.onTurnId = 0;
    messenger::sendMsgAll(users, "S_ROOM_INFO:---GAME_STARTED---\n");
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

    messenger::sendMsgAll(r->users, "S_ON_TURN:" + std::to_string(r->info.onTurnId) + "#\n");

    r->giveCardsToPlayers();

    previousStackNum = r->info.cards.size();
    for (int i = 0; i < r->users.size(); ++i) {
        messenger::sendMsg(r->users.at(i).uId, r->getPlayerCards(i));
    }

    while (!r->info.isOver) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (previousStackNum < r->info.cards.size()) {
            messenger::sendMsg(r->users.at(r->info.onTurnId).uId, r->getPlayerCards(r->info.onTurnId));
            r->nextPlayer();
            messenger::sendMsgAll(r->users, "S_ON_TURN:" + std::to_string(r->info.onTurnId) + "#\n");
            previousStackNum = r->info.cards.size();
        }
        if (r->info.cards.size() == 0 && previousStackNum != 0) previousStackNum = 0;

        r->checkOnlinePlayers();
//          checkCheat();

    }
    messenger::sendMsgAll(r->users,
                          "S_ROOM_INFO:Hráč s id " + std::to_string(r->info.winner) + " je vítěz. Gratulace!" += '\n');
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
}

void gameRoom::placeCard(int id, std::string card) {
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).uId == id && users.at(i).cards.size() == 0) {
            info.isOver = true;
            info.winner = id;
            return;
        } else if (users.at(i).uId == id && info.onTurnId == i) {
            for (int j = 0; j < users.at(i).cards.size(); ++j) {
                if (users.at(i).cards.at(j) == card) {
                    info.cards.push_back(users.at(i).cards.at(j));
                    users.at(i).cards.erase(users.at(i).cards.begin() + j);

                    if (users.at(info.lastTurnId).cards.size() == 0) {
                        info.isOver = true;
                        info.winner = info.lastTurnId;
                        return;
                    } else if (users.at(i).cards.size() == 0 && info.cards.size() == 1) {
                        info.isOver = true;
                        info.winner = id;
                        return;
                    }
                    info.lastTurnId = i;
                    messenger::sendMsgAll(users, "S_PLACED_CARD:" + std::to_string(info.lastTurnId) + "#" += '\n');
                    break;
                }
            }
            break;
        }
    }
}

void gameRoom::checkTopCard(int id) {
    if (info.cards.size() < 1) return;
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).uId == id) {
            if (info.cards.front() == info.cards.back()) takePack(i);
            else givePackToLast(i);
            break;
        }
    }
}

void gameRoom::givePackToLast(int pos) {
    int size = info.cards.size();
    for (int i = 0; i < size; ++i) {
        users.at(info.lastTurnId).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = pos;
    messenger::sendMsgAll(users, "S_ROOM_INFO:Hráč s id " + std::to_string(info.lastTurnId) +
                                 " podváděl a bere balíček. Na tahu je hráč " + std::to_string(info.onTurnId) += '\n');
}

void gameRoom::takePack(int pos) {
    int size = info.cards.size();
    for (int i = 0; i < size; ++i) {
        users.at(pos).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = info.lastTurnId;
    messenger::sendMsgAll(users, "S_ROOM_INFO:Hráč s id " + std::to_string(info.lastTurnId) +
                                 " nepodváděl a je na tahu. Balíček bere hráč " + std::to_string(pos) += '\n');
}

void gameRoom::nextPlayer() {
    if (users.size() - 1 != info.onTurnId) info.onTurnId++;
    else info.onTurnId = 0;
}

void gameRoom::checkOnlinePlayers() {
    timer disconnectTime;
    const int MAX_DISC_TIME = 10;

    int dcPlayer = getDcPlayer();

    if (dcPlayer != -1) {
        if (roomStatus != RoomStatus::ROOM_WAIT) {
            roomStatus = RoomStatus::GAME_WAITING;
            disconnectTime.start();
            messenger::sendMsgAll(users, "S_ROOM_INFO:Hráč s id " + std::to_string(dcPlayer) +
                                         " ztratil spojení. Čeká se na reconnect.\n");
            while (disconnectTime.elapsedTime() < MAX_DISC_TIME) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                if (roomStatus == RoomStatus::GAME_IN_PROGRESS) {
                    messenger::sendMsgAll(users, "S_ROOM_INFO:Hráč s id " + std::to_string(dcPlayer) +
                                                 " se znovu připojil. Hra pokračuje!\n");
                    return;
                }
            }
        }
        for (int i = 0; i < users.size(); ++i) {
            if (!users.at(i).isOnline){
                shutdown(users.at(i).uId, SHUT_RDWR);
                close(users.at(i).uId);
                removePlayer(users.at(i).uId);
                i--;
            }
        }
        messenger::sendMsgAll(users,
                              "S_ROOM_INFO:Hráč s id " + std::to_string(dcPlayer) + " byl vyhozen ze hry. Hra končí\n");
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
    messenger::sendMsg(socket, getPlayerCards(pos));
}

std::string gameRoom::getPlayerCards(int i) {
    std::string cards = "S_PLAYER_CARDS:";
    for (int j = 0; j < users.at(i).cards.size(); ++j) {
        cards += (users.at(i).cards.at(j) + ":");
    }
    return cards + "#\n";
}
