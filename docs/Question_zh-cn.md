#QA
### 开发过程中的问题记录
1. accept() Bad file descriptor
  发现自己打开了另外一个saker

1. accept() Resource temporarily unavailable
  在打开关闭几次客户端后报此错，但并不影响使用
  解决方法:改为直接移植redis的事件驱动代码

1. 在增加订阅发布模式后，出现了一个新的问题，当一个客户端订阅了某channel
  而此时客户端又有了操作，此时写的内容乱套了。 乱套的原因在于 协议的定制
  解决方法:每次只返回一个协议内容

1. 使用redis-cli订阅时，偶发下面的错误。是服务端发送还是客户端接受乱序？
3) "[2013-09-23 21:33:57] [INFO*3\r\n$7\r\nmessage\r\n$3\r\nlog\r\n$97\r\n[2013-09-23 21:33:57] [INFO] [../."
Error: Protocol error, got "s" as reply type byte
在做发布-订阅时，出现了一个问题，即在lua脚本里，大量的 publish了，导致客户端发布的内容乱套，然后我在 sendReplyToClient 这个event事件里，当发现不是一个同一个消息的内容时，就直接退出 写事件，直到下次发生写事件时再发送， 此时，另一个问题出现了，由于 publish的数据量太大，导致发送不及时，越积累越多（比如 一次 publish 10条内容， 而我只 send 一条， 第二次 又 publish 10条， 此时，我又只send一条，客户端此时有了18条剩余，如此下去，最后把内存吃完。）
[解决方法]:定性要求publish时针对每个channel每次只发送一个， 在networking发送处理时不再判断是否是一条协议，而是直接全部发送。

1. 在普通用户下，测试 exec的接口时，发现权限不够，不能访问/var/run/的文件，此时直接子进程exit，会产生 defunct 僵死进程，原因是因为子进程退出，而父进程未退出，后来用了一比较恶心的方法解决，先sleep，然后判断进程pidfile是否还存在，不存在了waitpid，此时僵死进程会从系统中退出。
解决方法:waitpid一下
```
	xsleep(100);
	if (pidfile_exists(pidfile) == UGOK) { // Process is still running
    	lua_pushnumber(L, pid);
		lua_pushstring(L, pidfile);
	}
	else { 
		waitpid(pid, &status, 0); // Wait blocking
	    lua_pushnil(L);
        lua_pushfstring(L, "child exit status code '%d' for '%s' ,please cat log by 'mypid is '%d''", status, pidfile, pid);
	}
```

1. 在saker基本成型时，发现运行中，RES不断增加，用valgrind memcheck 查不出内存泄露，这是一个非常棘手的问题。试图用以下方法解决这个问题：
打点记录下 saker的 smaps 信息，发现确实物理内存在不断增加，在排查代码中仍无结果。 最后，认为是lua执行造成的泄露，在调用lua_gc(server.ls, LUA_GCCOUNT, 0)*1024 + lua_gc(server.ls, LUA_GCCOUNTB, 0) 对 pcall前后打点发现 ，确定有内存不断增加，不过并不是每一次都增加，而是调几次后增加，很令人费解，尝试了用以下方法解决：
(1)：首先认为是lua脚本里强引用导致无法被回收，即没有有local变量，改为只带一个print的function来频繁调用，发现仍然如此。
(2)：认为是pcall的function强引用导致，所以改为luaL_ref  lua_rawgeti luaL_unref的方式尝试，仍未解决问题
(3)： 尝试在pcall后
```
while(lua_gettop(server.ls)) {
				lua_pop(server.ls ,1);
			}
```
销毁所有返回值，问题得以解决，不知其所以然。
后来方法改为
```
lua_settop(server.ls, ntop)
``` 
---补充	
lua_pcall后返回的堆栈需要手动调用lua_pop释放,lua_settop是同样原理

1. 在windows下，EnumProcessModules error :仅完成部分的 ReadProcessMemoty 或 Wr。iteProcessMemory 请求。

1. 在python客户端调用时会出现断开后连接不上的情况。

1. 针对代码中使用 lua_check* 的相关整改

1. 怎么使用第三方库（*.so) ？ 首先要是lua5.2库编译的动态库，然后将其放在 lua的 cpath所在路径下，然后直接 
库放在执行文件当前目录下, 相应的 动态库也要放到当前目录
require"base64"
print(base64.version)

### 挂载进程相关（linux）
1. 精灵进程，自写pidfile
  ---守护方法 查看pidfile，根据pid查看其状态，正则匹配进程名

2. 非精灵进程，saker fork出来
  ---根据pid文件判断(带文件锁)，正则匹配其进程名
  
3. 精灵进程，无pidfile
  ---太变态，自己判断

