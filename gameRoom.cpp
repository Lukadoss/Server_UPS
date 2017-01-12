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
        int packSize = r->info.cards.size();

        while(packSize==r->info.cards.size()){
//            checkOnlinePlayers();
//            checkCheat();
        }
        r->nextPlayer();
        std::string msg = "S_ON_TURN:" + std::to_string(r->info.onTurnId) +
                          "#" += '\n';
        messenger::sendMsg(r->users.at(r->info.onTurnId).uId, msg);
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

void gameRoom::placeCard(int id, std::string card) {
    for (int i = 0; i < users.size(); ++i) {
        if(users.at(i).uId == id && info.onTurnId == i){
            for (int j = 0; j < users.at(i).cards.size(); ++j) {
                if(users.at(i).cards.at(j) == card) {
                    info.cards.push_back(users.at(i).cards.at(j));
                    users.at(i).cards.erase(users.at(i).cards.begin()+j);
                    info.lastTurnId = i;
                    break;
                }
            }
            break;
        }
    }
}

void gameRoom::checkTopCard(int id){
    if (info.cards.size()<1) return;
    roomStatus = CARD_CHECK;
    for (int i = 0; i < users.size(); ++i) {
        if (users.at(i).uId == id) {
            if(info.cards.front() == info.cards.back()) takePack(i);
            else givePackToLast(i);
            break;
        }
    }
}

void gameRoom::givePackToLast(int pos){
    for (int i = 0; i < info.cards.size(); ++i) {
        users.at(info.lastTurnId).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = pos;
    roomStatus = GAME_IN_PROGRESS;
}

void gameRoom::takePack(int pos) {
    for (int i = 0; i < info.cards.size(); ++i) {
        users.at(pos).cards.push_back(info.cards.back());
        info.cards.pop_back();
    }
    info.onTurnId = info.lastTurnId;
    roomStatus = GAME_IN_PROGRESS;
}

void gameRoom::nextPlayer() {
    if (users.size()-1 != info.onTurnId) info.onTurnId++;
    else info.onTurnId = 0;
}
