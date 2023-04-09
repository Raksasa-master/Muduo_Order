#include <iostream>
#include <bits/stdc++.h>
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"
#include "codec.hpp"
using namespace muduo;
using namespace muduo::net;
class OrderServer : noncopyable
{
public:
    OrderServer(EventLoop* loop,
                const InetAddress& listenAddr,DishTable* dishTable,OrderTable* orderTable)
                : server_(loop,listenAddr,"OrderServer"),
                  codec_(std::bind(&OrderServer::onStringMessage,this,_1,_2,_3),dishTable,orderTable)
    {
        server_.setConnectionCallback(
                std::bind(&OrderServer::onConnection, this, _1));
        server_.setMessageCallback(
                std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }


    void start()
    {
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        LOG_INFO << conn->peerAddress().toIpPort() << "->"
                 << conn->localAddress().toIpPort() << "->"
                 << (conn->connected() ? "UP" : "DOWN");
        if (conn->connected())
        {
            connections_[conn->peerAddress().toIpPort()]=conn;
        }
        else
        {
            connections_.erase(conn->peerAddress().toIpPort());
        }
    }
    void onStringMessage(const TcpConnectionPtr&,
                         const string& message,
                         Timestamp)
    {
        std::string result;
        std::string accept;
        codec_.Task_Handle(message,&result,&accept);
        codec_.send(get_pointer(connections_[accept]), result);
    }
    typedef std::map<std::string,TcpConnectionPtr> ConnectionMap;
    LengthHeaderCodec codec_;
    TcpServer server_;
    ConnectionMap connections_;
};

MYSQL* mysql=NULL;
int main(int argc, char* argv[])
{
    mysql = MysqlInit();
    signal(SIGINT, [](int) { MysqlClose(mysql); exit(0);});
    DishTable dishTable(mysql);
    OrderTable orderTable(mysql);
    LOG_INFO << "pid = " << getpid();
    if (argc > 1)
    {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        OrderServer server(&loop, serverAddr,&dishTable,&orderTable);
        server.start();
        loop.loop();
    }
    else
    {
        printf("Usage: %s port\n", argv[0]);
    }
}