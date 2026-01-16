SimpleMailClient
=============================================

中文 | [English](README.md)

SimpleMailClient借用[SimpleMail](https://github.com/cutelyst/simple-mail) library开发的邮件客户端。


## Features

- 支持用户读取指定文本文件发送邮件
- 支持用户设定发送间隔和设定发送的条目数量
- 程序展示发送内容的计划表。用户可以看到未来要发送的计划任务
- 支持用户断点续发
- 支持邮件账户密码加密
- 支持静态编译
- 提供Cli版本和GUI版本


## Quickstart

1. Build

    `mkdir build && cd build && cmake ..`

    `cmake --build .` 


    **static-build**
    `mkdir build && cd build && cmake ..`

    `cmake .. -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_EXECUTABLE=ON`

    `cmake --build .`

    注意：静态编译需要静态的Qt libs
    
2. Config

    支持三种方式

    - 配置文件: --config mail_config.ini (格式参考docs/mail_config.ini)
    - 命令行参数: --smtp-server, --sender-email, --sender-password, --recipient-email
    - 头文件: ../account_info.h (编译时配置，配置在inc_default.h)

3. Run 
    `Usage: ./SimpleMailClientCli [options] content interval count lines`

    例如：`./SimpleMailClientCli ./content.txt 3s 3 3`
    表示: 读取content.txt 文件内容，并且间隔3s时间发送3份邮件，每次读取文件三行内容


## 常见问题

### 间歇性的 "Mail rejected" 错误通常是由以下原因造成的：

**邮箱服务器对发送频率有限制**

    短时间内多次发送会被临时拒绝

    建议发送间隔至少30秒-1分钟

**服务器负载**

    SMTP服务器繁忙时会拒绝部分请求

    邮箱服务器可能有负载均衡策略

**IP限制**

    同一IP短时间内发送过多邮件

    可能触发反垃圾邮件机制

**连接复用问题**

    频繁创建/销毁SMTP连接

    服务器可能限制连接数


**一些解决办法**

    增加发送间隔：

        测试时在每次发送间加入延时

        避免连续快速发送

    检查网络状况：

        网络不稳定也可能导致间歇性失败

    更换邮箱服务商：

        考虑使用Gmail、QQ邮箱等更稳定的服务


## 欢迎Contribution


## License

This project (all files including the demos/examples) is licensed under the GNU LGPL, version 2.1+.
