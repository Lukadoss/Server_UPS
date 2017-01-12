//
// Created by Lukado on 27/10/16.
//

#include "messenger.h"

void messenger::sendMsg(int socket, std::string msg) {
    const char *msgChar = msg.c_str();
    send(socket, (void *) msgChar, msg.length(), 0);
}

void messenger::sendMsgAll(std::vector<players::User> players) {
    for (int i = 0; i < players.size(); i++) {
        if (players.at(i).uId != 0) {
            sendMsg(players.at(i).uId,
                    "S_USR_READY:" + std::to_string(i) + "#" += '\n');
        }
    }
}
