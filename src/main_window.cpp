#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtGui>
#include <QtNetwork>
#include <QtWebKit>
#include <QScriptValueIterator>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    ,ui(new Ui::MainWindow)
    ,m_ssl(new QSslSocket(this))
    ,m_engine(new QScriptEngine(this))
    ,m_token("")
    ,m_timer(new QTimer(this))
    ,m_status_lbl(new QLabel(this))
    ,m_tray_menu(new QMenu(this))
    ,m_msg_sound(0)
    ,m_tray(new QSystemTrayIcon(this))
{
    ui->setupUi(this);
    connect(m_ssl, SIGNAL(encrypted()), this, SLOT(socket_encrypted()));
    connect(m_ssl, SIGNAL(sslErrors(QList<QSslError>)), this,
            SLOT(socket_ssl_errors(QList<QSslError>)));
    connect(m_ssl, SIGNAL(readyRead()), this, SLOT(socket_ready_read()));
    connect(m_ssl, SIGNAL(disconnected()), this, SLOT(socket_disconnected()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(stay_alive()));

    QSslConfiguration config = m_ssl->sslConfiguration();
    config.setProtocol(QSsl::TlsV1);
    m_ssl->setSslConfiguration(config);

    statusBar()->addPermanentWidget(m_status_lbl, 1);
    m_status_lbl->setText(tr("Not Connected"));

    m_msg_sound = new QSound("snd/msg_received.wav", this);
    m_tray_menu->addAction(QIcon(":img/icons/door_out.png"), "Exit", this, SLOT(close()));
    m_tray->setIcon(QIcon(":img/icons/transmit.png"));
    m_tray->setContextMenu(m_tray_menu);
    m_tray->show();

    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(raise()));

    load_settings();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::changeEvent(QEvent *e) {
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::save_settings() {
    QSettings s("UDP Software", "TalkerApp", this);
    s.group();
}

void MainWindow::load_settings() {
    QSettings s("UDP Software", "TalkerApp", this);
}

void MainWindow::closeEvent(QCloseEvent *e) {
    qDebug() << "window closing...";
    logout();
}

void MainWindow::set_interface_enabled(const bool &enabled) {
    ui->action_login->setEnabled(!enabled);
    ui->action_logout->setEnabled(enabled);
    ui->btn_chat_submit->setEnabled(enabled);
    ui->le_chat_entry->setEnabled(enabled);
}

void MainWindow::socket_encrypted() {
    // TODO: cut this shit out, ask for a room
    qDebug() << "socket encrypted";
    QString body("{\"type\":\"connect\",\"room\":\"Main\","
                 "\"token\":\"%1\"}\r\n");
    body = body.arg(m_token);
    qDebug() << "sending" << body;
    m_ssl->write(body.toAscii());
}

void MainWindow::socket_ssl_errors(const QList<QSslError> &errors) {
    qDebug() << "SSL ERROR:" << errors;
}

void MainWindow::socket_disconnected() {
    set_interface_enabled(false);
    m_status_lbl->setText(tr("Not Connected"));
    m_timer->stop();
}

void MainWindow::socket_ready_read() {
    qDebug() << "socket is ready to read";

    // get the server's reply
    QString reply = QString(m_ssl->readAll()).trimmed();
    qDebug() << QString("server said: (%1)").arg(reply);


    QScriptValue val = m_engine->evaluate(QString("(%1)").arg(reply));
    if (m_engine->hasUncaughtException()) {
        qWarning() << "SCRIPT EXCEPTION" << m_engine->uncaughtException().toString();
        QMessageBox::warning(this, tr("Communication Error!"),
                             tr("Failed to parse response from server:\n\n%1")
                             .arg(reply));
        logout();
        return;
    }
    qDebug() << "Evaluated response:" << val.toString();

    QString response_type = val.property("type").toString();
    qDebug() << "RESPONSE DISPATCH:" << response_type;
    if (response_type == "connected") {
        QString user = val.property("user").property("name").toString();
        qDebug() << "user is:" << user;
        m_status_lbl->setText(tr("Connected as %1").arg(user));

        set_interface_enabled(true);
        m_timer->setInterval(20000);
        m_timer->start();
        ui->le_chat_entry->setFocus();

        /*QWebView *web = new QWebView(this);
        web->load(QUrl("http://udp.talkerapp.com/rooms/497"));
        web->show();
        ui->tabs_main->addTab(web, "Web Version");*/
    } else if (response_type == "users") {
        ui->tbl_users->clearContents();
        ui->tbl_users->setRowCount(val.property("users").property("length").toInteger());
        QScriptValue users_obj = val.property("users");
        if (users_obj.isArray()) {
            QScriptValueIterator it(users_obj);
            int i = 0;
            while(it.hasNext()) {
                it.next();
                QScriptValue user = it.value();
                QString user_name = user.property("name").toString();
                QString user_email= user.property("email").toString();
                qDebug() << "user in room" << user_name << "email" << user_email;
                QTableWidgetItem *name_item = new QTableWidgetItem(user_name);
                //QTableWidgetItem *email_item = new QTableWidgetItem(user_email);
                QTableWidgetItem *email_item = new QTableWidgetItem("user@email.com");
                ui->tbl_users->setItem(i, 0, name_item);
                ui->tbl_users->setItem(i, 1, email_item);
                i++;
            }
        }

    } else if (response_type == "error") {
        QString msg = val.property("message").toString();
        qWarning() << "SERVER SENT ERROR:" << msg;
        QMessageBox::warning(this, tr("Server Error!"),
                             tr("Server sent the following error:\n\n%1")
                             .arg(msg));
    } else if (response_type == "message") {
        handle_message(val);
    }
}

void MainWindow::login() {
    // Start sign-in form...
    bool ok;
    m_token = QInputDialog::getText(this,
                                    tr("Enter Your TalkerApp User Token"),
                                    tr("User Token:"), QLineEdit::Normal,
                                    "39c765a90eb7debd9dc19a6498c843cb1296cbe0", &ok);

    if (!ok) {
        return;
    }
    if (!m_ssl || !QSslSocket::supportsSsl()) {
        QMessageBox::warning(this, tr("Secure Sockets Library Not Found!"),
                             tr("This tool requires an SSL library with TLSv1 "
                                "support. And was not found on your system"));
        return;
    }
    qDebug() << "user gave" << m_token;

    // open a connection
    m_ssl->connectToHostEncrypted("talkerapp.com", 8500);
}

void MainWindow::logout() {
    qDebug() << "logging out...";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"close\"}\r\n");
    }
    m_ssl->close();
}

