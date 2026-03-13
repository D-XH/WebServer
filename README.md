# WebServer
用c++实现的高性能Web服务器
## 功能
* 利用IO复用技术Epoll与线程池实现多线程的Reactor高并发模型；
* 利用正则匹配和状态机解析Http请求报文，实现处理静态资源的请求；
* 利用Vector封装实现动态增长的缓冲区；
* 利用小根堆实现定时器，负责处理定时任务和关闭超时非活动连接；
* 采用单例模式，使用阻塞队列实现异步日志系统，记录服务器信息；
* 利用RAII机制实现了数据库连接池，减少数据库频繁建立和释放连接的开销；
* (new)使用ini配置文件；
## 环境
* linux
* c++17
* mysql
## 目录树
```markdown
.
├── CMakeLists.txt
├── code
│   ├── CMakeLists.txt
│   ├── main.cc
│   ├── buffer
│   │   ├── buffer.cpp
│   │   └── buffer.h
│   ├── epoller
│   │   ├── epoller.cpp
│   │   └── epoller.h
│   ├── http
│   │   ├── httpConn.cpp
│   │   ├── httpConn.h
│   │   ├── httpRequest.cpp
│   │   ├── httpRequest.h
│   │   ├── httpResponse.cpp
│   │   └── httpResponse.h
│   ├── log
│   │   ├── blockQueue.h
│   │   ├── log.cpp
│   │   └── log.h
│   ├── pool
│   │   ├── sqlConnPool.cpp
│   │   ├── sqlConnPool.h
│   │   ├── sqlConnRAII.h
│   │   ├── threadPool.cpp
│   │   └── threadPool.h
│   ├── server
│   │   ├── webServer.cpp
│   │   └── webServer.h
│   └── timer
│       ├── heapTimer.cpp
│       └── heapTimer.h
├── 3rdparty
│   └── simpleini
├── resources
│   ...
|
├── test
│   ├── test.cc
│   └── testPool.cc
├── README.md
```

## 项目启动
### 数据库
```markdown
+-----------+-------------------------------------------------------------------------------------------------------------------------------------+
| WebServer | CREATE DATABASE `WebServer` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci */ /*!80016 DEFAULT ENCRYPTION='N' */ |
+-----------+-------------------------------------------------------------------------------------------------------------------------------------+

+----------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
| tbl_User | CREATE TABLE `tbl_User` (
  `UserName` char(255) DEFAULT NULL,
  `PassWord` char(255) DEFAULT NULL,
  `del` int DEFAULT '0'
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci |
+----------+------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
```

### 编译启动流程
1. 创建build文件夹
```shell
mkdir build && cd build
```
2. 修改配置文件`code/config.ini`

3. 编译
```shell
make
```

4. 启动
```shell
./code/webserver
```
## 致谢
Linux高性能服务器编程，游双著.
[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)
[@markparticle](https://github.com/markparticle/WebServer)