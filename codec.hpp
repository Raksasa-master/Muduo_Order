//
// Created by rkasasa on 23-3-8.
//

#ifndef MUDUO_ORDER_CODEC_HPP
#define MUDUO_ORDER_CODEC_HPP

#include <mysql/mysql.h>
#include "Sql.pb.h"
#include "muduo/base/Logging.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/Endian.h"
#include "muduo/net/TcpConnection.h"
#include "Sql.hpp"
using namespace Order_Sql;
class LengthHeaderCodec : muduo::noncopyable
{
public:
    typedef std::function<void (const muduo::net::TcpConnectionPtr&,
                                const muduo::string& message,
                                muduo::Timestamp)> StringMessageCallback;

    explicit LengthHeaderCodec(const StringMessageCallback& cb,DishTable* dishTable,OrderTable* orderTable)
            : messageCallback_(cb),dishTable_(dishTable),orderTable_(orderTable)
    {
    }

    void onMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buf,
                   muduo::Timestamp receiveTime)
    {
        while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
        {
            // FIXME: use Buffer::peekInt32()
            const void* data = buf->peek();
            int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
            const int32_t len = muduo::net::sockets::networkToHost32(be32);
            if (len > 65536 || len < 0)
            {
                LOG_ERROR << "Invalid length " << len;
                conn->shutdown();  // FIXME: disable reading
                break;
            }
            else if (buf->readableBytes() >= len + kHeaderLen)
            {
                buf->retrieve(kHeaderLen);
                muduo::string message(buf->peek(), len);
                messageCallback_(conn, message, receiveTime);
                buf->retrieve(len);
            }
            else
            {
                break;
            }
        }
    }

    // FIXME: TcpConnectionPtr

    void send(muduo::net::TcpConnection* conn,
              const muduo::StringPiece& message)
    {
        muduo::net::Buffer buf;
        buf.append(message.data(), message.size());
        int32_t len = static_cast<int32_t>(message.size());
        int32_t be32 = muduo::net::sockets::hostToNetwork32(len);
        buf.prepend(&be32, sizeof be32);
        conn->send(&buf);
    }


    bool Task_Handle(const std::string& message,std::string* result,std::string* accept)
    {
        T task;
        task.ParseFromString(message);
        {
            muduo::MutexLockGuard lock(mutex_);
            switch (task.op())
            {
                case T::News:break;
                case T::Dish_Insert: dishTable_->Insert(task);break;
                case T::Dish_Delete: dishTable_->Delete(task);break;
                case T::Dish_Update: dishTable_->Update(task);break;
                case T::Dish_QueryOne: dishTable_->SelectOne(task);break;
                case T::Dish_QueryAll: dishTable_->SelectAll(task);break;
                case T::Order_Insert: orderTable_->Insert(task);break;
                case T::Order_Update: orderTable_->ChangeState(task);break;
                case T::Order_QueryAll: orderTable_->SelectAll(task);break;
                default:
                {
                    LOG_ERROR<<"客户端发送数据非法！";
                    return false;
                }
            }
        }
        *accept=task.accept();
        *result=task.SerializeAsString();
        return true;
    }
private:
    muduo::MutexLock mutex_;
    StringMessageCallback messageCallback_;
    OrderTable* orderTable_;
    DishTable* dishTable_;

    const static size_t kHeaderLen = sizeof(int32_t);
};

#endif //MUDUO_ORDER_CODEC_HPP
