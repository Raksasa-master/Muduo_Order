//
// Created by rkasasa on 23-3-11.
//

#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/TcpClient.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include "codec.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;

class OrderClient : noncopyable
{
public:
    OrderClient(EventLoop* loop, const InetAddress& serverAddr,DishTable* dishTable,OrderTable* orderTable)
            : client_(loop, serverAddr, "OrderClient"),
              codec_(std::bind(&OrderClient::onStringMessage, this, _1, _2, _3),dishTable,orderTable)
    {
        client_.setConnectionCallback(
                std::bind(&OrderClient::onConnection, this, _1));
        client_.setMessageCallback(
                std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
        client_.enableRetry();
    }

    void connect()
    {
        client_.connect();
    }

    void disconnect()
    {
        client_.disconnect();
    }

    string get_Address()
    {
        return connection_->localAddress().toIpPort();
    }

    void write(const StringPiece& message)
    {
        MutexLockGuard lock(mutex_);
        if (connection_)
        {
            codec_.send(get_pointer(connection_), message);
        }
    }

private:
    void onConnection(const TcpConnectionPtr& conn)
    {
        LOG_INFO << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        MutexLockGuard lock(mutex_);
        if (conn->connected())
        {
            connection_ = conn;
        }
        else
        {
            connection_.reset();
        }
    }

    void onStringMessage(const TcpConnectionPtr&,
                         const string& message,
                         Timestamp)
    {
        T task;
        task.ParseFromString(message);
        switch (task.op())
        {
            case task.News:
            {
                printf("<<< %s\n", task.news().c_str());
                break;
            }
            case task.Dish_QueryOne:
            {
                for(auto &dish:task.dish_vector())
                {
                    printf("菜号：%d   菜名：%s   菜价：%d   数量：%d\n",dish.dish_id(),dish.name().c_str(),dish.price(),dish.number());
                }
                break;
            }
            case task.Dish_QueryAll:
            {
                for(auto &dish:task.dish_vector())
                {
                    printf("菜号：%d   菜名：%s   菜价：%d   数量：%d\n",dish.dish_id(),dish.name().c_str(),dish.price(),dish.number());
                }
                break;
            }
            case task.Order_QueryAll:
            {
                for(auto &order:task.order_vector())
                {
                    printf("单号：%d   桌号：%d   时间：%s    订单状态：%d\n",order.order_id(),order.table_id(),order.time().c_str(),order.state());
                    printf("菜品清单：\n");
                    T dish_in_task;
                    dish_in_task.ParseFromString(order.dish_ids());
                    for(auto &dish:dish_in_task.dish_vector())
                    {
                        printf("菜号：%d   数量：%d\n",dish.dish_id(),dish.number());
                    }
                }
                break;
            }
            default:LOG_ERROR<<"客户端发送数据非法！";
        }
    }

    TcpClient client_;
    LengthHeaderCodec codec_;
    MutexLock mutex_;
    TcpConnectionPtr connection_ GUARDED_BY(mutex_);
};
void menu()
{
    cout<<"1.News"<<endl;
    cout<<"2.Dish_Insert"<<endl;
    cout<<"3.Dish_Delete"<<endl;
    cout<<"4.Dish_Update"<<endl;
    cout<<"5.Dish_QueryOne"<<endl;
    cout<<"6.Dish_QueryAll"<<endl;
    cout<<"7.Order_Insert"<<endl;
    cout<<"8.Order_Update"<<endl;
    cout<<"9.Order_QueryAll"<<endl;
}
MYSQL* mysql_client=NULL;
int main(int argc, char* argv[]) {
    mysql_client = MysqlInit();
    signal(SIGINT, [](int) { MysqlClose(mysql_client); exit(0);});
        DishTable dishTable(mysql_client);
        OrderTable orderTable(mysql_client);
    LOG_INFO << "pid = " << getpid();
    if (argc > 2)
    {
        EventLoopThread loopThread;
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress serverAddr(argv[1], port);

        OrderClient client(loopThread.startLoop(), serverAddr,&dishTable,&orderTable);
        client.connect();
        std::string line;
        while (true)
        {
            menu();
            int op;
            cin>>op;
            T task;
            switch (op)
            {
                case 1:
                {
                    printf("请输入信息！\n");
                    string str;
                    cin>>str;
                    task.set_op(T::Task_List::Task_Task_List_News);
                    task.set_news(str);
                    break;
                }
                case 2:
                {
                    printf("请输入要插入的菜品信息！(菜名，价格，数量)\n");
                    string name;
                    int price,number;
                    cin>>name>>price>>number;
                    task.set_op(T::Task_List::Task_Task_List_Dish_Insert);
                    D* dish;
                    dish = task.add_dish_vector();
                    dish->set_name(name);
                    dish->set_price(price);
                    dish->set_number(number);
                    break;
                }
                case 3:
                {
                    printf("请输入要删除的菜品信息！(dish_id)\n");
                    int id;
                    cin>>id;
                    task.set_op(T::Task_List::Task_Task_List_Dish_Delete);
                    D* dish;
                    dish = task.add_dish_vector();
                    dish->set_dish_id(id);
                    break;
                }
                case 4:
                {
                    printf("请输入要更新的菜品信息！(菜名，价格，数量，dish_id)\n");
                    string name;
                    int price,number,id;
                    cin>>name>>price>>number>>id;
                    task.set_op(T::Task_List::Task_Task_List_Dish_Update);
                    D* dish;
                    dish = task.add_dish_vector();
                    dish->set_name(name);
                    dish->set_price(price);
                    dish->set_number(number);
                    dish->set_dish_id(id);
                    break;
                }
                case 5:
                {
                    printf("请输入要查询的菜品信息！(dish_id)\n");
                    int id;
                    cin>>id;
                    task.set_op(T::Task_List::Task_Task_List_Dish_QueryOne);
                    D* dish;
                    dish = task.add_dish_vector();
                    dish->set_dish_id(id);
                    break;
                }
                case 6:
                {
                    printf("菜品信息！\n");
                    task.set_op(T::Task_List::Task_Task_List_Dish_QueryAll);
                    break;
                }
                case 7:
                {
                    printf("请输入要插入的点单信息！(桌号，时间，状态)\n");
                    int table_id,state;
                    string time,dish_ids;
                    cin>>table_id>>time>>state;
                    printf("菜品信息！\n");
                    task.set_op(T::Task_List::Task_Task_List_Dish_QueryAll);
                    task.set_accept(client.get_Address());
                    string str=task.SerializeAsString();
                    client.write(str);
                    printf("请输入菜品(编号，数量，以0,0结尾)\n");
                    T dish_in_task;
                    while(true)
                    {
                        int id,number;
                        cin>>id>>number;
                        if(id==0&&number==0)
                        {
                            break;
                        }
                        D* dish;
                        dish=dish_in_task.add_dish_vector();
                        dish->set_dish_id(id);
                        dish->set_number(number);
                    }
                    dish_ids=dish_in_task.SerializeAsString();
                    O* order;
                    task.set_op(T::Task_List::Task_Task_List_Order_Insert);
                    order=task.add_order_vector();
                    order->set_table_id(table_id);
                    order->set_time(time);
                    order->set_dish_ids(dish_ids);
                    order->set_state(state);
                    break;
                }
                case 8:
                {
                    printf("请输入要更新的点单信息！(状态，order_id)\n");
                    int order_id,state;
                    cin>>order_id>>state;
                    O* order;
                    task.set_op(T::Task_List::Task_Task_List_Order_Update);
                    order=task.add_order_vector();
                    order->set_order_id(order_id);
                    order->set_state(state);
                    break;
                }
                case 9:
                {
                    printf("点单信息！\n");
                    task.set_op(T::Task_List::Task_Task_List_Order_QueryAll);
                    break;
                }
                default:printf("输入有误！\n");
            }
            task.set_accept(client.get_Address());
            string str=task.SerializeAsString();
            client.write(str);
        }
    }
    else
    {
        printf("Usage: %s host_ip port\n", argv[0]);
    }
    return 0;
}
