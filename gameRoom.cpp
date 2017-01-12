//
// Created by Lukado on 27/10/16.
//

#include <iostream>
#include "gameRoom.h"

gameRoom::gameRoom() {
    roomStatus = RoomStatus::ROOM_WAIT;
    info.isOver = false;
}

int gameRoom::addPlayer(players::User player) {
    if (!playerInOtherRoom(player)) {
        if (!isRoomFull()) {
            if (!playerAlreadyJoined(player.uId)) {
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
            users.erase(users.begin()+i);
            numPlaying--;
            isFull = false;
            return true;
        }
    }
    return false;
}

bool gameRoom::isRoomFull() {
    return isFull;
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

bool gameRoom::setPlayerReady(int playerId, bool ready) {
    if (roomStatus==RoomStatus::ROOM_WAIT) {
        for (int i = 0; i < numPlaying; i++) {
            if (users.at(i).uId == playerId) {
                users.at(i).isReady = ready;
                messenger::sendMsgAll(users, "S_USR_READY:" + std::to_string(i) + "#" += '\n');
                break;
            }
        }
        allPlayersReady();
    }
    return false;
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

    gameThread = std::thread(loop, this);
    gameThread.detach();
}

void gameRoom::init(){
    std::string card = "";
    info.cards = std::vector<std::string>();

    for (int i = 0; i < 4; i++) {
        switch (i){
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
    timer gameElapsed;

    std::string msg = "S_ON_TURN:" + std::to_string(r->info.onTurnId) +
                 "#" += '\n';
    messenger::sendMsg(r->users.at(r->info.onTurnId).uId, msg);

    r->giveCardsToPlayers();
    gameElapsed.start();
    while (!r->info.isOver) {

    }

    r->clearRoom(r);
}

void gameRoom::clearRoom(gameRoom *r) {
    r->isFull = false;
    r->roomStatus = RoomStatus::ROOM_WAIT;
    for (int i = 0; i < users.size(); ++i) {
        users.at(i).isReady = false;
    }
}

void gameRoom::giveCardsToPlayers() {
    double cardsforplayer = floor(info.cards.size()/numPlaying);
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


std::string gameRoom::getString(gameRoom::RoomStatus status) {
    return std::string();
}
