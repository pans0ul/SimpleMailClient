#ifndef SENDEMAILWIDGET_H
#define SENDEMAILWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QTextBrowser>
#include <QPushButton>
#include <QListWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QTimeEdit>
#include <QDateTime>
#include <QTimer>
#include <QApplication>
#include <QScreen>

class SendEmailWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SendEmailWidget(QWidget *parent = nullptr);

    // UI elements
    QTabWidget *tabWidget;
    
    // Common elements
    QPushButton *openFile;
    QLineEdit *uFilePath;
    QTextBrowser *utextBrowser;
    QLineEdit *host;
    QSpinBox *port;
    QLineEdit *username;
    QLineEdit *password;
    QComboBox *security;
    QLineEdit *sender;
    QLineEdit *recipients;
    QLineEdit *subject;
    QTextEdit *texteditor;
    QListWidget *attachments;
    QPushButton *addAttachment;
    
    // Normal send tab
    QPushButton *sendEmail;
    
    // Scheduled send tab
    QPushButton *startSchedule;
    QPushButton *stopSchedule;
    QLineEdit *intervalEdit;
    QSpinBox *emailCountSpin;
    QSpinBox *linesPerEmailSpin;
    QListWidget *scheduledTasksList;

public slots:
    void updateTaskSchedule();
    void addTaskToSchedule(const QString &taskInfo);
    void clearTaskSchedule();

private:
    void setupUI();
    void setupFonts();
    void setupNormalSendTab();
    void setupScheduledSendTab();
    QWidget* createConfigSection();
    QWidget* createEmailDetailsSection();
    QWidget* createEmailContentSection();
    QWidget* createNormalAttachmentsSection();
    QFont getScaledFont(int baseSize, bool bold = false);
    int getScaledSize(int baseSize);
};

#endif // SENDEMAILWIDGET_H