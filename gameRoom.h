//
// Created by seda on 27/10/16.
//

#ifndef UPS_SERVER_GAMEROOM_H
#define UPS_SERVER_GAMEROOM_H

#include <vector>
#include <string>
#include <thread>
#include "players.h"
#include "timer.h"
#include "stl.h"
#include <algorithm>
#include <chrono>
#include "messenger.h"

class server;

class players;

class gameRoom {
public:
    gameRoom();

    struct gameInfo {
        int onTurnId;
        bool isOver;
        int lastTurnUID;
        std::vector<std::string> cards;
    };

    int roomId;
    std::string roomName;
    std::vector<players::User> users;
    unsigned long numPlaying;
    unsigned long maxPlaying;
    bool isFull;
    gameInfo info;

    enum RoomStatus {
        ROOM_WAIT,
        ROOM_IN_PROGRESS,
        ROOM_END_GAME
    } roomStatus;

    int addPlayer(players::User player);

    bool removePlayer(int uId);

    bool isRoomFull();

    bool playerInOtherRoom(players::User player);

    bool playerAlreadyJoined(int uId);

    bool setPlayerReady(int playerId, bool ready);

    void allPlayersReady();

    void createNewGame();

    void turnCard(int playerId, int row, int col);

    void addTurned();

    void getRoomWinner(gameRoom *r, server *s);

    void clearRoom(gameRoom *r);

    static std::string getString(RoomStatus status);

private:
    std::thread gameThread;

    static void loop(gameRoom *r);

    void shuffleDeck();

    void sendToPlayers(gameRoom *r, server *s, std::string msg);

    bool allTurnedBack(gameRoom *r);

    void init();

    void giveCardsToPlayers();

};

#endif //UPS_SERVER_GAMEROOM_H
