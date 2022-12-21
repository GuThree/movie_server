//
// Created by gsr on 2022/12/11.
//

#include "server.h"

ChatInfo *Server::chatlist = new ChatInfo;
ChatDataBase *Server::chatdb = new ChatDataBase;

Server::Server(const char *ip, int port)
{
    chatlist = new ChatInfo;

    // 创建事件集合
    base = event_base_new();

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 创建监听对象
    listener = evconnlistener_new_bind(base, listener_cb, NULL,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 10, (struct sockaddr *)&server_addr,
                                       sizeof(server_addr));
    if (listener == NULL)
    {
        cout << "evconnlistener_new_bind error" << endl;
    }

    cout << "服务器初始化成功 开始监听客户端" << endl;
    event_base_dispatch(base);       // 监听集合
}

Server::~Server()
{
    event_base_free(base);
}

// 当有客户端发起连接的时候，会触发该函数
void Server::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg)
{
    cout << "接受客户端的连接，fd = " << fd << endl;

    // 创建工作线程来处理该客户端
    thread client_thread(client_handler, fd);
    client_thread.detach();    // 线程分离，当线程运行结束后，自动释放资源
}

void Server::client_handler(int fd)
{
    // 创建集合
    struct event_base *base = event_base_new();

    // 创建bufferevent对象
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (bev == NULL)
    {
        cout << "bufferevent_socket_new error" << endl;
    }

    // 给bufferevent设置回调函数
    bufferevent_setcb(bev, read_cb, NULL, event_cb, NULL);

    // 使能回调函数
    bufferevent_enable(bev, EV_READ);

    event_base_dispatch(base);    // 监听集合（监听客户端是否有数据发送过来）

    event_base_free(base);
    cout << "线程退出、释放集合" << endl;
}

void Server::read_cb(struct bufferevent *bev, void *ctx)
{
    char buf[1024] = {0};
    int size = bufferevent_read(bev, buf, sizeof(buf));
    if (size < 0)
    {
        cout << "bufferevent_read error" << endl;
    }

    cout << buf << endl;

    Json::CharReaderBuilder readerBuilder;       // 解析json对象
    Json::StreamWriterBuilder writer;   // 封装json对象
    std::unique_ptr<Json::CharReader>reader(readerBuilder.newCharReader());
    Json::Value val;
    Json::String jsErrors;

    if (!reader->parse(buf, buf + strlen(buf), &val, &jsErrors))    //把字符串转换成 json 对象
    {
        cout << "服务器解析数据失败" << endl;
    }

    string cmd = val["cmd"].asString();

    if (cmd == "register")   // 注册功能
    {
        server_register(bev, val);
    }
    else if (cmd == "login")
    {
        server_login(bev, val);
    }
    else if (cmd == "add_friend")
    {
        server_add_friend(bev, val);
    }
    else if (cmd == "create_group")
    {
        server_create_group(bev, val);
    }
    else if (cmd == "add_group")
    {
        server_add_group(bev, val);
    }

}

void Server::event_cb(struct bufferevent *bev, short what, void *ctx)
{
}

void Server::server_register(struct bufferevent *bev, Json::Value val)
{
    chatdb->my_database_connect("m_user");

    if (chatdb->my_database_user_exist(val["user"].asString()))   // 用户已存在
    {
        Json::Value val;
        val["cmd"] = "register_reply";
        val["result"] = "failure";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, val);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
    }
    else                                               // 用户不存在 可注册
    {
        chatdb->my_database_user_password(val["user"].asString(), val["password"].asString());
        Json::Value val;
        val["cmd"] = "register_reply";
        val["result"] = "success";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, val);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
    }

    chatdb->my_database_disconnect();
}

