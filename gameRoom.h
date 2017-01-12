//
// Created by Lukado on 27/10/16.
//

#ifndef UPS_SERVER_GAMEROOM_H
#define UPS_SERVER_GAMEROOM_H

#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <chrono>
#include "players.h"
#include "timer.h"
#include "stl.h"
#include "messenger.h"

class server;

class gameRoom {
public:
    gameRoom();

    struct gameInfo {
        int onTurnId;
        bool isOver;
        int lastTurnId;
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
        GAME_IN_PROGRESS,
        CARD_CHECK,
        GAME_END
    } roomStatus;

    int addPlayer(players::User player);

    bool removePlayer(int uId);

    bool isRoomFull();

    bool playerInOtherRoom(players::User player);

    bool playerAlreadyJoined(int uId);

    bool setPlayerReady(int playerId, bool ready);

    void allPlayersReady();

    void createNewGame();

    void clearRoom(gameRoom *r);

    void placeCard(int id, std::string basic_string);

    void checkTopCard(int id);

    void givePackToLast(int pos);

    void takePack(int pos);

private:
    std::thread gameThread;

    static void loop(gameRoom *r);

    void init();

    void giveCardsToPlayers();

    void nextPlayer();
};

#endif //UPS_SERVER_GAMEROOM_H
