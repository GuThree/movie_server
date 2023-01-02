//
// Created by gsr on 2022/12/11.
//

#include "server.h"

ChatInfo *Server::chatlist = new ChatInfo;
ChatDataBase *Server::chatdb = new ChatDataBase;

Server::Server(int port)
{
    // 创建事件集合
    base = event_base_new();

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    server_addr.sin_addr.s_addr = inet_addr(ip);

    // 创建监听对象
    listener = evconnlistener_new_bind(base, listener_cb, NULL,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 10, (struct sockaddr *)&server_addr,
                                       sizeof(server_addr));
    if (listener == NULL)
    {
        cout << "evconnlistener_new_bind error" << endl;
        exit(0);
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

    event_base_dispatch(base);    // 监听集合（监听客户端是否有数据发送过来） 阻塞

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
    else if (cmd == "get_nick")
    {
        server_get_nickname(bev, val);
    }
    else if (cmd == "add_friend")
    {
        server_add_friend(bev, val);
    }
    else if (cmd == "create_room")
    {
        server_create_room(bev, val);
    }
    else if (cmd == "enter_room")
    {
        server_enter_room(bev, val);
    }
    else if (cmd == "private_chat")
    {
        server_private_chat(bev, val);
    }
    else if (cmd == "room_chat")
    {
        server_room_chat(bev, val);
    }
    else if (cmd == "get_room_member")
    {
        server_get_room_member(bev, val);
    }
    else if (cmd == "offline")
    {
        server_user_offline(bev, val);
    }
    else if (cmd == "send_file")
    {
        server_send_file(bev, val);
    }
}

void Server::event_cb(struct bufferevent *bev, short what, void *ctx)
{
}

void Server::server_register(struct bufferevent *bev, Json::Value val)
{
    chatdb->my_database_connect("m_user");

    if (chatdb->my_database_user_exist(val["username"].asString()))   // 用户已存在
    {
        Json::Value v;
        v["cmd"] = "register_reply";
        v["result"] = "failure";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
    }
    else                                               // 用户不存在 可注册
    {
        chatdb->my_database_user_password(val["username"].asString(), val["nickname"].asString(), val["password"].asString());
        Json::Value v;
        v["cmd"] = "register_reply";
        v["result"] = "success";

        Json::StreamWriterBuilder writerBuilder;
        unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

        string s = Json::writeString(writerBuilder, v);
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
    if (!chatdb->my_database_user_exist(val["username"].asString()))   // 用户不存在
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

    if (!chatdb->my_database_password_correct(val["username"].asString(),
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
    User u = {val["username"].asString(), val["nickname"].asString(), bev};
    chatlist->online_user->push_back(u);

    // 获取好友列表并且返回
    string friend_list;
    chatdb->my_database_get_friend(val["username"].asString(), friend_list);

    v["cmd"] = "login_reply";
    v["result"] = "success";
    v["friend"] = friend_list;
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
            if (name == it->username)
            {
                v.clear();
                v["cmd"] = "friend_login";
                v["friend"] = val["username"];
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

void Server::server_get_nickname(struct bufferevent *bev, Json::Value val)
{
    Json::Value v;
    string nick, s;

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    chatdb->my_database_connect("m_user");
    chatdb->my_database_get_nickname(val["username"].asString(), nick);

    v["cmd"] = "nickname_reply";
    v["result"] = "success";
    v["username"] = val["username"];
    v["nickname"] = nick;

    s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
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


    if (chatdb->my_database_is_friend(val["username"].asString(), val["friend"].asString()))
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
    chatdb->my_database_add_new_friend(val["username"].asString(), val["friend"].asString());
    chatdb->my_database_add_new_friend(val["friend"].asString(), val["username"].asString());

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
        if (val["friend"] == it->username)
        {
            v.clear();
            v["cmd"] = "add_friend_reply";
            v["result"] = val["username"];

            s = Json::writeString(writerBuilder, v);
            if (bufferevent_write(it->bev, s.c_str(), strlen(s.c_str())) < 0)
            {
                cout << "bufferevent_write" << endl;
            }
        }
    }

    chatdb->my_database_disconnect();
}

void Server::server_create_room(struct bufferevent *bev, Json::Value val)
{

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    // 判断群是否存在
    if (chatlist->info_room_exist(val["roomid"].asString()))
    {
        Json::Value v;
        v["cmd"] = "create_room_reply";
        v["result"] = "room_exist";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 修改群链表
    string nick;
    chatdb->my_database_connect("m_user");
    chatdb->my_database_get_nickname(val["username"].asString(), nick);
    chatlist->info_add_new_room(val["roomid"].asString(), val["username"].asString(), nick);
    chatdb->my_database_disconnect();

    Json::Value value;
    value["cmd"] = "create_room_reply";
    value["result"] = "success";
    value["roomid"] = val["roomid"];

    string s = Json::writeString(writerBuilder, value);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }
}

void Server::server_enter_room(struct bufferevent *bev, Json::Value val)
{
    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    // 判断群是否存在
    if (!chatlist->info_room_exist(val["roomid"].asString()))
    {
        Json::Value v;
        v["cmd"] = "enter_room_reply";
        v["result"] = "room_not_exist";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 判断用户是否在群里
    if (chatlist->info_user_in_room(val["roomid"].asString(), val["username"].asString()))
    {
        Json::Value v;
        v["cmd"] = "enter_room_reply";
        v["result"] = "user_in_room";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 修改链表
    chatlist->info_room_add_user(val["roomid"].asString(), val["username"].asString(), val["nickname"].asString());

    Json::Value v;
    v["cmd"] = "enter_room_reply";
    v["result"] = "success";
    v["roomid"] = val["roomid"];

    string s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }
}

void Server::server_private_chat(struct bufferevent *bev, Json::Value val)
{
    struct bufferevent *to_bev = chatlist->info_get_friend_bev(val["user_to"].asString());

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    if (to_bev == NULL)
    {
        Json::Value v;
        v["cmd"] = "private_chat_reply";
        v["result"] = "offline";

        string s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    string s = Json::writeString(writerBuilder, val);
    if (bufferevent_write(to_bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    Json::Value v;
    v["cmd"] = "private_chat_reply";
    v["result"] = "success";

    s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }
}

void Server::server_room_chat(struct bufferevent *bev, Json::Value val)
{
    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    for (list<Room>::iterator it = chatlist->room_info->begin(); it != chatlist->room_info->end(); it++)
    {
        if (val["roomid"].asString() == it->roomid)
        {
            for (list<RoomUser>::iterator i = it->l->begin(); i != it->l->end(); i++)
            {
                struct bufferevent *to_bev = chatlist->info_get_friend_bev(i->username);
                if (to_bev != NULL)
                {
                    string s = Json::writeString(writerBuilder, val);
                    if (bufferevent_write(to_bev, s.c_str(), strlen(s.c_str())) < 0)
                    {
                        cout << "bufferevent_write" << endl;
                    }
                }
            }
        }
    }

    Json::Value v;
    v["cmd"] = "room_chat_reply";
    v["result"] = "success";

    string s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }
}

void Server::server_get_room_member(struct bufferevent *bev, Json::Value val)
{
    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    string member = chatlist->info_get_room_member(val["roomid"].asString());

    Json::Value v;
    v["cmd"] = "get_room_member_reply";
    v["member"] = member;
    v["roomid"] = val["roomid"];

    string s = Json::writeString(writerBuilder, v);
    if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

}

void Server::server_user_offline(struct bufferevent *bev, Json::Value val)
{
    // 从链表中删除用户
    for (list<User>::iterator it = chatlist->online_user->begin();
         it != chatlist->online_user->end(); it++)
    {
        if (it->username == val["user"].asString())
        {
            chatlist->online_user->erase(it);
            break;
        }
    }

    chatdb->my_database_connect("m_user");

    // 获取好友列表并且返回
    string friend_list;
    string name, s;
    Json::Value v;

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    chatdb->my_database_get_friend(val["username"].asString(), friend_list);

    // 向好友发送下线提醒
    int start = 0, end = 0, flag = 1;
    while (flag)
    {
        end = friend_list.find('|', start);
        if (-1 == end)
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
            if (name == it->username)
            {
                v.clear();
                v["cmd"] = "friend_offline";
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

void Server::send_file_handler(int length, int port, int *f_fd, int *t_fd)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        return;
    }

    int opt = 1;
    setsockopt(sockfd,SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 接收缓冲区
    int nRecvBuf = MAXSIZE;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
    // 发送缓冲区
    int nSendBuf = MAXSIZE;    // 设置为1M
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (const char*)&nSendBuf, sizeof(int));

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
//    server_addr.sin_addr.s_addr = inet_addr(IP);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(sockfd, 10);

    int len = sizeof(client_addr);
    // 接受发送客户端的连接请求
    *f_fd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len);
    // 接受接收客户端的连接请求
    *t_fd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&len);

    char buf[MAXSIZE] = {0};
    size_t size, sum = 0;
    while (1)
    {
        size = recv(*f_fd, buf, MAXSIZE, 0);
        if (size <= 0 || size > MAXSIZE)
        {
            break;
        }
        sum += size;
        send(*t_fd, buf, size, 0);
        if (sum >= length)
        {
            break;
        }
        memset(buf, 0, MAXSIZE);
    }

    close(*f_fd);
    close(*t_fd);
    close(sockfd);
}

void Server::server_send_file(struct bufferevent *bev, Json::Value val)
{
    Json::Value v;
    string s;

    Json::StreamWriterBuilder writerBuilder;
    unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());

    // 判断对方是否在线
    struct bufferevent *to_bev = chatlist->info_get_friend_bev(val["to_user"].asString());
    if (to_bev == NULL)
    {
        v["cmd"] = "send_file_reply";
        v["result"] = "offline";
        s = Json::writeString(writerBuilder, v);
        if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
        {
            cout << "bufferevent_write" << endl;
        }
        return;
    }

    // 启动新线程，创建文件服务器
    int port = 8080, from_fd = 0, to_fd = 0;
    thread send_file_thread(send_file_handler, val["length"].asInt(), port, &from_fd, &to_fd);
    send_file_thread.detach();

    v.clear();
    v["cmd"] = "send_file_port_reply";
    v["port"] = port;
    v["filename"] = val["filename"];
    v["length"] = val["length"];
    s = Json::writeString(writerBuilder, v);
    //if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
    if (send(bev->ev_read.ev_fd, s.c_str(), strlen(s.c_str()), 0) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    int count = 0;
    while (from_fd <= 0)
    {
        count++;
        usleep(100000);
        if (count == 100)
        {
            pthread_cancel(send_file_thread.native_handle());   // 取消线程
            v.clear();
            v["cmd"] = "send_file_reply";
            v["result"] = "timeout";
            s = Json::writeString(writerBuilder, v);
            if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
            {
                cout << "bufferevent_write" << endl;
            }
            return;
        }
    }

    // 返回端口号给接收客户端
    v.clear();
    v["cmd"] = "recv_file_port_reply";
    v["port"] = port;
    v["filename"] = val["filename"];
    v["length"] = val["length"];
    s = Json::writeString(writerBuilder, v);
    //if (bufferevent_write(to_bev, s.c_str(), strlen(s.c_str())) < 0)
    if (send(to_bev->ev_read.ev_fd, s.c_str(), strlen(s.c_str()), 0) < 0)
    {
        cout << "bufferevent_write" << endl;
    }

    count = 0;
    while (to_fd <= 0)
    {
        count++;
        usleep(100000);
        if (count == 100)
        {
            pthread_cancel(send_file_thread.native_handle());   //取消线程
            v.clear();
            v["cmd"] = "send_file_reply";
            v["result"] = "timeout";
            s = Json::writeString(writerBuilder, v);
            if (bufferevent_write(bev, s.c_str(), strlen(s.c_str())) < 0)
            {
                cout << "bufferevent_write" << endl;
            }
        }
    }
}