void Server::server_login(struct bufferevent *bev, Json::Value val)
{
    chatdb->my_database_connect("m_user");
    if (!chatdb->my_database_user_exist(val["user"].asString()))   // 用户不存在
    {
        Json::Value val;
        val["cmd"] = "login_reply";
        val["result"] = "user_not_exist";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, val);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    if (!chatdb->my_database_password_correct(val["user"].asString(),
                                              val["password"].asString()))    // 密码不正确
    {
        Json::Value val;
        val["cmd"] = "login_reply";
        val["result"] = "password_error";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, val);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    Json::Value v;
    string s, name;

    // 向链表中添加用户
    User u = {val["user"].asString(), bev};
    chatlist->online_user->push_back(u);

    // 获取好友列表并且返回
    string friend_list, group_list;
    chatdb->my_database_get_friend_group(val["user"].asString(), friend_list, group_list);

    v["cmd"] = "login_reply";
    v["result"] = "success";
    v["friend"] = friend_list;
    v["group"] = group_list;
    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    // 向好友发送上线提醒
    int start = 0, end = 0, flag = 1;
    while (flag)
    {
        end = friend_list.find('|', start);
        if (end == -1)
        {
            name = friend_list.substr(start, friend_list.size() - start);
            flag = 0;
        }
        else
        {
            name = friend_list.substr(start, end - start);
        }

        for (list<User>::iterator it = chatlist->online_user->begin();
             it != chatlist->online_user->end(); it++)
        {
            if (name == it->name)
            {
                v.clear();
                v["cmd"] = "friend_login";
                v["friend"] = val["user"];
                s = Json::writeString(writerBuilder, v);
                if (bufferevent_write(it->bev, s.c_str(), strlen(s.c_str())) < 0)
                {
                    cout << "bufferevent_write" << endl;
                }
            }
        }
        start = end + 1;
    }

    chatdb->my_database_disconnect();
}

void  Server::server_add_friend(struct bufferevent *bev, Json::Value val)
{
    Json::Value v;
    string s;

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    chatdb->my_database_connect("m_user");

    if (!chatdb->my_database_user_exist(val["friend"].asString()))   // 添加的好友不存在
    {
        v["cmd"] = "add_reply";
        v["result"] = "user_not_exist";

        s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }


    if (chatdb->my_database_is_friend(val["user"].asString(), val["friend"].asString()))
    {
        v.clear();
        v["cmd"] = "add_reply";
        v["result"] = "already_friend";

        s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 修改双方的数据库
    chatdb->my_database_add_new_friend(val["user"].asString(), val["friend"].asString());
    chatdb->my_database_add_new_friend(val["friend"].asString(), val["user"].asString());

    v.clear();
    v["cmd"] = "add_reply";
    v["result"] = "success";
    v["friend"] = val["friend"];

    s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    // 回复被添加的对方
    for (list<User>::iterator it = chatlist->online_user->begin();
         it != chatlist->online_user->end(); it++)
    {
        if (val["friend"] == it->name)
        {
            v.clear();
            v["cmd"] = "add_friend_reply";
            v["result"] = val["user"];

            s = Json::writeString(writerBuilder, v);
            if (bufferevent_write(it->bev, s.c_str(), strlen(s.c_str())) < 0)
            {
                cout << "bufferevent_write" << endl;
            }
        }
    }

    chatdb->my_database_disconnect();
}

void Server::server_create_group(struct bufferevent *bev, Json::Value val)
{
    chatdb->my_database_connect("m_group");

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    // 判断群是否存在
    if (chatdb->my_database_group_exist(val["group"].asString()))
    {
        Json::Value v;
        v["cmd"] = "create_group_reply";
        v["result"] = "group_exist";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 把群信息写入数据库
    chatdb->my_database_add_new_group(val["group"].asString(), val["user"].asString());
    chatdb->my_database_disconnect();

    chatdb->my_database_connect("m_user");
    // 修改数据库个人信息
    chatdb->my_database_user_add_group(val["user"].asString(), val["group"].asString());
    //修改群链表
//    chatlist->info_add_new_group(val["group"].asString(), val["user"].asString());

    Json::Value value;
    value["cmd"] = "create_group_reply";
    value["result"] = "success";
    value["group"] = val["group"];

    string s = Json::writeString(writerBuilder, value);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    chatdb->my_database_disconnect();
}

void Server::server_add_group(struct bufferevent *bev, Json::Value val)
{
    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    // 判断群是否存在
    if (!chatlist->info_group_exist(val["group"].asString()))
    {
        Json::Value v;
        v["cmd"] = "add_group_reply";
        v["result"] = "group_not_exist";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 判断用户是否在群里
    if (chatlist->info_user_in_group(val["group"].asString(), val["user"].asString()))
    {
        Json::Value v;
        v["cmd"] = "add_group_reply";
        v["result"] = "user_in_group";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 修改数据库（用户表 群表）
    chatdb->my_database_connect("m_user");
    chatdb->my_database_user_add_group(val["user"].asString(), val["group"].asString());
    chatdb->my_database_disconnect();

    chatdb->my_database_connect("m_group");
    chatdb->my_database_group_add_user(val["group"].asString(), val["user"].asString());
    chatdb->my_database_disconnect();

    // 修改链表
    chatlist->info_group_add_user(val["group"].asString(), val["user"].asString());

    Json::Value v;
    v["cmd"] = "add_group_reply";
    v["result"] = "success";
    v["group"] = val["group"];

    string s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }
}
