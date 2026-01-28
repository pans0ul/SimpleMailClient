#include "sendemailwidget.h"

SendEmailWidget::SendEmailWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupFonts();
    
    // Connect signals to update task schedule
    connect(emailCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SendEmailWidget::updateTaskSchedule);
    connect(intervalEdit, &QLineEdit::textChanged,
            this, &SendEmailWidget::updateTaskSchedule);
}

void SendEmailWidget::setupUI()
{
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();
    
    setMinimumSize(screenWidth * 0.5, screenHeight * 0.75);
    resize(screenWidth * 0.6, screenHeight * 0.9);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(getScaledSize(10));
    mainLayout->setContentsMargins(getScaledSize(15), getScaledSize(15), 
                                   getScaledSize(15), getScaledSize(15));
    
    tabWidget = new QTabWidget();
    tabWidget->setMinimumHeight(getScaledSize(500));
    
    setupNormalSendTab();
    setupScheduledSendTab();
    
    mainLayout->addWidget(tabWidget);
}

void SendEmailWidget::setupFonts()
{
    setFont(getScaledFont(10));
    
    QList<QLabel*> labels = findChildren<QLabel*>();
    for (QLabel *label : labels) {
        label->setFont(getScaledFont(10));
    }
    
    QList<QLineEdit*> lineEdits = findChildren<QLineEdit*>();
    for (QLineEdit *edit : lineEdits) {
        edit->setFont(getScaledFont(10));
    }
    
    QList<QPushButton*> buttons = findChildren<QPushButton*>();
    for (QPushButton *button : buttons) {
        if (button != sendEmail) {
            button->setFont(getScaledFont(10));
        }
    }
    
    if (texteditor) texteditor->setFont(getScaledFont(10));
    if (utextBrowser) utextBrowser->setFont(getScaledFont(9));
    if (attachments) attachments->setFont(getScaledFont(9));
}

QFont SendEmailWidget::getScaledFont(int baseSize, bool bold)
{
    QScreen *screen = QApplication::primaryScreen();
    qreal dpi = screen->logicalDotsPerInch();
    qreal scaleFactor = dpi / 96.0;
    
    int scaledSize = qMax(8, static_cast<int>(baseSize * scaleFactor));
    
    QFont font;
    font.setPointSize(scaledSize);
    font.setBold(bold);
    return font;
}

int SendEmailWidget::getScaledSize(int baseSize)
{
    QScreen *screen = QApplication::primaryScreen();
    qreal dpi = screen->logicalDotsPerInch();
    qreal scaleFactor = dpi / 96.0;
    
    return qMax(baseSize, static_cast<int>(baseSize * scaleFactor));
}

void SendEmailWidget::setupNormalSendTab()
{
    QWidget *normalTab = new QWidget();
    QVBoxLayout *normalLayout = new QVBoxLayout(normalTab);
    normalLayout->setSpacing(getScaledSize(10));
    
    normalLayout->addWidget(createConfigSection());
    normalLayout->addWidget(createEmailDetailsSection());
    normalLayout->addWidget(createEmailContentSection());
    normalLayout->addWidget(createNormalAttachmentsSection());
    
    sendEmail = new QPushButton("Send Email");
    sendEmail->setMinimumHeight(getScaledSize(40));
    sendEmail->setFont(getScaledFont(12, true));
    sendEmail->setStyleSheet("QPushButton { background-color: #0066cc; color: white; border-radius: 5px; }"
                            "QPushButton:hover { background-color: #0052a3; }"
                            "QPushButton:pressed { background-color: #004080; }");
    normalLayout->addWidget(sendEmail);
    
    normalLayout->addStretch();
    tabWidget->addTab(normalTab, "Normal Send");
}

