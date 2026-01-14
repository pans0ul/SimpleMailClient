#include <QtCore>
#include <QTimer>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QSettings>
#include <QCommandLineParser>

#include "../../src/SimpleMail"
#include "../account_info.h"

using namespace SimpleMail;

struct MailConfig {
    QString smtpServer;
    int smtpPort = 465;
    QString senderName;
    QString senderEmail;
    QString senderPassword;
    QString recipientName;
    QString recipientEmail;
};

MailConfig mailConfig;
QStringList emailContents;
int currentIndex = 0;
int maxCount = 0;
int linesPerEmail = 1;
int retryCount = 0;
const int MAX_RETRIES = 3;
QString progressFile = "mail_progress.ini";

bool loadConfigFromFile(const QString &filename, MailConfig &config) {
    QSettings settings(filename, QSettings::IniFormat);
    if (!QFile::exists(filename)) {
        return false;
    }
    
    config.smtpServer = settings.value("smtp/server").toString();
    config.smtpPort = settings.value("smtp/port", 465).toInt();
    config.senderName = settings.value("sender/name").toString();
    config.senderEmail = settings.value("sender/email").toString();
    config.senderPassword = settings.value("sender/password").toString();
    config.recipientName = settings.value("recipient/name").toString();
    config.recipientEmail = settings.value("recipient/email").toString();
    
    return !config.smtpServer.isEmpty() && !config.senderEmail.isEmpty() && !config.recipientEmail.isEmpty();
}

QStringList readTxtFile(const QString &filename) {
    QStringList lines;
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            if (!line.trimmed().isEmpty()) {
                lines.append(line);
            }
        }
    }
    return lines;
}

qint64 parseTimeInterval(const QString &timeStr) {
    QString str = timeStr.toLower();
    qint64 value = 0;
    QString unit;
    
    for (int i = 0; i < str.length(); ++i) {
        if (str[i].isDigit()) {
            value = value * 10 + str[i].digitValue();
        } else {
            unit = str.mid(i);
            break;
        }
    }
    
    if (unit.startsWith("d")) return value * 24 * 60 * 60 * 1000; // 天
    if (unit.startsWith("h")) return value * 60 * 60 * 1000;      // 小时
    if (unit.startsWith("m")) return value * 60 * 1000;           // 分钟
    if (unit.startsWith("s")) return value * 1000;                // 秒
    return value; // 默认毫秒
}

void saveProgress() {
    QSettings settings(progressFile, QSettings::IniFormat);
    settings.setValue("currentIndex", currentIndex);
    settings.setValue("retryCount", retryCount);
}

void loadProgress() {
    QSettings settings(progressFile, QSettings::IniFormat);
    currentIndex = settings.value("currentIndex", 0).toInt();
    retryCount = settings.value("retryCount", 0).toInt();
}

QStringList getEmailContent(int index) {
    QStringList content;
    int startLine = index * linesPerEmail;
    for (int i = 0; i < linesPerEmail && (startLine + i) < emailContents.size(); ++i) {
        content.append(emailContents[startLine + i]);
    }
    return content;
}

void printSchedule(qint64 intervalMs, int count) {
    qDebug() << "=== 邮件发送计划 ===";
    qDebug() << "总邮件数量:" << count;
    qDebug() << "文件内容读取行数:" << linesPerEmail;
    qDebug() << "发送间隔:" << intervalMs << "毫秒";
    qDebug() << "当前进度: 从第" << (currentIndex + 1) << "封开始";
    
    QDateTime now = QDateTime::currentDateTime();
    for (int i = currentIndex; i < count; ++i) {
        QDateTime sendTime = now.addMSecs((i - currentIndex) * intervalMs);
        qDebug() << QString("第%1封邮件 - 预计发送时间: %2")
                    .arg(i + 1)
                    .arg(sendTime.toString("yyyy-MM-dd hh:mm:ss"));
    }
    qDebug() << "===================";
}

