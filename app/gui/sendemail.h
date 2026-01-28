#ifndef SENDEMAIL_H
#define SENDEMAIL_H

#include "../../src/SimpleMail"
#include "sendemailwidget.h"

#include <QSettings>
#include <QWidget>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QRegularExpression>
#else
#include <QRegExp>
#endif



namespace SimpleMail {
class Server;
}

using namespace SimpleMail;

class SendEmail : public QWidget
{
    Q_OBJECT

public:
    explicit SendEmail(QWidget *parent = nullptr);
    ~SendEmail();

private Q_SLOTS:
    void on_addAttachment_clicked();
    void on_openFile_clicked();
    void on_sendEmail_clicked();
    void on_startSchedule_clicked();
    void on_stopSchedule_clicked();
    void on_tabPageChanged(const int);
    void sendMailAsync(const MimeMessage &msg);
    void sendNextEmail();

private:
    QSettings m_settings;
    std::vector<Server *> m_aServers;
    SendEmailWidget *ui;
    QTimer *m_sendTimer;
    QTimer *m_displayTimer;
    QStringList m_emailContents;
    int m_currentIndex;
    int m_linesPerEmail;
    int m_totalEmails;
    int m_maxCount;
    int m_hasSentNum;
    qint64 m_intervalMs;
    QDateTime m_nextSendTime;
    QString m_progressFile;
    QString m_currentFilePath;

    void errorMessage(const QString &message);
    void loadProgress();
    void saveProgress();
    QStringList getEmailContent(int index);
    qint64 parseTimeInterval(const QString &timeStr);
    void startScheduledSending();
    void stopScheduledSending();
};

#endif // SENDEMAIL_H
