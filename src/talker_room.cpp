/*
SmoothTalker
Copyright (c) 2010 Trey Stout (chmod)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <QtGui>
#include <QtNetwork>
#include <QtScript>

#include "talker_room.h"

TalkerRoom::TalkerRoom(TalkerAccount *acct, const QString &room_name,
                       const int id, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_acct(acct)
    , m_name(room_name)
    , m_ssl(new QSslSocket(this))
    , m_engine(new QScriptEngine(this))
    , m_timer(new QTimer(this))
    , m_chat(new QTableView(QObject::findChild<QMainWindow*>("MAINWINDOW")))
    , m_model(new QStandardItemModel(this))
{
    QSslConfiguration config = m_ssl->sslConfiguration();
    config.setProtocol(QSsl::TlsV1);
    m_ssl->setSslConfiguration(config);

    connect(m_ssl, SIGNAL(encrypted()), SLOT(socket_encrypted()));
    connect(m_ssl, SIGNAL(sslErrors(QList<QSslError>)),
            SLOT(socket_ssl_errors(QList<QSslError>)));
    connect(m_ssl, SIGNAL(readyRead()), SLOT(socket_ready_read()));
    connect(m_ssl, SIGNAL(disconnected()), SLOT(socket_disconnected()));
    connect(m_timer, SIGNAL(timeout()), SLOT(stay_alive()));

    m_chat->setModel(m_model);
    m_chat->horizontalHeader()->setStretchLastSection(true);
    m_model->setHeaderData(0, Qt::Horizontal, tr("User"));
    m_model->setHeaderData(1, Qt::Horizontal, tr("Message"));
}

void TalkerRoom::join_room() const {
    // open a connection
    m_ssl->connectToHostEncrypted("talkerapp.com", 8500);
}

void TalkerRoom::logout() {
    qDebug() << this << "logging out";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"close\"}\r\n");
    }
    m_ssl->disconnectFromHost();
}

void TalkerRoom::socket_encrypted() {
    // connection has been made successfully
    qDebug() << "socket encrypted";
    QString body("{\"type\":\"connect\",\"room\":\"%1\","
                 "\"token\":\"%2\"}\r\n");
    body = body.arg(m_name).arg(m_acct->token());
    //qDebug() << "sending" << body;
    m_ssl->write(body.toAscii());
    emit connected(this);

    // make our widget ready to rock...
    m_model->clear();
    m_chat->setStyleSheet("QTableWidget {border: 1px solid red;}");
}

void TalkerRoom::socket_ssl_errors(const QList<QSslError> &errors) {
    qWarning() << "SSL ERROR:" << errors;
}

void TalkerRoom::socket_disconnected() {
    emit disconnected(this);
    /*
    set_interface_enabled(false);
    m_status_lbl->setText(tr("Not Connected"));
    */
    m_timer->stop();
}

void TalkerRoom::socket_ready_read() {
    qDebug() << "socket is ready to read";

    // get the server's reply
    QString reply = QString(m_ssl->readAll()).trimmed();
    qDebug() << QString("server said: (%1)").arg(reply);


    QScriptValue val = m_engine->evaluate(QString("(%1)").arg(reply));
    if (m_engine->hasUncaughtException()) {
        qWarning() << "SCRIPT EXCEPTION" << m_engine->uncaughtException().toString();
        QMessageBox::warning(NULL, tr("Communication Error!"),
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

        // m_status_lbl->setText(tr("Connected as %1").arg(user));

        m_timer->setInterval(20000);
        m_timer->start();
        //ui->le_chat_entry->setFocus();
    } else if (response_type == "users") {
        // ui->tbl_users->clearContents();
        // ui->tbl_users->setRowCount(val.property("users").property("length").toInteger());
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
                //QTableWidgetItem *name_item = new QTableWidgetItem(user_name);
                //QTableWidgetItem *email_item = new QTableWidgetItem(user_email);
                //QTableWidgetItem *email_item = new QTableWidgetItem("user@email.com");
                // ui->tbl_users->setItem(i, 0, name_item);
                // ui->tbl_users->setItem(i, 1, email_item);
                i++;
            }
        }
    } else if (response_type == "error") {
        QString msg = val.property("message").toString();
        qWarning() << "SERVER SENT ERROR:" << msg;
        QMessageBox::warning(NULL, tr("Server Error!"),
                             tr("Server sent the following error:\n\n%1")
                             .arg(msg));
    } else if (response_type == "message") {
        handle_message(val);
    }
}

void TalkerRoom::stay_alive() {
    qDebug() << "pinging...";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"ping\"}\r\n");
    }
}

void TalkerRoom::handle_message(const QScriptValue &val) {
    QDateTime timestamp = val.property("time").toDateTime();
    QString content = val.property("content").toString();
    QString sender = val.property("user").property("name").toString();

    qDebug() << "got message from:" << sender << "MSG:" << content;
    QStandardItem *i_sender = new QStandardItem(sender);
    QStandardItem *i_content = new QStandardItem(content);
    m_model->appendRow(QList<QStandardItem*>() << i_sender << i_content);

    //entry->setText(1, QString("TIME:[%1]\n%2").arg(timestamp.toLocalTime().toString("MMM d h:m:s ap")).arg(content));
    m_chat->scrollToBottom();

    /*if (!isActiveWindow()) {
        m_tray->showMessage(QString("message from %1").arg(sender), content, QSystemTrayIcon::Information, 2000);
        //raise();
    }*/
    //emit message_received(sender, content, this);
}

QDebug operator<<(QDebug dbg, const TalkerRoom &r) {
    dbg.nospace() << "<ROOM:" << r.name() << ">";
    return dbg.space();
}