void SendEmailWidget::setupScheduledSendTab()
{
    QWidget *scheduleTab = new QWidget();
    QHBoxLayout *mainScheduleLayout = new QHBoxLayout(scheduleTab);
    mainScheduleLayout->setSpacing(getScaledSize(10));
    
    // Main content area
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *scheduleLayout = new QVBoxLayout(contentWidget);
    scheduleLayout->setSpacing(getScaledSize(10));
    
    // Sidebar for schedule display
    QWidget *sidebarWidget = new QWidget();
    sidebarWidget->setFixedWidth(getScaledSize(300));
    sidebarWidget->setStyleSheet("QWidget { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 5px; }");
    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarWidget);
    
    QLabel *sidebarTitle = new QLabel("Email Sending Schedule");
    sidebarTitle->setFont(getScaledFont(12, true));
    sidebarTitle->setStyleSheet("QLabel { background-color: #007bff; color: white; padding: 8px; border-radius: 3px; }");
    sidebarLayout->addWidget(sidebarTitle);
    
    scheduledTasksList = new QListWidget();
    scheduledTasksList->setStyleSheet("QListWidget { border: none; background-color: transparent; }");
    sidebarLayout->addWidget(scheduledTasksList);
    
    mainScheduleLayout->addWidget(contentWidget, 2);
    mainScheduleLayout->addWidget(sidebarWidget, 1);
    
    QHBoxLayout *fileLayout = new QHBoxLayout();
    openFile = new QPushButton("Open File");
    openFile->setMinimumHeight(getScaledSize(35));
    uFilePath = new QLineEdit();
    uFilePath->setMinimumHeight(getScaledSize(35));
    uFilePath->setPlaceholderText("Select content file for batch sending...");
    fileLayout->addWidget(openFile);
    fileLayout->addWidget(uFilePath);
    scheduleLayout->addLayout(fileLayout);
    
    QLabel *contentLabel = new QLabel("Content Preview:");
    contentLabel->setFont(getScaledFont(12, true));
    scheduleLayout->addWidget(contentLabel);
    utextBrowser = new QTextBrowser();
    utextBrowser->setMinimumHeight(getScaledSize(120));
    utextBrowser->setMaximumHeight(getScaledSize(150));
    utextBrowser->setPlaceholderText("File content will be displayed here...");
    scheduleLayout->addWidget(utextBrowser);


    QWidget *scheduleConfigWidget = new QWidget();
    QVBoxLayout *scheduleConfigLayout = new QVBoxLayout(scheduleConfigWidget);

    QLabel *scheduleLabel = new QLabel("Schedule Configuration");
    scheduleLabel->setFont(getScaledFont(14, true));
    scheduleConfigLayout->addWidget(scheduleLabel);
    
    // Interval configuration
    QHBoxLayout *intervalLayout = new QHBoxLayout();
    QLabel *intervalLabel = new QLabel("Interval:");
    intervalEdit = new QLineEdit("3s");
    intervalEdit->setMinimumHeight(getScaledSize(30));
    intervalEdit->setMaximumWidth(getScaledSize(80));
    intervalEdit->setPlaceholderText("e.g. 3s, 1m, 1h");
    intervalEdit->setToolTip("Set sending interval. Supports combinations like:\n"
                            "• 3s (3 seconds)\n"
                            "• 2m (2 minutes)\n"
                            "• 1h (1 hour)\n"
                            "• 1d (1 day)\n"
                            "• 1d2h30m (1 day, 2 hours, 30 minutes)\n"
                            "• 2h30s (2 hours, 30 seconds)");
    intervalLayout->addWidget(intervalLabel);
    intervalLayout->addWidget(intervalEdit);
    
    QLabel *countLabel = new QLabel("Email Count:");
    emailCountSpin = new QSpinBox();
    emailCountSpin->setRange(1, 10000);
    emailCountSpin->setValue(10);
    emailCountSpin->setMinimumHeight(getScaledSize(30));
    intervalLayout->addWidget(countLabel);
    intervalLayout->addWidget(emailCountSpin);
    
    QLabel *linesLabel = new QLabel("Lines per Email:");
    linesPerEmailSpin = new QSpinBox();
    linesPerEmailSpin->setRange(1, 100);
    linesPerEmailSpin->setValue(1);
    linesPerEmailSpin->setMinimumHeight(getScaledSize(30));
    intervalLayout->addWidget(linesLabel);
    intervalLayout->addWidget(linesPerEmailSpin);
    intervalLayout->addStretch();
    scheduleConfigLayout->addLayout(intervalLayout);
    
    // Timer display
    scheduleLayout->addWidget(scheduleConfigWidget);
    

    
    scheduleLayout->addWidget(createSchdConfigSection());
    scheduleLayout->addWidget(createSchdEmailDetailsSection());
    scheduleLayout->addWidget(createSchdEmailContentSection());
    

    
    
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startSchedule = new QPushButton("Start Scheduled Sending");
    startSchedule->setMinimumHeight(getScaledSize(40));
    startSchedule->setFont(getScaledFont(12, true));
    startSchedule->setStyleSheet("QPushButton { background-color: #28a745; color: white; border-radius: 5px; }"
                                "QPushButton:hover { background-color: #218838; }"
                                "QPushButton:pressed { background-color: #1e7e34; }");
    
    stopSchedule = new QPushButton("Stop Scheduled Sending");
    stopSchedule->setMinimumHeight(getScaledSize(40));
    stopSchedule->setFont(getScaledFont(12, true));
    stopSchedule->setStyleSheet("QPushButton { background-color: #dc3545; color: white; border-radius: 5px; }"
                               "QPushButton:hover { background-color: #c82333; }"
                               "QPushButton:pressed { background-color: #bd2130; }");
    stopSchedule->setEnabled(false);
    
    buttonLayout->addWidget(startSchedule);
    buttonLayout->addWidget(stopSchedule);
    scheduleLayout->addLayout(buttonLayout);
    

    
    scheduleLayout->addStretch();
    tabWidget->addTab(scheduleTab, "Scheduled Send");
    
    // Initialize task schedule display
    QTimer::singleShot(0, this, &SendEmailWidget::updateTaskSchedule);
}

