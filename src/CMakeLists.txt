set(SRC_LIST
    MprpcApplication.cc
    MprpcProvider.cc
    rpc_header.pb.cc
    MprpcChannel.cc
    MprpcController.cc
    ZookeeperUtil.cc
    )

# 这个命令用于自动查找mprpc目录中的所有源文件，并将它们添加到SRC_LIST变量中。
# 但是，这种方法并不推荐，因为如果新增或删除文件，可能会导致意外的行为。通常最好是在SRC_LIST中明确列出所有源文件
aux_source_directory(mprpc SRC_LIST)

# 生成静态库
add_library(mprpc ${SRC_LIST})
# 目标需要链接的库
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)