int workerSendMail() {
    int totalEmails = (emailContents.size() + linesPerEmail - 1) / linesPerEmail;
    if (currentIndex >= totalEmails || currentIndex >= maxCount) {
        qDebug() << "所有邮件发送完成，程序退出";
        QFile::remove(progressFile);
        QCoreApplication::quit();
        return 0;
    }

    QStringList content = getEmailContent(currentIndex);
    QString emailText = content.join("\n");
    qDebug() << QString("发送第%1封邮件 (重试:%2/%3)").arg(currentIndex + 1).arg(retryCount).arg(MAX_RETRIES);
    
    Server server;
    server.setHost(mailConfig.smtpServer);
    server.setPort(mailConfig.smtpPort);
    server.setConnectionType(Server::SslConnection);
    server.setUsername(mailConfig.senderEmail);
    server.setPassword(mailConfig.senderPassword);

    MimeMessage message;
    EmailAddress sender(mailConfig.senderEmail, mailConfig.senderName);
    message.setSender(sender);
    EmailAddress to(mailConfig.recipientEmail, mailConfig.recipientName);
    message.addTo(to);
    message.setSubject(QString("邮件 #%1").arg(currentIndex + 1));

    auto text = std::make_shared<MimeText>();
    text->setText(emailText);
    message.addPart(text);

    ServerReply *reply = server.sendMail(message);
    
    int exitCode = 0;
    bool finished = false;
    QEventLoop loop;
    
    // 设置5秒超时定时器
    QTimer *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, [&loop, &finished, &exitCode] {
        if (!finished) {
            qDebug() << "发送超时(5秒)，视为失败";
            exitCode = -1;
            finished = true;
            loop.quit();
        }
    });
    
    QObject::connect(reply, &ServerReply::finished, [reply, &loop, &exitCode, &finished, timeoutTimer] {
        if (!finished) {
            timeoutTimer->stop();
            exitCode = reply->error() ? -3 : 0;
            finished = true;
            loop.quit();
        }
        reply->deleteLater();
    });

    timeoutTimer->start(5000); // 5秒超时
    loop.exec();
    
    timeoutTimer->deleteLater();
    
    if (exitCode == 0) {
        qDebug() << "邮件发送成功";
        currentIndex++;
        retryCount = 0;
        saveProgress();
    } else {
        retryCount++;
        qDebug() << QString("邮件发送失败，重试次数: %1/%2").arg(retryCount).arg(MAX_RETRIES);
        if (retryCount >= MAX_RETRIES) {
            qDebug() << "达到最大重试次数，跳过此邮件";
            currentIndex++;
            retryCount = 0;
        }
        saveProgress();
    }
    
    return exitCode;
}



