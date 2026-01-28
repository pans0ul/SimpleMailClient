#include "sendemail.h"

#include "server.h"
#include "serverreply.h"
#include "sendemailwidget.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QSslError>
#include <QVBoxLayout>

using namespace SimpleMail;

SendEmail::SendEmail(QWidget *parent)
    : QWidget(parent)
    , ui(new SendEmailWidget(this))
    , m_settings("pans0ul", "SimpleMailClient")
    , m_sendTimer(new QTimer(this))
    , m_displayTimer(new QTimer(this))
    , m_currentIndex(0)
    , m_linesPerEmail(1)
    , m_totalEmails(0)
    , m_hasSentNum(0)
    , m_maxCount(0)
    , m_intervalMs(3000)
    , m_progressFile("mail_progress_gui.ini")
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(ui);
    layout->setContentsMargins(0, 0, 0, 0);

    //Normal Tab
    ui->host->setText(
        m_settings.value(QStringLiteral("host"), QStringLiteral("localhost")).toString());
    ui->port->setValue(m_settings.value(QStringLiteral("port"), 465).toInt());
    ui->username->setText(m_settings.value(QStringLiteral("username")).toString());
    ui->password->setText(m_settings.value(QStringLiteral("password")).toString());
    ui->security->setCurrentIndex(m_settings.value(QStringLiteral("ssl"), 1).toInt());
    ui->sender->setText(m_settings.value(QStringLiteral("sender")).toString());
    ui->recipients->setText(m_settings.value(QStringLiteral("recipients")).toString());
    ui->subject->setText(m_settings.value(QStringLiteral("subject")).toString());


    //Schedule Tab
    ui->hostSchd->setText(
            m_settings.value(QStringLiteral("host"), QStringLiteral("localhost")).toString());
    ui->portSchd->setValue(m_settings.value(QStringLiteral("port"), 465).toInt());
    ui->usernameSchd->setText(m_settings.value(QStringLiteral("username")).toString());
    
    ui->passwordSchd->setText(m_settings.value(QStringLiteral("password")).toString());
    ui->securitySchd->setCurrentIndex(m_settings.value(QStringLiteral("ssl"), 1).toInt());

    ui->senderSchd->setText(m_settings.value(QStringLiteral("sender")).toString());
    ui->recipientsSchd->setText(m_settings.value(QStringLiteral("recipients")).toString());
    ui->subjectSchd->setText(m_settings.value(QStringLiteral("subject")).toString());


    connect(ui->openFile, &QPushButton::clicked, this, &SendEmail::on_openFile_clicked);
    connect(ui->sendEmail, &QPushButton::clicked, this, &SendEmail::on_sendEmail_clicked);
    connect(ui->addAttachment, &QPushButton::clicked, this, &SendEmail::on_addAttachment_clicked);
    connect(ui->startSchedule, &QPushButton::clicked, this, &SendEmail::on_startSchedule_clicked);
    connect(ui->stopSchedule, &QPushButton::clicked, this, &SendEmail::on_stopSchedule_clicked);
    connect(ui->tabWidget,&QTabWidget::currentChanged, this, &SendEmail::on_tabPageChanged);


    connect(m_sendTimer, &QTimer::timeout, this, &SendEmail::sendNextEmail);

}

SendEmail::~SendEmail()
{
    delete ui;
}

void SendEmail::on_addAttachment_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if (dialog.exec()) {
        ui->attachments->addItems(dialog.selectedFiles());
    }
}

void SendEmail::on_openFile_clicked()
{
    QFileDialog dialog(this);
    dialog.setFileMode(QFileDialog::ExistingFiles);

    if (dialog.exec()) {
        QString filePath = dialog.selectedFiles().first();
        ui->uFilePath->setText(filePath);
        
        // Check if file changed and reset progress if needed
        if (m_currentFilePath != filePath) {
            m_currentFilePath = filePath;
            m_currentIndex = 0;
        }
        
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            ui->utextBrowser->setText(content);
            
            m_emailContents.clear();
            QStringList lines = content.split('\n');
            for (const QString &line : lines) {
                if (!line.trimmed().isEmpty()) {
                    m_emailContents.append(line);
                }
            }
            
            // Update total emails calculation when file is loaded
            m_linesPerEmail = ui->linesPerEmailSpin->value();
            m_totalEmails = (m_emailContents.size() + m_linesPerEmail - 1) / m_linesPerEmail;
            loadProgress();
            
            file.close();
        } else {
            QMessageBox::warning(this, "Error", "Failed to open file: " + filePath);
        }
    }
}


