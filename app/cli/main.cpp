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
    
    if (unit.startsWith("d")) return value * 24 * 60 * 60 * 1000; // days
    if (unit.startsWith("h")) return value * 60 * 60 * 1000;      // hours
    if (unit.startsWith("m")) return value * 60 * 1000;           // minutes
    if (unit.startsWith("s")) return value * 1000;                // seconds
    return value; // default milliseconds
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
    qDebug() << "=== Email Sending Schedule ===";
    qDebug() << "Total emails:" << count;
    qDebug() << "Lines per email:" << linesPerEmail;
    qDebug() << "Send interval:" << intervalMs << "ms";
    qDebug() << "Current progress: Starting from email" << (currentIndex + 1);
    
    QDateTime now = QDateTime::currentDateTime();
    for (int i = currentIndex; i < count; ++i) {
        QDateTime sendTime = now.addMSecs((i - currentIndex) * intervalMs);
        qDebug() << QString("Email #%1 - Scheduled time: %2")
                    .arg(i + 1)
                    .arg(sendTime.toString("yyyy-MM-dd hh:mm:ss"));
    }
    qDebug() << "===============================";
}

int workerSendMail() {
    int totalEmails = (emailContents.size() + linesPerEmail - 1) / linesPerEmail;
    if (currentIndex >= totalEmails || currentIndex >= maxCount) {
        qDebug() << "All emails sent, exiting program";
        QFile::remove(progressFile);
        QCoreApplication::quit();
        return 0;
    }

    QStringList content = getEmailContent(currentIndex);
    QString emailText = content.join("\n");
    qDebug() << QString("Sending email #%1 (retry: %2/%3)").arg(currentIndex + 1).arg(retryCount).arg(MAX_RETRIES);
    
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
    message.setSubject(QString("Email #%1").arg(currentIndex + 1));

    auto text = std::make_shared<MimeText>();
    text->setText(emailText);
    message.addPart(text);

    ServerReply *reply = server.sendMail(message);
    
    int exitCode = 0;
    bool finished = false;
    QEventLoop loop;
    
    // Set 5 second timeout timer
    QTimer *timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    QObject::connect(timeoutTimer, &QTimer::timeout, [&loop, &finished, &exitCode] {
        if (!finished) {
            qDebug() << "Send timeout (5s), marked as failed";
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

    timeoutTimer->start(5000); // 5 second timeout
    loop.exec();
    
    timeoutTimer->deleteLater();
    
    if (exitCode == 0) {
        qDebug() << "Email sent successfully";
        currentIndex++;
        retryCount = 0;
        saveProgress();
    } else {
        retryCount++;
        qDebug() << QString("Email send failed, retry count: %1/%2").arg(retryCount).arg(MAX_RETRIES);
        if (retryCount >= MAX_RETRIES) {
            qDebug() << "Max retries reached, skipping this email";
            currentIndex++;
            retryCount = 0;
        }
        saveProgress();
    }
    
    return exitCode;
}



int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("SimpleMailClient CLI");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("SimpleMailClient CLI email sending tool");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addPositionalArgument("content", "Email content file path");
    parser.addPositionalArgument("interval", "Send interval (1d/1h/1m/1s/1000ms)");
    parser.addPositionalArgument("count", "Number of emails to send");
    parser.addPositionalArgument("lines", "Lines to read per email");

    QCommandLineOption configOption(QStringList() << "c" << "config", "Config file path", "file");
    QCommandLineOption smtpServerOption("smtp-server", "SMTP server address", "server");
    QCommandLineOption smtpPortOption("smtp-port", "SMTP port", "port", "465");
    QCommandLineOption senderNameOption("sender-name", "Sender name", "name");
    QCommandLineOption senderEmailOption("sender-email", "Sender email", "email");
    QCommandLineOption senderPasswordOption("sender-password", "Sender password", "password");
    QCommandLineOption recipientNameOption("recipient-name", "Recipient name", "name");
    QCommandLineOption recipientEmailOption("recipient-email", "Recipient email", "email");
    QCommandLineOption resetOption("reset", "Reset sending progress");

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

    // Load config: Priority - command line args > config file > header defaults
    bool hasConfig = false;
    
    if (parser.isSet(configOption)) {
        if (loadConfigFromFile(parser.value(configOption), mailConfig)) {
            hasConfig = true;
        } else {
            qDebug() << "Unable to load config file or config incomplete";
            return -1;
        }
    }

    // Command line arguments override config file
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

    // If not set via arguments or config file, use header defaults
    if (!hasConfig) {
        mailConfig.smtpServer = SMTP_SERVER;
        mailConfig.smtpPort = SMTP_SERVER_PORT;
        mailConfig.senderName = SENDER_NAME;
        mailConfig.senderEmail = SENDER_EMAIL;
        mailConfig.senderPassword = SENDER_PASSWORD;
        mailConfig.recipientName = RECIPIENT_NAME;
        mailConfig.recipientEmail = RECIPIENT_EMAIL;
        qDebug() << "Using default config from header file";
    }

    // Validate required configuration
    if (mailConfig.smtpServer.isEmpty() || mailConfig.senderEmail.isEmpty() || 
        mailConfig.senderPassword.isEmpty() || mailConfig.recipientEmail.isEmpty()) {
        qDebug() << "Error: Missing required email configuration";
        qDebug() << "Please provide configuration via one of the following methods:";
        qDebug() << "1. Config file: --config mail_config.ini";
        qDebug() << "2. Command line arguments: --smtp-server, --sender-email, --sender-password, --recipient-email";
        qDebug() << "3. Header file: ../account_info.h (compile-time configuration)";
        return -1;
    }

    if (parser.isSet(resetOption)) {
        QFile::remove(progressFile);
        qDebug() << "Sending progress has been reset";
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
        qDebug() << "Unable to read file or file is empty:" << txtFile;
        return -1;
    }

    int totalEmails = (emailContents.size() + linesPerEmail - 1) / linesPerEmail;
    if (maxCount > totalEmails) {
        maxCount = totalEmails;
    }

    loadProgress();
    if (currentIndex >= maxCount) {
        qDebug() << "All emails have been sent";
        return 0;
    }

    printSchedule(interval, maxCount);

    QTimer *timer = new QTimer(&app);
    QObject::connect(timer, &QTimer::timeout, workerSendMail);
    timer->setInterval(interval);
    timer->start();

    return app.exec();
}