int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("SimpleMail CLI");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("SimpleMail 命令行邮件发送工具");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("content", "邮件内容文件路径");
    parser.addPositionalArgument("interval", "发送间隔 (1d/1h/1m/1s/1000ms)");
    parser.addPositionalArgument("count", "发送数量");
    parser.addPositionalArgument("lines", "文件内容读取行数");

    QCommandLineOption configOption(QStringList() << "c" << "config", "配置文件路径", "file");
    QCommandLineOption smtpServerOption("smtp-server", "SMTP服务器地址", "server");
    QCommandLineOption smtpPortOption("smtp-port", "SMTP端口", "port", "465");
    QCommandLineOption senderNameOption("sender-name", "发件人名称", "name");
    QCommandLineOption senderEmailOption("sender-email", "发件人邮箱", "email");
    QCommandLineOption senderPasswordOption("sender-password", "发件人密码", "password");
    QCommandLineOption recipientNameOption("recipient-name", "收件人名称", "name");
    QCommandLineOption recipientEmailOption("recipient-email", "收件人邮箱", "email");
    QCommandLineOption resetOption("reset", "重置发送进度");

    parser.addOption(configOption);
    parser.addOption(smtpServerOption);
    parser.addOption(smtpPortOption);
    parser.addOption(senderNameOption);
    parser.addOption(senderEmailOption);
    parser.addOption(senderPasswordOption);
    parser.addOption(recipientNameOption);
    parser.addOption(recipientEmailOption);
    parser.addOption(resetOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.size() < 4) {
        parser.showHelp(1);
    }

    // 加载配置：优先级 命令行参数 > 配置文件 > 头文件默认值
    bool hasConfig = false;
    
    if (parser.isSet(configOption)) {
        if (loadConfigFromFile(parser.value(configOption), mailConfig)) {
            hasConfig = true;
        } else {
            qDebug() << "无法加载配置文件或配置不完整";
            return -1;
        }
    }

    // 命令行参数覆盖配置文件
    if (parser.isSet(smtpServerOption)) {
        mailConfig.smtpServer = parser.value(smtpServerOption);
        hasConfig = true;
    }
    if (parser.isSet(smtpPortOption)) {
        mailConfig.smtpPort = parser.value(smtpPortOption).toInt();
        hasConfig = true;
    }
    if (parser.isSet(senderNameOption)) {
        mailConfig.senderName = parser.value(senderNameOption);
        hasConfig = true;
    }
    if (parser.isSet(senderEmailOption)) {
        mailConfig.senderEmail = parser.value(senderEmailOption);
        hasConfig = true;
    }
    if (parser.isSet(senderPasswordOption)) {
        mailConfig.senderPassword = parser.value(senderPasswordOption);
        hasConfig = true;
    }
    if (parser.isSet(recipientNameOption)) {
        mailConfig.recipientName = parser.value(recipientNameOption);
        hasConfig = true;
    }
    if (parser.isSet(recipientEmailOption)) {
        mailConfig.recipientEmail = parser.value(recipientEmailOption);
        hasConfig = true;
    }

    // 如果没有通过参数或配置文件设置，使用头文件默认值
    if (!hasConfig) {
        mailConfig.smtpServer = SMTP_SERVER;
        mailConfig.smtpPort = SMTP_SERVER_PORT;
        mailConfig.senderName = SENDER_NAME;
        mailConfig.senderEmail = SENDER_EMAIL;
        mailConfig.senderPassword = SENDER_PASSWORD;
        mailConfig.recipientName = RECIPIENT_NAME;
        mailConfig.recipientEmail = RECIPIENT_EMAIL;
        qDebug() << "使用头文件默认配置";
    }

    // 验证必需配置
    if (mailConfig.smtpServer.isEmpty() || mailConfig.senderEmail.isEmpty() || 
        mailConfig.senderPassword.isEmpty() || mailConfig.recipientEmail.isEmpty()) {
        qDebug() << "错误: 缺少必需的邮件配置";
        qDebug() << "请通过以下方式之一提供配置:";
        qDebug() << "1. 配置文件: --config mail_config.ini";
        qDebug() << "2. 命令行参数: --smtp-server, --sender-email, --sender-password, --recipient-email";
        qDebug() << "3. 头文件: ../account_info.h (编译时配置)";
        return -1;
    }

    if (parser.isSet(resetOption)) {
        QFile::remove(progressFile);
        qDebug() << "发送进度已重置";
    }

    QString txtFile = args[0];
    qint64 interval = parseTimeInterval(args[1]);
    maxCount = args[2].toInt();
    linesPerEmail = args[3].toInt();

    if (interval <= 0) interval = 2000;
    if (maxCount <= 0) maxCount = 1;
    if (linesPerEmail <= 0) linesPerEmail = 1;

    emailContents = readTxtFile(txtFile);
    if (emailContents.isEmpty()) {
        qDebug() << "无法读取文件或文件为空:" << txtFile;
        return -1;
    }

    int totalEmails = (emailContents.size() + linesPerEmail - 1) / linesPerEmail;
    if (maxCount > totalEmails) {
        maxCount = totalEmails;
    }

    loadProgress();
    if (currentIndex >= maxCount) {
        qDebug() << "所有邮件已发送完成";
        return 0;
    }

    printSchedule(interval, maxCount);

    QTimer *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, workerSendMail);
    timer->setInterval(interval);
    timer->start();

    return app.exec();
}