void SendEmail::on_sendEmail_clicked()
{
    // Send single email immediately
    MimeMessage message;
    message.setSender(EmailAddress{ui->sender->text()});
    message.setSubject(ui->subject->text());

    const QStringList rcptStringList =
        ui->recipients->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &to : rcptStringList) {
        message.addTo(EmailAddress{to});
    }

    message.addPart(std::make_shared<MimeHtml>(ui->texteditor->toHtml()));

    for (int i = 0; i < ui->attachments->count(); ++i) {
        QFile *file = new QFile(ui->attachments->item(i)->text());
        if (file->exists()) {
            message.addPart(std::make_shared<MimeAttachment>(std::shared_ptr<QFile>(file)));
        } else {
            delete file;
            QMessageBox::warning(this, "Warning", "Attachment file not found: " + ui->attachments->item(i)->text());
        }
    }

    m_settings.setValue(QStringLiteral("host"), ui->host->text());
    m_settings.setValue(QStringLiteral("port"), ui->port->value());
    m_settings.setValue(QStringLiteral("username"), ui->username->text());
    m_settings.setValue(QStringLiteral("password"), ui->password->text());
    m_settings.setValue(QStringLiteral("ssl"), ui->security->currentIndex());
    m_settings.setValue(QStringLiteral("sender"), ui->sender->text());
    m_settings.setValue(QStringLiteral("recipients"), ui->recipients->text());
    m_settings.setValue(QStringLiteral("subject"), ui->subject->text());

    sendMailAsync(message);
}

void SendEmail::sendMailAsync(const MimeMessage &msg)
{
    qDebug() << "sendMailAsync";

    const QString host = ui->host->text();
    const quint16 port(ui->port->value());
    const Server::ConnectionType ct = ui->security->currentIndex() == 0   ? Server::TcpConnection
                                      : ui->security->currentIndex() == 1 ? Server::SslConnection
                                                                          : Server::TlsConnection;

    Server *server = nullptr;
    for (auto srv : m_aServers) {
        if (srv->host() == host && srv->port() == port && srv->connectionType() == ct) {
            server = srv;
            break;
        }
    }

    if (!server) {
        server = new Server(this);
        connect(server, &Server::sslErrors, this, [](const QList<QSslError> &errors) {
            qDebug() << "Server SSL errors" << errors.size();
        });
        server->setHost(host);
        server->setPort(port);
        server->setConnectionType(ct);
        m_aServers.push_back(server);
    }

    const QString user = ui->username->text();
    if (!user.isEmpty()) {
        server->setAuthMethod(Server::AuthLogin);
        server->setUsername(user);
        server->setPassword(ui->password->text());
    }

    ServerReply *reply = server->sendMail(msg);
    connect(reply, &ServerReply::finished, this, [=] {
        qDebug() << "ServerReply finished" << reply->error() << reply->responseText();
        reply->deleteLater();
        if (reply->error()) {
            errorMessage(QLatin1String("Mail sending failed:\n") + reply->responseText());
        } else {
            QMessageBox okMessage(this);
            okMessage.setText(QLatin1String("The email was successfully sent:\n") +
                              reply->responseText());
            okMessage.exec();
        }
    });
}

void SendEmail::errorMessage(const QString &message)
{
    QMessageBox::critical(this, "Error", message);
}



void SendEmail::loadProgress()
{
    QSettings settings(m_progressFile, QSettings::IniFormat);
    QString savedFilePath = settings.value("filePath", "").toString();
    
    if (savedFilePath != m_currentFilePath) {
        m_currentIndex = 0;
    } else {
        m_currentIndex = settings.value("currentIndex", 0).toInt();
    }
}

