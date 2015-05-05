
* 完善客户端管理
* 命令那边改成Object方式
* 所有lua_check*的方法修改
* 整改cmakelists.txt
* ‘execvp’ 相关环境变量的问题 .bashrc
* lua 库 环境变量问题server.unixtime ?
* 超时机制 
    lua_sethook
* 收集coredump
    

* luaworkCallByRef 返回结果和命令 exec调用结果格式一致，都采用redis结构
* 增加配置文件中配置 cpath, path 的配置项
* 将 luasocket， sqlite 整理为自编译动态库 