//
// Created by Lukado on 27/10/16.
//

#include "messenger.h"

void messenger::sendMsg(int socket, std::string msg) {
    const char *msgChar = msg.c_str();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    send(socket, (void *) msgChar, msg.length(), 0);
}

void messenger::sendMsgAll(std::vector<players::User> players, std::string msg) {
    for (int i = 0; i < players.size(); i++) {
        if (players.at(i).uId != 0 && players.at(i).isOnline) {
            sendMsg(players.at(i).uId, msg);
        }
    }
}

void messenger::sendMsgAllOthers(int uid, std::vector<players::User> players, std::string msg) {
    for (int i = 0; i < players.size(); i++) {
        if (players.at(i).uId != 0 && players.at(i).uId != uid && players.at(i).isOnline) {
            sendMsg(players.at(i).uId, msg);
        }
    }
}