void SendEmail::saveProgress()
{
    QSettings settings(m_progressFile, QSettings::IniFormat);
    settings.setValue("filePath", m_currentFilePath);
    settings.setValue("currentIndex", m_currentIndex);
}

QStringList SendEmail::getEmailContent(int index)
{
    QStringList content;
    int startLine = index * m_linesPerEmail;
    for (int i = 0; i < m_linesPerEmail && (startLine + i) < m_emailContents.size(); ++i) {
        content.append(m_emailContents[startLine + i]);
    }
    return content;
}

qint64 SendEmail::parseTimeInterval(const QString &timeStr)
{
    QString str = timeStr.toLower().trimmed();
    qint64 totalMs = 0;
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    // Qt5/Qt6: Use QRegularExpression
    QRegularExpression rx("(\\d+)([dhms])");
    QRegularExpressionMatchIterator it = rx.globalMatch(str);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        qint64 value = match.captured(1).toLongLong();
        QString unit = match.captured(2);
        
        if (unit == "d") totalMs += value * 24 * 60 * 60 * 1000;
        else if (unit == "h") totalMs += value * 60 * 60 * 1000;
        else if (unit == "m") totalMs += value * 60 * 1000;
        else if (unit == "s") totalMs += value * 1000;
    }
#else
    // Qt4: Use QRegExp
    QRegExp rx("(\\d+)([dhms])");
    int pos = 0;
    
    while ((pos = rx.indexIn(str, pos)) != -1) {
        qint64 value = rx.cap(1).toLongLong();
        QString unit = rx.cap(2);
        
        if (unit == "d") totalMs += value * 24 * 60 * 60 * 1000;
        else if (unit == "h") totalMs += value * 60 * 60 * 1000;
        else if (unit == "m") totalMs += value * 60 * 1000;
        else if (unit == "s") totalMs += value * 1000;
        
        pos += rx.matchedLength();
    }
#endif
    
    return totalMs > 0 ? totalMs : 3000;
}

void SendEmail::startScheduledSending()
{
    if (m_emailContents.isEmpty()) {
        QMessageBox::warning(this, "Warning", "Please load a content file first.");
        return;
    }

    // Parse interval from text input
    QString intervalStr = ui->intervalEdit->text().trimmed();
    m_intervalMs = parseTimeInterval(intervalStr);
    if (m_intervalMs <= 0) m_intervalMs = 3000;
    
    // Get email count and lines per email from spin boxes
    m_maxCount = ui->emailCountSpin->value();
    m_linesPerEmail = ui->linesPerEmailSpin->value();
    
    // Recalculate total emails based on lines per email
    m_totalEmails = (m_emailContents.size() + m_linesPerEmail - 1) / m_linesPerEmail;
    if (m_maxCount > m_totalEmails) {
        m_maxCount = m_totalEmails;
    }
    m_totalEmails = m_maxCount;

    m_nextSendTime = QDateTime::currentDateTime();
    m_sendTimer->start(m_intervalMs);
    ui->startSchedule->setEnabled(false);
    ui->stopSchedule->setEnabled(true);

    // Save settings
    m_settings.setValue(QStringLiteral("host"), ui->host->text());
    m_settings.setValue(QStringLiteral("port"), ui->port->value());
    m_settings.setValue(QStringLiteral("username"), ui->username->text());
    m_settings.setValue(QStringLiteral("password"), ui->password->text());
    m_settings.setValue(QStringLiteral("ssl"), ui->security->currentIndex());
    m_settings.setValue(QStringLiteral("sender"), ui->sender->text());
    m_settings.setValue(QStringLiteral("recipients"), ui->recipients->text());
    m_settings.setValue(QStringLiteral("subject"), ui->subject->text());
    
    QMessageBox::information(this, "Scheduled Sending", 
        QString("Started scheduled sending.\nInterval: %1\nTotal emails: %2\nLines per email: %3")
        .arg(intervalStr).arg(m_totalEmails).arg(m_linesPerEmail));
}

