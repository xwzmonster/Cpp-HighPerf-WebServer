#pragma once
#include<unistd.h>
#include<sys/epoll.h>
#include<memory>
#include<vector>
#include<functional>

class Epoll;
class Channel;

class Eventloop { // 负责事件循环和Epoll, Eventloop拥有Epoll
private:  
    std::unique_ptr<Epoll> ep_;
    /*
        定义一种函数类型: using 别名 = 原类型
        比typedef好, using 支持模板别名, typedef 做不到, 例如
                1、
                    // using 可以直接定义模板别名
                    template<typename T>
                    using ConnCallback = std::function<void(T)>;
                    // 使用
                    using IntConnCb = ConnCallback<int>;
                    using StringConnCb = ConnCallback<std::string>;
                2、
                    typedef 无法单独定义模板别名, 只能包一层 struct 绕弯, 代码臃肿难用：
                    // typedef 不支持模板别名, 只能套类/struct 曲线救国
                    template<typename T>
                    struct ConnCallbackWrap {
                        typedef std::function<void(T)> type;
                    };
                    // 使用还要多打一层 ::type
                    using IntConnCb = ConnCallbackWrap<int>::type;

    */
    using RemoveConnectionCallback = std::function<void(int)>;
    RemoveConnectionCallback RemoveConnectionCallback_;

public:
    Eventloop();
    Eventloop(const Eventloop&) = delete;
    Eventloop& operator=(const Eventloop&) = delete;
    
    // 运行epoll_wait(), 等待事件发生, 已发生的事件用vector容器返回
    std::vector<Channel*> poll(int timeout); 
    
    // 把channel添加/更新到红黑树上, channel中有fd, 也有需要监视的事件
    bool updateChannel(Channel* chn);
    
    // 删除Channel
    bool removeChannel(Channel* chn);

    void loop();
    void setRemoveConnectionCallback(const RemoveConnectionCallback& cb);

    ~Eventloop();
};