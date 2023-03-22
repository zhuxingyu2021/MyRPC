# MyRPC
**一个轻量级的RPC框架**

## 构建

**框架构建方法：**
```shell
git clone https://github.com/zhuxingyu2021/MyRPC
cd MyRPC
git submodule update --init
mkdir build
cd build
cmake ..
make
```
## 架构
![pic](https://i.328888.xyz/2023/02/02/I6quQ.png)

### 目录结构
* fiber -- 基于线程-协程模型的线程池实现，由线程池核心模块、协程间同步原语模块、针对系统调用的hook模块三部分组成。
* net -- 该目录提供了网络库、序列化、反序列化的核心代码。网络库在线程池的基础上，提供了TCP Socket的封装。
* rpc -- 该目录提供了RPC核心代码的实现，包括RPC通信协议、客户端、服务端、注册中心的代码实现。

## 快速开始
### 1. 编写注册中心
```c++
    Config::ptr config = make_shared<Config>(); 

    RpcRegistryServer server(config);
    if(!server.bind()){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();
    server.Loop();
```
**通过修改配置文件来修改注册中心的IP地址或端口：**

配置文件的这一部分决定了注册中心的IP地址或端口：
```json
"RegistryServerAddr":{
    "IP":"127.0.0.1",
    "Port":9000,
    "IsIPv6":false
}
```
修改配置文件之后，我们可以通过Config类的LoadFromJson方法加载json配置文件：
```c++
    Config::ptr config(Config::LoadFromJson("test.json")); 

    RpcRegistryServer server(config);
    if(!server.bind()){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();
    server.Loop();
```

### 2. 编写服务端
```c++
    RPCServer server(std::make_shared<InetAddr>("127.0.0.1", 5678), config); // 服务端IP地址：127.0.0.1，端口：5678

    if(!server.bind()){
        Logger::error("bind error: {}", strerror(errno));
        return -1;
    }
    server.Start();

    server.ConnectToRegistryServer();
    
    server.Loop();
```

### 3. 编写服务端的服务接口
**Lambda表达式**
```c++
    server.ConnectToRegistryServer();

    // 向注册中心注册服务，服务名：echo
    server.RegisterMethod("echo", [](const std::string& in)->std::string{
        return in;
    });

    server.Loop();
```
**回调函数**
```c++
    // 函数声明&实现
    std::string func_echo(const std::string& in){
        return in;
    }   

    int main(int argc, char** argv){
        ......
    
        server.ConnectToRegistryServer();
        
        // 向注册中心注册服务，服务名：echo
        server.RegisterMethod("echo", func_echo);
    
        server.Loop();
        
        ......
    }
```

### 4. 编写客户端
**初始化客户端**
```c++
    RPCClient client(config);

    // 连接注册中心
    if(!client.ConnectToRegistryServer()){
        Logger::error("Can't connect to registry server!");
        exit(-1);
    }
    client.Start();
```

**调用函数**

本框架支持通过c++的future/promise方式异步调用远程函数：
```c++
    std::promise<std::string> promise_echo;
    // 调用服务名为echo的远程函数
    auto future_echo = client.InvokeAsync(promise_echo, "echo", std::string("Hello world!"));

    // 异步调用，你可以在这里做任何你想做的事
    
    std::cout << future_echo.get() << std::endl;
```

### 5.自定义数据类型
本框架支持几乎所有STL类型，并且支持类型嵌套，如`std::map<std::vector<int>>`。以下是本框架支持的STL类型：
* string
* array, vector, deque, list, forward_list, tuple
* set, unordered_set
* map, unordered_map, multimap, unordered_multimap
* pair
* optional
* shared_ptr, unique_ptr

本框架也支持用户自定义的结构体类型，只不过需要在结构体中添加额外的代码，如下所示：
```c++
    struct Person{
        std::string _id;
        int age;
        std::string picture;
        std::string name;
        std::string gender;
        std::string company;
        std::string email;
        std::string phone;
        std::string address;
        std::string about;

        std::vector<std::string> tags;
        std::map<int,std::string> friends;

        SAVE_BEGIN
            SAVE_ITEM(_id)
            SAVE_ITEM(age)
            SAVE_ITEM(picture)
            SAVE_ITEM(name)
            SAVE_ITEM(gender)
            SAVE_ITEM(company)
            SAVE_ITEM(email)
            SAVE_ITEM(phone)
            SAVE_ITEM(address)
            SAVE_ITEM(about)
            SAVE_ITEM(tags)
            SAVE_ITEM(friends)
        SAVE_END

        LOAD_BEGIN
            LOAD_ITEM(_id)
            LOAD_ITEM(age)
            LOAD_ITEM(picture)
            LOAD_ITEM(name)
            LOAD_ITEM(gender)
            LOAD_ITEM(company)
            LOAD_ITEM(email)
            LOAD_ITEM(phone)
            LOAD_ITEM(address)
            LOAD_ITEM(about)
            LOAD_ITEM(tags)
            LOAD_ITEM(friends)
        LOAD_END

    };
```

之后可以基于自定义类型实现RPC服务：

服务端：
```c++
    server.ConnectToRegistryServer();
    
    // 向注册中心注册服务，服务名：get_phone_number
    server.RegisterMethod("get_phone_number", [](const Person& p)->std::string{
        return p.phone;
    });

    server.Loop();
```
客户端：
```c++
    std::promise<std::string> promise_get_phone_number;
    // 调用服务名为get_phone_number的远程函数
    auto future_get_phone_number = client.InvokeAsync(promise_get_phone_number, "get_phone_number", person);事
    
    std::cout << future_get_phone_number.get() << std::endl;
```

## 反馈与参与
* Bug、建议都欢迎提在[Issues](https://github.com/zhuxingyu2021/MyRPC/issues)或者发送邮件至wode057406422181@gmail.com
