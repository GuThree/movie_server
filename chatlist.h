//
// Created by gsr on 2022/12/14.
//

#ifndef MOVIE_SERVER_CHATLIST_H
#define MOVIE_SERVER_CHATLIST_H

#include <event.h>
#include <list>
#include <string>
#include "chat_database.h"

using namespace std;

#define MAXNUM    1024     //表示群最大个数


struct User
{
    string username;
    string nickname;
    struct bufferevent *bev;
};
typedef struct User User;

struct RoomUser
{
    string username;
    string nickname;
};

typedef struct RoomUser RoomUser;

struct Room
{
    string roomid;
    list<RoomUser> *l;
};
typedef struct Room Room;

class Server;

class ChatInfo
{
    friend class Server;
private:
    list<User> *online_user;     // 保存所有在线的用户信息
    list<Room> *room_info;     // 保存所有房间信息
    ChatDataBase *mydatabase;    // 数据库对象

public:
    ChatInfo();
    ~ChatInfo();

    bool info_room_exist(string);
    bool info_user_in_room(string, string);
    void info_room_add_user(string, string, string);
    void info_room_del_user(string, string);
    struct bufferevent *info_get_friend_bev(string);
    string info_get_room_member(string);
    void info_add_new_room(string, string, string);
};

#endif //MOVIE_SERVER_CHATLIST_H
