//
// Created by Lukado on 28/10/16.
//

#ifndef UPS_SERVER_PLAYERS_H
#define UPS_SERVER_PLAYERS_H

#include <string>
#include <vector>
#include <thread>

class players {
public:
    struct User {
        int uId;
        std::string name;
        int roomId;
        bool isReady;
        bool isOnline;
        std::vector<std::string> cards;
        struct timespec lastPing;
        int socketPos;
    };
};

#endif //UPS_SERVER_PLAYERS_H