QWidget* SendEmailWidget::createConfigSection()
{
    QWidget *configWidget = new QWidget();
    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    
    QLabel *smtpLabel = new QLabel("SMTP Configuration");
    smtpLabel->setFont(getScaledFont(14, true));
    configLayout->addWidget(smtpLabel);
    
    QGridLayout *smtpLayout = new QGridLayout();
    smtpLayout->setSpacing(getScaledSize(8));
    
    smtpLayout->addWidget(new QLabel("SMTP server:"), 0, 0);
    host = new QLineEdit("localhost");
    host->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(host, 0, 1);
    
    smtpLayout->addWidget(new QLabel("Port:"), 0, 2);
    port = new QSpinBox();
    port->setRange(1, 99999);
    port->setValue(465);
    port->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(port, 0, 3);
    
    security = new QComboBox();
    security->addItems({"Unencrypted", "SSL", "TLS/STARTTLS"});
    security->setCurrentText("SSL");
    security->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(security, 0, 4);
    
    smtpLayout->addWidget(new QLabel("Username:"), 1, 0);
    username = new QLineEdit();
    username->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(username, 1, 1);
    
    smtpLayout->addWidget(new QLabel("Password:"), 1, 2);
    password = new QLineEdit();
    password->setEchoMode(QLineEdit::Password);
    password->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(password, 1, 3, 1, 2);
    
    configLayout->addLayout(smtpLayout);
    return configWidget;
}


