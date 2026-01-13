SimpleMail
=============================================

The SimpleMail is small library writen for Qt 5 or 6 (C++14) that allows applications to send complex emails (plain text, html, attachments, inline files, etc.) using the Simple Mail Transfer Protocol (SMTP).

## Features:

- Asyncronous operation
- SMTP pipelining
- TCP and SSL connections to SMTP servers (STARTTLS included)
- SMTP authentication (PLAIN, LOGIN, CRAM-MD5 methods)
- sending MIME emails (to multiple recipients)
- plain text and HTML (with inline files) content in emails
- nested mime emails (mixed/alternative, mixed/related)
- multiple attachments and inline files (used in HTML)
- different character sets (ascii, utf-8, etc) and encoding methods (7bit, 8bit, base64)
- multiple types of recipients (to, cc, bcc)
- error handling (including RESET command)
- output compilant with RFC2045

## Examples

```c++
#include <QCoreApplication>
#include <SimpleMail/SimpleMail>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // First we need to create an Server object
    auto server = new SimpleMail::Server;

    // We will use the Gmail's smtp server (smtp.gmail.com, port 465, ssl)
    server->setHost("smtp.gmail.com");
    server->setPort(465);
    server->setConnectionType(SimpleMail::Server::SslConnection);

    // We need to set the username (your email address) and the password for smtp authentification.
    server->setUsername("your_email_address@gmail.com");
    server->setPassword("your_password");

    // Now we create a MimeMessage object. This will be the email.
    SimpleMail::MimeMessage message;
    message.setSender(SimpleMail::EmailAddress("your_email_address@gmail.com", "Your Name"));
    message.addTo(SimpleMail::EmailAddress("Recipient's Name <recipient@host.com>"));
    message.setSubject("Testing Subject");

    // First we create a MimeText object.
    auto text = std::make_shared<MimeText>();

    // Now add some text to the email.
    text->setText("Hi,\nThis is a simple email message.\n");

    // Now add it to the mail
    message.addPart(text);

    // Now we can send the mail
    SimpleMail::ServerReply *reply = server->sendMail(message);
    QObject::connect(reply, &SimpleMail::ServerReply::finished, [reply] {
        qDebug() << "ServerReply finished" << reply->error() << reply->responseText();
        reply->deleteLater();// Don't forget to delete it

        qApp->quit();
    });

    app.exec();
}
```

## Reason of Email rejected

间歇性的 "Mail rejected" 错误通常是由以下原因造成的：

常见原因
频率限制

139邮箱对发送频率有限制

短时间内多次发送会被临时拒绝

建议发送间隔至少30秒-1分钟

服务器负载

SMTP服务器繁忙时会拒绝部分请求

139邮箱服务器可能有负载均衡策略

IP限制

同一IP短时间内发送过多邮件

可能触发反垃圾邮件机制

连接复用问题

频繁创建/销毁SMTP连接

服务器可能限制连接数

解决方案
添加重试机制：

// 在连接回调中添加重试逻辑
QObject::connect(reply, &ServerReply::finished, [=] {
    if (reply->error()) {
        qDebug() << "发送失败，3秒后重试...";
        QTimer::singleShot(3000, [=]() {
            // 重新发送
        });
    }
});

Copy
增加发送间隔：

测试时在每次发送间加入延时

避免连续快速发送

检查网络状况：

网络不稳定也可能导致间歇性失败

更换邮箱服务商：

考虑使用Gmail、QQ邮箱等更稳定的服务

建议先测试发送间隔，如果问题持续存在，可能需要联系139邮箱客服确认账户状态。



## License

This project (all files including the demos/examples) is licensed under the GNU LGPL, version 2.1+.
