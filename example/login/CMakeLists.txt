
# 创建了一个变量 CLIENT_LIST，包括 client.cc 和位于上级目录的 user.pb.cc
set(CLIENT_LIST client.cc ../user.pb.cc)

# 创建了一个变量 SERVER_LIST，包括 server.cc 和位于上级目录的 user.pb.cc
set(SERVER_LIST server.cc ../user.pb.cc)

# 用 CLIENT_LIST 中列出的源文件来创建一个名为 client 的可执行文件
add_executable(client ${CLIENT_LIST})

# 用 SERVER_LIST 中列出的源文件来创建一个名为 server 的可执行文件
add_executable(server ${SERVER_LIST})

#  把 mprpc库 和 protobuf库 链接到 client 可执行文件上
target_link_libraries(client mprpc protobuf)

#  把 mprpc库 和 protobuf库 链接到 server 可执行文件上
target_link_libraries(server mprpc protobuf)