QWidget* SendEmailWidget::createSchdConfigSection()
{
    QWidget *configWidget = new QWidget();
    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    
    QLabel *smtpLabel = new QLabel("SMTP Configuration");
    smtpLabel->setFont(getScaledFont(14, true));
    configLayout->addWidget(smtpLabel);
    
    QGridLayout *smtpLayout = new QGridLayout();
    smtpLayout->setSpacing(getScaledSize(8));
    
    smtpLayout->addWidget(new QLabel("SMTP server:"), 0, 0);
    hostSchd = new QLineEdit("localhost");
    hostSchd->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(hostSchd, 0, 1);
    
    smtpLayout->addWidget(new QLabel("Port:"), 0, 2);
    portSchd = new QSpinBox();
    portSchd->setRange(1, 99999);
    portSchd->setValue(465);
    portSchd->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(portSchd, 0, 3);
    
    securitySchd = new QComboBox();
    securitySchd->addItems({"Unencrypted", "SSL", "TLS/STARTTLS"});
    securitySchd->setCurrentText("SSL");
    securitySchd->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(securitySchd, 0, 4);
    
    smtpLayout->addWidget(new QLabel("Username:"), 1, 0);
    usernameSchd = new QLineEdit();
    usernameSchd->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(usernameSchd, 1, 1);
    
    smtpLayout->addWidget(new QLabel("Password:"), 1, 2);
    passwordSchd = new QLineEdit();
    passwordSchd->setEchoMode(QLineEdit::Password);
    passwordSchd->setMinimumHeight(getScaledSize(30));
    smtpLayout->addWidget(passwordSchd, 1, 3, 1, 2);
    
    configLayout->addLayout(smtpLayout);
    return configWidget;
}



QWidget* SendEmailWidget::createEmailDetailsSection()
{
    QWidget *emailWidget = new QWidget();
    QVBoxLayout *emailLayout = new QVBoxLayout(emailWidget);
    
    QLabel *emailLabel = new QLabel("Email Details");
    emailLabel->setFont(getScaledFont(14, true));
    emailLayout->addWidget(emailLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(getScaledSize(8));
    
    sender = new QLineEdit();
    sender->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Sender:", sender);
    
    recipients = new QLineEdit();
    recipients->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Recipients:", recipients);
    
    subject = new QLineEdit();
    subject->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Subject:", subject);
    
    emailLayout->addLayout(formLayout);
    
    return emailWidget;
}

QWidget* SendEmailWidget::createSchdEmailDetailsSection()
{
    QWidget *emailWidget = new QWidget();
    QVBoxLayout *emailLayout = new QVBoxLayout(emailWidget);
    
    QLabel *emailLabel = new QLabel("Email Details");
    emailLabel->setFont(getScaledFont(14, true));
    emailLayout->addWidget(emailLabel);
    
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(getScaledSize(8));
    
    senderSchd = new QLineEdit();
    senderSchd->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Sender:", senderSchd);
    
    recipientsSchd = new QLineEdit();
    recipientsSchd->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Recipients:", recipientsSchd);
    
    subjectSchd = new QLineEdit();
    subjectSchd->setMinimumHeight(getScaledSize(30));
    formLayout->addRow("Subject:", subjectSchd);
    
    emailLayout->addLayout(formLayout);
    
    return emailWidget;
}
QWidget* SendEmailWidget::createEmailContentSection()
{
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    
    QLabel *contentLabel = new QLabel("Email Content");
    contentLabel->setFont(getScaledFont(14, true));
    contentLayout->addWidget(contentLabel);
    
    texteditor = new QTextEdit();
    texteditor->setMinimumHeight(getScaledSize(150));
    contentLayout->addWidget(texteditor);
    
    return contentWidget;
}

QWidget* SendEmailWidget::createSchdEmailContentSection()
{
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);
    
    QLabel *contentLabel = new QLabel("Email Content");
    contentLabel->setFont(getScaledFont(14, true));
    contentLayout->addWidget(contentLabel);
    
    texteditor = new QTextEdit();
    texteditor->setPlaceholderText("Email Content will be auto-generated...");
    texteditor->setMinimumHeight(getScaledSize(150));
    contentLayout->addWidget(texteditor);
    
    return contentWidget;
}

