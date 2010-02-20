#include "main_window.h"
#include "ui_main_window.h"
#include "ui_account_edit_dialog.h"
#include "talker_account.h"
#include <QtGui>
#include <QtNetwork>
#include <QtWebKit>
#include <QScriptValueIterator>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(new QSettings(this))
    , m_net(new QNetworkAccessManager(this))
    , m_ssl(new QSslSocket(this))
    , m_engine(new QScriptEngine(this))
    , m_token("")
    , m_timer(new QTimer(this))
    , m_status_lbl(new QLabel(this))
    , m_tray_menu(new QMenu(this))
    , m_tray(new QSystemTrayIcon(this))
    , m_acct(0)
{
    // load up our pretty design
    ui->setupUi(this);

    // turn off the toolbar
    ui->toolBar->hide();

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

    //setup system tray icon
    m_tray_menu->addAction(QIcon(":img/icons/door_out.png"), tr("E&xit"), this, SLOT(close()));
    m_tray->setContextMenu(m_tray_menu);
    m_tray->setIcon(QIcon(":img/icons/transmit.png"));
    m_tray->setToolTip("SmoothTalker");
    m_tray->show();

    load_settings();

    if (load_accounts() < 1) { // no configured accounts, launch the dialog
        QDialog *d = new QDialog(this);
        Ui::AccountEditDialog *dui = new Ui::AccountEditDialog;
        dui->setupUi(d);
        while (!d->exec()) {
            if  (QMessageBox::question(this, tr("Do this later?"),
                              tr("Do you want to enter your account information later?"),
                              QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {
                qDebug() << "user cancelled";
                break;
            }
        }
        TalkerAccount *acct = new TalkerAccount(dui->le_token->text(), dui->le_domain->text());
        m_accounts.push_back(acct);
        m_acct = acct;
        save_accounts();
        delete dui;
        d->deleteLater();
    }
    m_acct = m_accounts.at(0);
    qDebug() << "first account" << m_acct->token() << "domain:" << m_acct->domain();

    // get rooms...
    QUrl url(QString("https://%1.talkerapp.com/rooms.json").arg(m_acct->domain()));
    QNetworkRequest req(url);
    req.setRawHeader(QByteArray("Accept"),
                     QByteArray("application/json"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QByteArray("application/json"));
    req.setRawHeader(QByteArray("X-Talker-Token"),
                     m_acct->token().toAscii());

    qDebug() << "sending request" << req.url();
    foreach(QByteArray hdr, req.rawHeaderList()) {
        qDebug() << "HEADER:" << hdr << ":" << req.rawHeader(hdr);
    }

    connect(m_net, SIGNAL(finished(QNetworkReply*)), SLOT(rooms_request_finished(QNetworkReply*)));
    m_net->get(req);
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
    if (!m_settings) {
        m_settings = new QSettings(this);
    }
    m_settings->beginGroup("geometry");
    m_settings->setValue("size", size());
    m_settings->setValue("pos", pos());
    m_settings->endGroup();
}

void MainWindow::load_settings() {
    if (!m_settings) {
        m_settings = new QSettings(this);
    }
    m_settings->beginGroup("geometry");
    resize(m_settings->value("size", QSize(500, 600)).toSize());
    move(m_settings->value("pos", QPoint(200, 200)).toPoint());
    m_settings->endGroup();
}

int MainWindow::load_accounts() {
    if (!m_settings) {
        m_settings = new QSettings(this);
    }
    int total_accounts = m_settings->beginReadArray("accounts");
    for (int i = 0; i < total_accounts; ++i) {
        m_settings->setArrayIndex(i);
        this->m_accounts.append(new TalkerAccount(*m_settings));
    }
    m_settings->endArray();
    qDebug() << "loaded" << total_accounts << "accounts from settings";
    return total_accounts;
}

void MainWindow::save_accounts() {
    if (!m_settings) {
        m_settings = new QSettings(this);
    }
    m_settings->beginWriteArray("accounts", m_accounts.size());
    for (int i = 0; i < m_accounts.size(); ++i) {
        m_settings->setArrayIndex(i);
        m_accounts.at(i)->save(*m_settings);
    }
    m_settings->endArray();
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

void MainWindow::rooms_request_finished(QNetworkReply *r) {
    m_net->disconnect(this, SLOT(rooms_request_finished(QNetworkReply*)));
    QString reply(r->readAll());
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

    if (val.isArray()) {
        QMap<QString, int> rooms;
        QScriptValueIterator it(val);
        while(it.hasNext()) {
            it.next();
            QScriptValue room = it.value();
            QString room_name = room.property("name").toString();
            int room_id = room.property("id").toInteger();
            rooms.insert(room_name, room_id);
        }

        m_room_to_join = QInputDialog::getItem(this, tr("Choose which room to join"),
                                                     tr("Select a room"), rooms.keys(), 0, false);
        qDebug() << "will join" << m_room_to_join << rooms.value(m_room_to_join);
        login();
    }

    //[{"name": "Main", "id": 497}, {"name": "Second Room", "id": 512}]
    r->deleteLater();
}

void MainWindow::socket_encrypted() {
    qDebug() << "socket encrypted";
    QString body("{\"type\":\"connect\",\"room\":\"%1\","
                 "\"token\":\"%2\"}\r\n");
    body = body.arg(m_room_to_join).arg(m_acct->token());
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
    if (!m_ssl || !QSslSocket::supportsSsl()) {
        QMessageBox::warning(this, tr("Secure Sockets Library Not Found!"),
                             tr("This tool requires an SSL library with TLSv1 "
                                "support. And was not found on your system"));
        return;
    }
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
    //QDateTime timestamp = val.property("time").toDateTime();
    QString content = val.property("content").toString();
    QString sender = val.property("user").property("name").toString();

    QTreeWidgetItem *entry = new QTreeWidgetItem(ui->chat_log);
    entry->setText(0, sender);
    entry->setText(1, content);
    //entry->setText(1, QString("TIME:[%1]\n%2").arg(timestamp.toLocalTime().toString("MMM d h:m:s ap")).arg(content));

    ui->chat_log->scrollToBottom();

    if (!isActiveWindow()) {
        m_tray->showMessage(QString("message from %1").arg(sender), content, QSystemTrayIcon::Information, 2000);
        //raise();
    }
}

void MainWindow::on_test() {
    qDebug() << "test called";
    m_tray->showMessage("test1", "test2", QSystemTrayIcon::Warning, 10000);
}