void MainWindow::stay_alive() {
    qDebug() << "pinging...";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"ping\"}\r\n");
    }
}

void MainWindow::submit_message() {
    QString msg = ui->le_chat_entry->text();
    ui->le_chat_entry->clear();
    qDebug() << "submitting message:" << msg;
    QString body("{\"type\":\"message\",\"content\":\"%1\"}\r\n");
    m_ssl->write(body.arg(msg).toAscii());
    ui->le_chat_entry->setFocus();
}

void MainWindow::handle_message(const QScriptValue &val) {
    m_msg_sound->play();
    //QDateTime timestamp = val.property("time").toDateTime();
    QString content = val.property("content").toString();
    QString sender = val.property("user").property("name").toString();

    QTreeWidgetItem *entry = new QTreeWidgetItem(ui->chat_log);
    entry->setText(0, sender);
    entry->setText(1, content);
    //entry->setText(1, QString("TIME:[%1]\n%2").arg(timestamp.toLocalTime().toString("MMM d h:m:s ap")).arg(content));

    ui->chat_log->scrollToBottom();

    if (!this->isActiveWindow()) {
        m_tray->showMessage(QString("New Message from %1").arg(sender), content, QSystemTrayIcon::Information, 2000);
        m_msg_sound->play();
    }
}
