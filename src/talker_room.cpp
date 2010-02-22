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
    , m_users(QMap<int, TalkerUser>())
{
    QSslConfiguration config = m_ssl->sslConfiguration();
    config.setProtocol(QSsl::TlsV1);
    m_ssl->setSslConfiguration(config);

    connect(m_ssl, SIGNAL(encrypted()), SLOT(socket_encrypted()));
    connect(m_ssl, SIGNAL(sslErrors(QList<QSslError>)),
            SLOT(socket_ssl_errors(QList<QSslError>)));
    connect(m_ssl, SIGNAL(readyRead()), SLOT(socket_ready_read()));
    connect(m_ssl, SIGNAL(disconnected()), SLOT(socket_disconnected()));
    connect(m_ssl, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
            SLOT(socket_state_changed(QAbstractSocket::SocketState)));
    connect(m_timer, SIGNAL(timeout()), SLOT(stay_alive()));

    m_chat->setModel(m_model);
    m_chat->horizontalHeader()->setStretchLastSection(true);
    m_chat->horizontalHeader()->hide();
    m_chat->verticalHeader()->hide();
    m_chat->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_chat->setWordWrap(true);
    m_chat->setShowGrid(false);
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
        qDebug() << this << "sent close command";
    }
    m_ssl->flush();
    qDebug() << this << "flushed";
    m_ssl->disconnectFromHost();
    //m_ssl->abort();
}

void TalkerRoom::socket_encrypted() {
    // connection has been made successfully
    //qDebug() << "socket encrypted";
    QString body("{\"type\":\"connect\",\"room\":\"%1\","
                 "\"token\":\"%2\"}\r\n");
    body = body.arg(m_name).arg(m_acct->token());
    m_ssl->write(body.toAscii());
    emit connected(this);

    // make our widget ready to rock...
    m_model->clear();
    m_chat->setStyleSheet("QTableWidget {border: 1px solid red;}");
}

void TalkerRoom::socket_ssl_errors(const QList<QSslError> &errors) {
    qWarning() << "\tSSL ERROR:" << errors;
}

void TalkerRoom::socket_state_changed(QAbstractSocket::SocketState state) {
    qDebug() << "\tRoom:" << m_name << "socket state changed to:" << state;
}

void TalkerRoom::socket_disconnected() {
    qDebug() << this << "DISCONNECTED";
    emit disconnected(this);
    m_timer->stop();
    //m_ssl->deleteLater();
    //m_ssl = 0;
}

void TalkerRoom::socket_ready_read() {
    // get the server's reply
    QString reply = QString(m_ssl->readAll()).trimmed();
    //qDebug() << QString("server said: (%1)").arg(reply);


    QScriptValue val = m_engine->evaluate(QString("(%1)").arg(reply));
    if (m_engine->hasUncaughtException()) {
        qWarning() << "SCRIPT EXCEPTION" << m_engine->uncaughtException().toString();
        QMessageBox::warning(NULL, tr("Communication Error!"),
                             tr("Failed to parse response from server:\n\n%1")
                             .arg(reply));
        logout();
        return;
    }
    //qDebug() << "Evaluated response:" << val.toString();

    QString response_type = val.property("type").toString();
    //qDebug() << "RESPONSE DISPATCH:" << response_type;
    if (response_type == "connected") {
        QString user = val.property("user").property("name").toString();
        qDebug() << "user is:" << user;
        m_timer->setInterval(20000);
        m_timer->start();
        // m_status_lbl->setText(tr("Connected as %1").arg(user));
    } else if (response_type == "users") {
        m_users.clear();
        QScriptValue users_obj = val.property("users");
        /*
        {"type":"users","users":[{"name":"LODE","id":1617,"email":"khermann@gmail.com"},
                                 {"name":"Mochnant","id":2207,"email":null},
                                 {"name":"chmod","id":1615,"email":"treystout@gmail.com"}],
         "id":"da975c70021c012de69812313d01d943"}
        */
        if (users_obj.isArray()) {
            QScriptValueIterator it(users_obj);
            int i = 0;
            while(it.hasNext()) {
                it.next();
                QScriptValue user = it.value();
                TalkerUser u;
                u.name = user.property("name").toString().trimmed();
                u.email = user.property("email").toString().trimmed();
                u.id = user.property("id").toInteger();
                m_users[u.id] = u;
                //qDebug() << "user in room" << u.name << "email" << u.email << "id" << u.id;
                i++;
            }
            emit users_updated(this);
        }
    } else if (response_type == "error") {
        QString msg = val.property("message").toString();
        qWarning() << "SERVER SENT ERROR:" << msg;
        QMessageBox::warning(NULL, tr("Server Error!"),
                             tr("Server sent the following error:\n\n%1")
                             .arg(msg));
    } else if (response_type == "message") {
        handle_message(val);
    } else {
        qDebug() << "unhandled message type" << response_type;
    }
}

void TalkerRoom::stay_alive() {
    //qDebug() << "pinging...";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"ping\"}\r\n");
    }
}

void TalkerRoom::handle_message(const QScriptValue &val) {
    int time = val.property("time").toInt32();
    QString content = val.property("content").toString();
    QString sender = val.property("user").property("name").toString();
    QDateTime timestamp;
    timestamp.setTime_t(time);

    //qDebug() << "got message from:" << sender << "MSG:" << content;
    QStandardItem *i_sender = new QStandardItem(sender);
    QStandardItem *i_content = new QStandardItem(content);
    QStandardItem *i_time = new QStandardItem(timestamp.toString("h:mmap"));
    m_model->appendRow(QList<QStandardItem*>() << i_time << i_sender << i_content);

    m_chat->resizeRowsToContents();
    m_chat->scrollToBottom();

    emit message_received(sender, content, this);
}

void TalkerRoom::submit_message(const QString &msg) {
    if (m_ssl && m_ssl->isEncrypted()) {
        QString body("{\"type\":\"message\",\"content\":\"%1\"}\r\n");
        m_ssl->write(body.arg(msg).toAscii());
    } else {
        qWarning() << "tried to submit message to non-opened socket." << this;
    }
}

QDebug operator<<(QDebug dbg, const TalkerRoom &r) {
    dbg.nospace() << "<ROOM:" << r.name() << ">";
    return dbg.space();
}
