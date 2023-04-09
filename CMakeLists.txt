cmake_minimum_required(VERSION 3.24)
#project(Muduo_Order)
project(Server)
project(Client)
set(CMAKE_CXX_STANDARD 11)
link_libraries(muduo_net muduo_base mysqlclient)
link_libraries(protobuf)

add_executable(Server Server.cpp Sql.hpp codec.hpp  Sql.pb.cc)
add_executable(Client  Sql.hpp codec.hpp client.cpp Sql.pb.cc)
#target_link_libraries(Muduo_Order pthread)
target_link_libraries(Server pthread)
target_link_libraries(Client pthread)

