//
// Created by Lukado on 28/10/16.
//

#ifndef UPS_SERVER_PLAYERS_H
#define UPS_SERVER_PLAYERS_H

#include <string>
#include <vector>

class players {
public:
    struct User {
        int uId;
        std::string name;
        int roomId;
        bool isReady;
        bool isOnline;
        std::vector<std::string> cards;
    };
};

#endif //UPS_SERVER_PLAYERS_H