QWidget* SendEmailWidget::createNormalAttachmentsSection()
{
    QWidget *attachWidget = new QWidget();
    QVBoxLayout *attachLayout = new QVBoxLayout(attachWidget);
    
    QHBoxLayout *attachHeaderLayout = new QHBoxLayout();
    QLabel *attachLabel = new QLabel("Attachments:");
    attachLabel->setFont(getScaledFont(12, true));
    addAttachment = new QPushButton("Add File");
    addAttachment->setMinimumHeight(getScaledSize(30));
    addAttachment->setMaximumWidth(getScaledSize(100));
    attachHeaderLayout->addWidget(attachLabel);
    attachHeaderLayout->addStretch();
    attachHeaderLayout->addWidget(addAttachment);
    attachLayout->addLayout(attachHeaderLayout);
    
    attachments = new QListWidget();
    attachments->setMaximumHeight(getScaledSize(60));
    attachLayout->addWidget(attachments);
    
    return attachWidget;
}

void SendEmailWidget::updateTaskSchedule()
{
    if (!scheduledTasksList) return;
    
    scheduledTasksList->clear();
    
    int emailCount = emailCountSpin->value();
    QString interval = intervalEdit->text();
    
    // Parse interval using same logic as CLI
    qint64 intervalMs = 0;
    QString str = interval.toLower().trimmed();
    
#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
    // Qt5/Qt6: Use QRegularExpression
    QRegularExpression rx("(\\d+)([dhms])");
    QRegularExpressionMatchIterator it = rx.globalMatch(str);
    
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        qint64 value = match.captured(1).toLongLong();
        QString unit = match.captured(2);
        
        if (unit == "d") intervalMs += value * 24 * 60 * 60 * 1000;
        else if (unit == "h") intervalMs += value * 60 * 60 * 1000;
        else if (unit == "m") intervalMs += value * 60 * 1000;
        else if (unit == "s") intervalMs += value * 1000;
    }
#else
    // Qt4: Use QRegExp
    QRegExp rx("(\\d+)([dhms])");
    int pos = 0;
    
    while ((pos = rx.indexIn(str, pos)) != -1) {
        qint64 value = rx.cap(1).toLongLong();
        QString unit = rx.cap(2);
        
        if (unit == "d") intervalMs += value * 24 * 60 * 60 * 1000;
        else if (unit == "h") intervalMs += value * 60 * 60 * 1000;
        else if (unit == "m") intervalMs += value * 60 * 1000;
        else if (unit == "s") intervalMs += value * 1000;
        
        pos += rx.matchedLength();
    }
#endif
    
    if (intervalMs <= 0) return;
    
    // Add schedule header info
    scheduledTasksList->addItem(QString("Total emails: %1").arg(emailCount));
    scheduledTasksList->addItem(QString("Lines per email: %1").arg(linesPerEmailSpin->value()));
    scheduledTasksList->addItem(QString("Send interval: %1ms").arg(intervalMs));
    scheduledTasksList->addItem("Current progress: Starting from email 1");
    scheduledTasksList->addItem(""); // separator
    
    QDateTime now = QDateTime::currentDateTime();
    for (int i = 0; i < emailCount; ++i) {
        QDateTime sendTime = now.addMSecs(i * intervalMs);
        QString taskInfo = QString("Email #%1 - %2")
                          .arg(i + 1)
                          .arg(sendTime.toString("yyyy-MM-dd hh:mm:ss"));
        scheduledTasksList->addItem(taskInfo);
    }
}

void SendEmailWidget::addTaskToSchedule(const QString &taskInfo)
{
    if (scheduledTasksList) {
        scheduledTasksList->addItem(taskInfo);
    }
}

void SendEmailWidget::clearTaskSchedule()
{
    if (scheduledTasksList) {
        scheduledTasksList->clear();
    }
}