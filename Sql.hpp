//
// Created by rkasasa on 23-3-7.
//

#ifndef MUDUO_ORDER_SQL_HHP
#define MUDUO_ORDER_SQL_HHP

#include <stdio.h>
#include <mysql/mysql.h>
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "Sql.pb.h"
typedef SQL::Dish D;
typedef SQL::Task T;
typedef SQL::Order O;
namespace Order_Sql
{
    static MYSQL* MysqlInit()
    {
        MYSQL* connect_fd = mysql_init(NULL);
        if(mysql_real_connect(connect_fd,"127.0.0.1",
                              "root","142724","order_system",3306,NULL,0)==NULL)
        {
            printf("连接失败！ %s\n",mysql_error(connect_fd));
            return NULL;
        }
        mysql_set_character_set(connect_fd,"utf8");
        return connect_fd;
    }
    static void MysqlClose(MYSQL* mysql)
    {
        mysql_close(mysql);
    }
    class DishTable
    {
    public:
        DishTable(MYSQL* mysql):mysql_(mysql){}
        bool Insert(T &task)
        {
            char sql[1024] = {0};
            try
            {
                for(auto &dish:task.dish_vector())
                sprintf(sql, "insert into dish_table values(null, '%s', %d,%d)",
                        dish.name().c_str(),dish.price(),dish.number());
            }
            catch (std::exception &e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int ret = mysql_query(mysql_,sql);
            if(ret != 0)
            {
                LOG_ERROR<<"dish insert error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("dish insert error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            task.set_news("dish insert finish!");
            task.set_op(T::Task_List::Task_Task_List_News);
            return true;
        }
        bool Delete(T &task)
        {
            char sql[1024 * 4] = {0};
            try
            {
                for(auto &dish:task.dish_vector())
                sprintf(sql, "delete from dish_table where dish_id=%d", dish.dish_id());
            }
            catch (std::exception &e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"dish delete error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("dish delete error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            task.set_news("dish delete finish!");
            task.set_op(T::Task_List::Task_Task_List_News);
            return true;
        }
        bool Update(T &task)
        {
            char sql[1024 * 4] = {0};
            try
            {
                for(auto &dish:task.dish_vector())
                sprintf(sql, "update dish_table SET name='%s', price=%d , number=%d where dish_id=%d",
                        dish.name().c_str(),dish.price(),dish.number(), dish.dish_id());
            }
            catch (std::exception &e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"dish update error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("dish update error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            task.set_news("dish update finish!");
            task.set_op(T::Task_List::Task_Task_List_News);
            return true;
        }

        bool SelectOne(T &task)
        {
            char sql[1024 * 4] = {0};
            try
            {
                for(auto &dish:task.dish_vector())
                sprintf(sql, "select * from dish_table where dish_id = %d", dish.dish_id());
                T* taskPtr;
                taskPtr=&task;
                taskPtr->mutable_dish_vector()->DeleteSubrange(0,1);
            }
            catch (std::exception &e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"dish select error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("dish select error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            MYSQL_RES* result = mysql_store_result(mysql_);
            MYSQL_ROW row = mysql_fetch_row(result);
            D* DishPtr;
            DishPtr = task.add_dish_vector();
            DishPtr->set_dish_id(atoi(row[0]));
            DishPtr->set_name(row[1]);
            DishPtr->set_price(atoi(row[2]));
            DishPtr->set_number(atoi(row[3]));
            return true;
        }
        bool SelectAll(T &task)
        {
            char sql[1024 * 4] = {0};
            sprintf(sql, "select * from dish_table");
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"dish select error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("dish select error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            MYSQL_RES* result = mysql_store_result(mysql_);
            int rows= mysql_num_rows(result);
            for(int i=0;i<rows;i++)
            {
                MYSQL_ROW row = mysql_fetch_row(result);
                D* DishPtr;
                DishPtr = task.add_dish_vector();
                DishPtr->set_dish_id(atoi(row[0]));
                DishPtr->set_name(row[1]);
                DishPtr->set_price(atoi(row[2]));
                DishPtr->set_number(atoi(row[3]));
            }
            return true;
        }

    private:
        MYSQL* mysql_;
    };

    class OrderTable
    {
    public:
        OrderTable(MYSQL* mysql):mysql_(mysql){}
        bool Insert(T &task) {
            char sql[1024 * 4] = {0};
            int ret = 0;
            ret = mysql_query(mysql_,"begin");
            try
            {
                T dish_in_task;
                for(auto &order:task.order_vector())
                {
                    dish_in_task.ParseFromString(order.dish_ids());
                    sprintf(sql, "insert into order_table values(null, '%d', '%s', '%s', '%d')",
                            order.table_id(), order.time().c_str(),
                            order.dish_ids().c_str(), order.state());
                }
                ret = mysql_query(mysql_, sql);
                if (ret != 0) {
                    LOG_ERROR<<"order insert error!";
                    LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                    task.set_news("order insert error!");
                    task.set_op(T::Task_List::Task_Task_List_News);
                    ret = mysql_query(mysql_,"rollback");
                    return false;
                }
                for(auto &dish:dish_in_task.dish_vector())
                {
                    sprintf(sql, "update dish_table set number= now - %d where dish_id=%d",
                            dish.number(), dish.dish_id());
                    ret = mysql_query(mysql_,sql);
                    if (ret != 0) {
                        LOG_ERROR<<"order insert error!";
                        LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                        task.set_news("order insert error!");
                        task.set_op(T::Task_List::Task_Task_List_News);
                        ret = mysql_query(mysql_,"rollback");
                        return false;
                    }
                }
            }
            catch (std::exception &e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                ret = mysql_query(mysql_,"rollback");
                return false;
            }
            ret = mysql_query(mysql_,"commit");
            task.set_news("order insert finish!");
            task.set_op(T::Task_List::Task_Task_List_News);
            return true;
        }

        bool ChangeState(T &task) {
            char sql[1024 * 4] = {0};

            try
            {
                for(auto &order:task.order_vector())
                sprintf(sql, "update order_table set state = %d where order_id = %d",
                        order.state(), order.order_id());
            }
            catch (std::exception e)
            {
                LOG_ERROR<<"数据不符合规格！";
                task.set_news("数据不符合规格！");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"order ChangeState error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("order ChangeState error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            task.set_news("order ChangeState finish!");
            task.set_op(T::Task_List::Task_Task_List_News);
            return true;
        }

        bool SelectAll(T &task) {
            char sql[1024 * 4] = {0};
            sprintf(sql, "select * from order_table");

            int ret = mysql_query(mysql_, sql);
            if (ret != 0) {
                LOG_ERROR<<"dish select error!";
                LOG_ERROR<<"sql = "<<sql<<" error = "<< mysql_error(mysql_);
                task.set_news("order select error!");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            MYSQL_RES* result = mysql_store_result(mysql_);
            if (result == NULL) {
                LOG_ERROR<<"获取结果失败! "<<mysql_error(mysql_);
                task.set_news("获取结果失败! ");
                task.set_op(T::Task_List::Task_Task_List_News);
                return false;
            }
            int rows= mysql_num_rows(result);
            for(int i=0;i<rows;i++)
            {
                MYSQL_ROW row = mysql_fetch_row(result);
                O* OrderPtr;
                OrderPtr = task.add_order_vector();
                OrderPtr->set_order_id(atoi(row[0]));
                OrderPtr->set_table_id(atoi(row[1]));
                OrderPtr->set_time(row[2]);
                OrderPtr->set_dish_ids(row[3]);
                OrderPtr->set_state(atoi(row[4]));
            }
            return true;
        }

    private:
        MYSQL* mysql_;
    };


}

#endif //MUDUO_ORDER_SQL_HHP
