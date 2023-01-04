//
// Created by gsr on 2022/12/14.
//

#ifndef MOVIE_SERVER_CHAT_DATABASE_H
#define MOVIE_SERVER_CHAT_DATABASE_H

#include <mysql/mysql.h>
#include <iostream>
#include <stdio.h>
#include <string.h>

using namespace std;

class ChatDataBase
{
private:
    MYSQL *mysql;
public:
    ChatDataBase();
    ~ChatDataBase();

    void my_database_connect(const char *name);
    bool my_database_user_exist(string);
    void my_database_user_password(string, string, string);
    bool my_database_password_correct(string, string);
    bool my_database_is_friend(string, string);
    void my_database_get_friend(string, string &, string &);
    void my_database_add_new_friend(string, string, string);
    void my_database_get_nickname(string, string &);
    void my_database_disconnect();
};

#endif //MOVIE_SERVER_CHAT_DATABASE_H
