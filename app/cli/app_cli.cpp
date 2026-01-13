#include <QtCore>
#include <QTimer>
#include <QTextStream>
#include <QFile>
#include <QDateTime>
#include <QSettings>

#include "../../src/SimpleMail"
#include "../account_info.h"  

using namespace SimpleMail;

QStringList emailContents;
int currentIndex = 0;
int maxCount = 0;
int linesPerEmail = 1;
int retryCount = 0;
const int MAX_RETRIES = 3;
QString progressFile = "mail_progress.ini";

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
    qDebug() << "每封邮件行数:" << linesPerEmail;
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
    server.setHost(QLatin1String(SMTP_SERVER));
    server.setPort(SMTP_SERVER_PORT);
    server.setConnectionType(Server::SslConnection);
    server.setUsername(QLatin1String(SENDER_EMAIL));
    server.setPassword(QLatin1String(SENDER_PASSWORD));

    MimeMessage message;
    EmailAddress sender(QLatin1String(SENDER_EMAIL), QLatin1String(SENDER_NAME));
    message.setSender(sender);
    EmailAddress to(QLatin1String(RECIPIENT_EMAIL), QLatin1String("Recipient's Name"));
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

    if (argc < 5) {
        qDebug() << "用法: myapp <txt文件路径> <发送间隔> <发送数量> <每封邮件行数> [--reset]";
        qDebug() << "时间单位: 1d(天) 1h(小时) 1m(分钟) 1s(秒) 1000(毫秒)";
        qDebug() << "--reset: 重置发送进度，从头开始";
        return -1;
    }

    // 检查是否需要重置进度
    bool resetProgress = false;
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "--reset") {
            resetProgress = true;
            break;
        }
    }

    if (resetProgress) {
        QFile::remove(progressFile);
        qDebug() << "发送进度已重置";
    }

    QString txtFile = argv[1];
    qint64 interval = parseTimeInterval(argv[2]);
    maxCount = QString(argv[3]).toInt();
    linesPerEmail = QString(argv[4]).toInt();

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