void SendEmail::stopScheduledSending()
{
    m_sendTimer->stop();
    ui->startSchedule->setEnabled(true);
    ui->stopSchedule->setEnabled(false);
    m_hasSentNum = 0;
}

void SendEmail::on_startSchedule_clicked()
{
    startScheduledSending();
}

void SendEmail::on_stopSchedule_clicked()
{
    stopScheduledSending();
}

void SendEmail::on_tabPageChanged(int index)
{
    if(index == 1) // If schedule tab is selected
    {
                // Load saved settings for schedule tab
        ui->hostSchd->setText(ui->host->text());
        ui->portSchd->setValue(ui->port->value());
        ui->usernameSchd->setText(ui->username->text());
        ui->passwordSchd->setText(ui->password->text());
        ui->securitySchd->setCurrentIndex(ui->security->currentIndex());
        ui->senderSchd->setText(ui->sender->text());
        ui->recipientsSchd->setText(ui->recipients->text());
        ui->subjectSchd->setText(ui->subject->text());    
    }
    else if(index == 0)
    {
        // Save settings from schedule tab back to normal tab
        ui->host->setText(ui->hostSchd->text());
        ui->port->setValue(ui->portSchd->value());
        ui->username->setText(ui->usernameSchd->text());
        ui->password->setText(ui->passwordSchd->text());
        ui->security->setCurrentIndex(ui->securitySchd->currentIndex());
        ui->sender->setText(ui->senderSchd->text());
        ui->recipients->setText(ui->recipientsSchd->text());
        ui->subject->setText(ui->subjectSchd->text());    
    }

}

void SendEmail::sendNextEmail()
{
    if(m_hasSentNum >= m_maxCount)
    {
        stopScheduledSending();
        QMessageBox::information(this, "Complete", "All emails have been sent.");
        QFile::remove(m_progressFile);
        m_hasSentNum = 0;
        return;
    }

    QStringList content = getEmailContent(m_currentIndex);
    QString emailText = content.join("\n");
    
    // Update the email content display in schedule tab
    ui->texteditor->setText(emailText);

    MimeMessage message;
    message.setSender(EmailAddress{ui->senderSchd->text()});
    message.setSubject(ui->subjectSchd->text().isEmpty() ? QString("Email #%1").arg(m_currentIndex + 1) : ui->subjectSchd->text());
    
    const QStringList rcptStringList =
        ui->recipientsSchd->text().split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &to : rcptStringList) {
        message.addTo(EmailAddress{to});
    }

    auto text = std::make_shared<MimeText>();
    text->setText(emailText);
    message.addPart(text);

    const QString host = ui->hostSchd->text();
    const quint16 port(ui->portSchd->value());
    const Server::ConnectionType ct = ui->securitySchd->currentIndex() == 0 ? Server::TcpConnection
                                      : ui->securitySchd->currentIndex() == 1 ? Server::SslConnection
                                                                          : Server::TlsConnection;

    Server *server = nullptr;
    for (auto srv : m_aServers) {
        if (srv->host() == host && srv->port() == port && srv->connectionType() == ct) {
            server = srv;
            break;
        }
    }

    if (!server) {
        server = new Server(this);
        connect(server, &Server::sslErrors, this, [](const QList<QSslError> &errors) {
            qDebug() << "Server SSL errors" << errors.size();
        });
        server->setHost(host);
        server->setPort(port);
        server->setConnectionType(ct);
        m_aServers.push_back(server);
    }

    const QString user = ui->usernameSchd->text();
    if (!user.isEmpty()) {
        server->setAuthMethod(Server::AuthLogin);
        server->setUsername(user);
        server->setPassword(ui->passwordSchd->text());
    }

    ServerReply *reply = server->sendMail(message);
    connect(reply, &ServerReply::finished, this, [=] {
        reply->deleteLater();
        if (reply->error()) {
            qDebug() << "Email send failed:" << reply->responseText();
        } else {
            qDebug() << "Email sent successfully:" << (m_hasSentNum + 1);
            m_currentIndex++;
            m_hasSentNum++;
            saveProgress();
        }
    });
    
    m_nextSendTime = QDateTime::currentDateTime().addMSecs(m_intervalMs);
}