//
// Created by gsr on 2022/12/14.
//

#include "chat_database.h"

ChatDataBase::ChatDataBase()
{
}

ChatDataBase::~ChatDataBase()
{
    mysql_close(mysql);
}

void ChatDataBase::my_database_connect(const char *name)
{
    mysql = mysql_init(NULL);
    mysql = mysql_real_connect(mysql, "127.0.0.1", "root", "root", name, 0, NULL, 0);
    if (mysql == NULL)
    {
        cout << "connect database failure" << endl;
    }

    if (mysql_query(mysql, "set names utf8;") != 0)
    {
        cout << "mysql_query error" << endl;
    }
}

void ChatDataBase::my_database_disconnect()
{
    mysql_close(mysql);
}

bool ChatDataBase::my_database_user_exist(string name)
{

    char sql[128] = {0};
    sprintf(sql, "show tables like '%s';", name.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == NULL)      // 用户不存在
        return false;
    else                  // 用户存在
        return true;
}

void ChatDataBase::my_database_user_password(string username, string nickname, string password)
{
    char sql[128] = {0};
    sprintf(sql, "create table %s (password varchar(16), nickname varchar(16), friends_id varchar(4096), friends_nick varchar(4096)) character set utf8;"
            , username.c_str());

    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "insert into %s (password, nickname) values ('%s', '%s');",
            username.c_str(), password.c_str(), nickname.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }
}

bool ChatDataBase::my_database_password_correct(string name, string password)
{
    char sql[128] = {0};
    sprintf(sql, "select password from %s;", name.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (password == row[0])
        return true;
    else
        return false;
}

void ChatDataBase::my_database_get_friend(string name, string &f_id, string &f_nick)
{
    char sql[128] = {0};
    sprintf(sql, "select friends_id, friends_nick from %s;", name.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }
    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row[0] != NULL)
    {
        f_id.append(row[0]);
    }
    if (row[1] != NULL)
    {
        f_nick.append(row[1]);
    }
    mysql_free_result(res);
}

bool ChatDataBase::my_database_is_friend(string n1, string n2)
{
    char sql[128] = {0};
    sprintf(sql, "select friends_id from %s;", n1.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row[0] == NULL)
    {
        return false;
    }
    else
    {
        string all_friend(row[0]);
        int start = 0, end = 0;
        while (1)
        {
            end = all_friend.find('|', start);
            if (end == -1)
            {
                break;
            }
            if (n2 == all_friend.substr(start, end - start))
            {
                return true;
            }

            start = end + 1;
        }

        if (n2 == all_friend.substr(start, all_friend.size() - start))
        {
            return true;
        }
    }

    return false;
}

void ChatDataBase::my_database_add_new_friend(string n1, string n2, string n3)
{
    char sql[1024] = {0};
    sprintf(sql, "select friends_id, friends_nick from %s;", n1.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query" << endl;
    }
    string friend_id, friend_nick;
    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row[0] == NULL)    // 原来没有好友
    {
        friend_id.append(n2);
        friend_nick.append(n3);
    }
    else
    {
        friend_id.append(row[0]);
        friend_nick.append(row[1]);
        friend_id += "|";
        friend_id += n2;
        friend_nick += "|";
        friend_nick += n3;
    }

    memset(sql, 0, sizeof(sql));
    sprintf(sql, "update %s set friends_id = '%s', friends_nick = '%s';", n1.c_str(), friend_id.c_str(), friend_nick.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }
}

void ChatDataBase::my_database_get_nickname(string username, string &nickname)
{
    char sql[128] = {0};
    sprintf(sql, "select nickname from %s;", username.c_str());
    if (mysql_query(mysql, sql) != 0)
    {
        cout << "mysql_query error" << endl;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    nickname = row[0];
}