//
// Created by lukado on 11.1.17.
//

#ifndef UPS_SERVER_MESSENGER_H
#define UPS_SERVER_MESSENGER_H

#include <string>
#include <sys/socket.h>
#include "gameRoom.h"

class messenger {

public:
    static void sendMsg(int socket, std::string message);

    static void sendMsgAll(std::vector<players::User> vector);
};


#endif //UPS_SERVER_MESSENGER_H
