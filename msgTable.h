//
// Created by Lukado on 22/10/16.
//

#ifndef UPS_SERVER_MSGTABLE_H
#define UPS_SERVER_MSGTABLE_H

#include <string>

class msgtable {
public:
    enum msgTypes {
        C_LOGIN,
        C_LOGOUT,
        C_GET_TABLE,
        C_JOIN_ROOM, //User joins room
        C_LEAVE_ROOM, //User leaves room
        C_ROW_UPDATE, //Updates one row in table
        C_ROOM_INFO, //Info about room
        C_ROOM_USERS, //Info about users in room
        C_USR_READY,
        C_USR_NREADY,
        C_PUT_CARD,
        C_CHECK_CHEAT,
        EOS,
        ERR,
        NO_CODE
    };

    static msgTypes getType(std::string msg);
};

#endif //UPS_SERVER_MSGTABLE_H
