//
// Created by gsr on 2022/12/14.
//

#include "chatlist.h"

ChatInfo::ChatInfo()
{
    online_user = new list<User>;

    room_info = new list<Room>;

    cout << "初始化链表成功" << endl;
}

bool ChatInfo::info_room_exist(string room_name)
{
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (it->roomid == room_name)
        {
            return true;
        }
    }
    return false;
}

bool ChatInfo::info_user_in_room(string room_id, string user_name)
{
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (it->roomid == room_id)
        {
            for (list<RoomUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
            {
                if (i->username == user_name)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void ChatInfo::info_room_add_user(string room_name, string user_name, string user_nick)
{
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (it->roomid == room_name)
        {
            RoomUser u;
            u.username = user_name;
            u.nickname = user_nick;
            it->l->push_back(u);
        }
    }
}

void ChatInfo::info_room_del_user(string room_name, string user_name)
{
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (it->roomid == room_name)
        {
            for (list<RoomUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
            {
                if (i->username == user_name)
                {
                    it->l->erase(i);
                    break;
                }
            }
        }
    }
}

struct bufferevent *ChatInfo::info_get_friend_bev(string username)
{
    for (list<User>::iterator it = online_user->begin(); it != online_user->end(); it++)
    {
        if (it->username == username)
        {
            return it->bev;
        }
    }
    return NULL;
}

string ChatInfo::info_get_room_member_id(string room)
{
    string member;
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (room == it->roomid)
        {
            for (list<RoomUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
            {
                member += i->username;
                member += "|";
            }
        }
    }
    member.pop_back();
    return member;
}

string ChatInfo::info_get_room_member(string room)
{
    string member;
    for (list<Room>::iterator it = room_info->begin(); it != room_info->end(); it++)
    {
        if (room == it->roomid)
        {
            for (list<RoomUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
            {
                member += i->nickname;
                member += "|";
            }
        }
    }
    member.pop_back();
    return member;
}

void ChatInfo::info_add_new_room(string room_name, string user_name, string user_nick)
{
    Room r;
    r.roomid = room_name;
    r.l = new list<RoomUser>;
    room_info->push_back(r);

    RoomUser u;
    u.username = user_name;
    u.nickname = user_nick;
    r.l->push_back(u);
}