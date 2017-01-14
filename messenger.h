//
// Created by Lukado on 27/10/16.
//

#ifndef UPS_SERVER_MESSENGER_H
#define UPS_SERVER_MESSENGER_H

#include <string>
#include <sys/socket.h>
#include "players.h"
#include <thread>

class messenger {

public:
    static void sendMsg(int socket, std::string message);

    static void sendMsgAll(std::vector<players::User> players, std::string msg);

    static void sendMsgAllOthers(int uid, std::vector<players::User> players, std::string msg);
};


#endif //UPS_SERVER_MESSENGER_H
