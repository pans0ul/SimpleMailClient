#include "../../src/SimpleMail"

#include <QtCore>
#include "../account_info.h"
#include <fstream>  

using namespace SimpleMail;
using namespace std;

int wokerSendMail()
{ 
    qDebug() << "Starting email sending...";
    Server server;
    server.setHost(QLatin1String(SMTP_SERVER));
    server.setPort(SMTP_SERVER_PORT);
    server.setConnectionType(Server::SslConnection);

    server.setUsername(QLatin1String(SENDER_EMAIL));
    server.setPassword(QLatin1String(SENDER_PASSWORD));

    // Now we create a MimeMessage object. This is the email.

    MimeMessage message;

    EmailAddress sender(QLatin1String(SENDER_EMAIL), QLatin1String(SENDER_NAME));
    message.setSender(sender);

    EmailAddress to(QLatin1String(RECIPIENT_EMAIL), QLatin1String("Recipient's Name"));
    message.addTo(to);

    message.setSubject(QLatin1String("SmtpClient for Qt - Demo"));

    // Now add some text to the email.
    // First we create a MimeText object.

    auto text = std::make_shared<MimeText>();

    text->setText(QLatin1String("Hi,\nThis is a simple email message.\n"));

    // Now add it to the mail

    message.addPart(text);

    // Now we can send the mail
    ServerReply *reply = server.sendMail(message);

    int exitCode = 0; // 退出码，发送失败时设为非零
    QEventLoop loop;
    QObject::connect(reply, &ServerReply::finished, [reply, &loop, &exitCode] {
        qDebug() << "ServerReply finished" << reply->error() << reply->responseText();

        // 记录退出码并清理
        exitCode = reply->error() ? -3 : 0;
        reply->deleteLater();

        // 退出本地事件循环
        loop.quit();
    });

    qDebug() << "Entering local event loop...";
    loop.exec(); // 等待发送完成
    qDebug() << "Local event loop exited, exitCode=" << exitCode;

    return exitCode;

}



int main(int argc, char *argv[])
{

    QCoreApplication app(argc, argv);

    wokerSendMail();

    return  0;

}
