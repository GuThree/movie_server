//
// Created by gsr on 2022/12/11.
//

#ifndef MOVIE_SERVER_SERVER_H
#define MOVIE_SERVER_SERVER_H

#include <event.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <event2/listener.h>
#include "chatlist.h"
#include <json/json.h>
#include <unistd.h>
#include<cstdlib>

using namespace std;

//#define IP     "172.28.184.34"
#define PORT   8000
#define MAXSIZE  1024 * 1024

class Server {
private:
    struct event_base *base;                    // 事件集合
    struct evconnlistener *listener;            // 监听事件
    static ChatInfo *chatlist;                  // 链表对象（含有两个链表）
    static ChatDataBase *chatdb;                // 数据库对象

private:
    static void listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg);
    static void client_handler(int);
    static void read_cb(struct bufferevent *bev, void *ctx);
    static void event_cb(struct bufferevent *bev, short what, void *ctx);

    static void server_register(struct bufferevent *bev, Json::Value val);
    static void server_login(struct bufferevent *bev, Json::Value val);
    static void server_get_nickname(struct bufferevent *bev, Json::Value val);
    static void server_add_friend(struct bufferevent *bev, Json::Value val);
    static void server_create_room(struct bufferevent *bev, Json::Value val);
    static void server_enter_room(struct bufferevent *bev, Json::Value val);
    static void server_invite_room(struct bufferevent *bev, Json::Value val);
    static void server_leave_room(struct bufferevent *bev, Json::Value val);
    static void server_private_chat(struct bufferevent *bev, Json::Value val);
    static void server_room_chat(struct bufferevent *bev, Json::Value val);
    static void server_get_room_member_id(struct bufferevent *bev, Json::Value val);
    static void server_get_room_member(struct bufferevent *bev, Json::Value val);
    static void server_user_offline(struct bufferevent *bev, Json::Value val);
    static void server_kick_member(struct bufferevent *bev, Json::Value val);
    static void send_file_handler(int, int, int *, int *);
    static void server_send_file(struct bufferevent *bev, Json::Value val);

public:
    Server(int port = 8000);
    ~Server();
};


#endif //MOVIE_SERVER_SERVER_H
