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
#include <QtWebKit>

#include "main_window.h" // to get settings
#include "talker_room.h"
#include "talker_user.h"

TalkerRoom::TalkerRoom(TalkerAccount *acct, const QString &room_name,
                       const int id, QObject *parent)
    : QObject(parent)
    , m_id(id)
    , m_user_id(0)
    , m_last_event_id(QString())
    , m_acct(acct)
    , m_name(room_name)
    , m_ssl(new QSslSocket(this))
    , m_net(new QNetworkAccessManager(this))
    , m_engine(new QScriptEngine(this))
    , m_timer(new QTimer(this))
    , m_chat(new QWebView(0))
    , m_html(new QTemporaryFile(this))
    , m_users(QMap<int, TalkerUser*>())
{
    m_users.clear();

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

    m_html->open();
    m_html->write(QString("<table>").toUtf8());
    m_chat->setStyleSheet("div {border: 1px solid red;}");

    load();
}

TalkerRoom::~TalkerRoom() {
    foreach(TalkerUser *u, m_users.values()) {
        delete u;
    }
    m_users.clear();
}

void TalkerRoom::save() {
    QSettings *s = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                 QCoreApplication::organizationName(),
                                 QCoreApplication::applicationName(), this);
    s->beginGroup(QString("room_%1").arg(m_id));
    s->setValue("id", m_id);
    s->setValue("name", m_name);
    s->setValue("last_event_id", m_last_event_id);
    s->endGroup();
    delete s;
}

void TalkerRoom::load() {
    QSettings *s = new QSettings(QSettings::IniFormat, QSettings::UserScope,
                                 QCoreApplication::organizationName(),
                                 QCoreApplication::applicationName(), this);
    s->beginGroup(QString("room_%1").arg(m_id));
    m_last_event_id = s->value("last_event_id").toString();
    s->endGroup();
    delete s;
}

void TalkerRoom::join_room() const {
    // open a connection
    status_message(tr("connecting to server..."));
    m_ssl->connectToHostEncrypted("talkerapp.com", 8500);
}

void TalkerRoom::logout() {
    /*
      Stop sending the close message as this caused all clients logged into
      this account to also log out.

      See: https://talker.tenderapp.com/discussions/problems/33-bug-close-message-disconnects-all-clients-from-a-room
    qDebug() << this << "logging out";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"close\"}\r\n");
        qDebug() << this << "sent close command";
    }
    */
    save();
    status_message(tr("disconnecting from server..."));
    m_ssl->flush();
    m_ssl->disconnectFromHost();
}

void TalkerRoom::socket_encrypted() {
    // connection has been made successfully
    qDebug() << "socket encrypted";
    status_message(tr("connection encrypted. logging in..."));

    QString body;
    if (!m_last_event_id.isEmpty()) {
        qDebug() << "\tUSING LAST EVENT ID" << m_last_event_id;
        body = "{\"type\":\"connect\","
               "\"room\":\"%1\","
               "\"token\":\"%2\","
               "\"last_event_id\":\"%3\"}\r\n";
        body = body.arg(m_name).arg(m_acct->token()).arg(m_last_event_id);
    } else {
        body = "{\"type\":\"connect\","
               "\"room\":\"%1\","
                "\"token\":\"%2\"}\r\n";
        body = body.arg(m_name).arg(m_acct->token());
    }
    m_ssl->write(body.toAscii());
    emit connected(this);
}

void TalkerRoom::socket_ssl_errors(const QList<QSslError> &errors) {
    qWarning() << "\tSSL ERROR:" << errors;
}

void TalkerRoom::socket_state_changed(QAbstractSocket::SocketState state) {
    //qDebug() << "\tRoom:" << m_name << "socket state changed to:" << state;
}

void TalkerRoom::socket_disconnected() {
    qDebug() << this << "DISCONNECTED";
    status_message(tr("disconnected from server"));
    emit disconnected(this);
    m_timer->stop();
    save();
}

void TalkerRoom::socket_ready_read() {
    // get the server's reply
    QString reply = QString(m_ssl->readAll()).trimmed();
    //qDebug() << QString("server said: (%1)").arg(reply);


    QScriptValue val = m_engine->evaluate(QString("(%1)").arg(reply));
    if (m_engine->hasUncaughtException()) {
        qWarning() << "SCRIPT EXCEPTION"
                << m_engine->uncaughtException().toString();
        QMessageBox::warning(NULL, tr("Communication Error!"),
                             tr("Failed to parse response from server:\n\n%1")
                             .arg(reply));
        logout();
        return;
    }

    QString response_type = val.property("type").toString();
    if (!val.property("id").isNull()) {
        m_last_event_id = val.property("id").toString();
    }
    //qDebug() << "RESPONSE DISPATCH:" << response_type;
    if (response_type == "connected") {
        m_user_id = val.property("user").property("id").toInteger();
        // start the keep-alive timer
        m_timer->setInterval(20000);
        m_timer->start();
        //QString user = val.property("user").property("name").toString();
        //qDebug() << "user is:" << user;
        //m_status_lbl->setText(tr("Connected as %1").arg(user));
        status_message(tr("connected as %1").arg(val.property("user")
                                                 .property("name").toString()));
    } else if (response_type == "users") {
        handle_users(val);
    } else if (response_type == "message") {
        handle_message(val);
    } else if (response_type == "idle") {
        handle_idle(val);
    } else if (response_type == "back") {
        handle_back(val);
    } else if (response_type == "join") {
        handle_join(val);
    } else if (response_type == "leave") {
        handle_leave(val);
    } else if (response_type == "error") {
        QString msg = val.property("message").toString();
        qWarning() << "SERVER SENT ERROR:" << msg;
        QMessageBox::warning(NULL, tr("Server Error!"),
                             tr("Server sent the following error:\n\n%1")
                             .arg(msg));
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

void TalkerRoom::handle_users(const QScriptValue &val) {
    // wipe our internal list
    foreach(TalkerUser *u, m_users.values()) {
        delete u;
    }
    m_users.clear();

    QScriptValue users_obj = val.property("users");
    if (users_obj.isArray()) {
        QScriptValueIterator it(users_obj);
        int i = 0;
        while(it.hasNext()) {
            it.next();
            QScriptValue user = it.value();
            add_user(user);
            i++;
        }
        emit users_updated(this);
    }
}

void TalkerRoom::handle_message(const QScriptValue &val) {
    int sender_id = val.property("user").property("id").toInteger();

    TalkerUser *u = m_users[sender_id];
    if (!u || !u->valid()) {
        // chances are the server is catching us up on a big list of messages
        // we missed, and we haven't gotten the user list for this room yet
        // add this unknown user and then handle the message again...
        qWarning() << "received message from unknown user with id:"
                << sender_id << "name:"
                << val.property("user").property("name").toString();
        add_user(val.property("user"));
        handle_message(val);
        return;
    }

    int time = val.property("time").toInt32();
    QString content = val.property("content").toString();
    /*
    content = content.replace("&lt;", "<", Qt::CaseInsensitive);
    content = content.replace("&gt;", ">", Qt::CaseInsensitive);
    content = content.replace("&quot;", "\"", Qt::CaseInsensitive);
    content = content.replace("<br/>", "\n", Qt::CaseSensitive);

    QDateTime timestamp;
    timestamp.setTime_t(time);
    */

    //qDebug() << "got message from:" << m_users[sender_id]->name
    //        << "MSG:" << content;

    //timestamp.toString("h:mmap")

    QFile f(":templates/message.txt");
    f.open(QFile::ReadOnly);
    QString temp(f.readAll());
    f.close();

    m_html->seek(m_html->size());
    content = temp.arg(m_last_event_id,
                       u->avatar_url().toString(),
                       u->name, content);
    m_html->write(content.toUtf8());
    m_html->flush();

    m_html->seek(0);
    content = QString(m_html->readAll());
    m_chat->setHtml(content);
    m_chat->page()->mainFrame()->setScrollBarValue(
            Qt::Vertical,
            m_chat->page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
    emit message_received(u->name, content, this);
}

void TalkerRoom::handle_idle(const QScriptValue &val) {
    int user_id = val.property("user").property("id").toInteger();
    QDateTime timestamp = time_from_message(val);
    TalkerUser *u = m_users[user_id];
    if (u) {
        qDebug() << "user" << u->id << u->name << "has gone idle";
        if (user_id == m_user_id) {
            return; // ignore these messages for ourselves
        }
        /*system_message(timestamp.toString("h:mmap"),
                       QString("%1 is now idle").arg(u->name));*/
        u->idle = true;
        emit user_updated(this, u);
    } else {
        qWarning() << "got idle event for unknown user id" << user_id;
    }
}

void TalkerRoom::handle_back(const QScriptValue &val) {
    int user_id = val.property("user").property("id").toInteger();
    QDateTime timestamp = time_from_message(val);
    TalkerUser *u = m_users[user_id];
    if (u) {
        qDebug() << "user" << u->id << u->name << "has come back";
        if (user_id == m_user_id) {
            return; // ignore these messages for ourselves
        }
        /*system_message(timestamp.toString("h:mmap"),
                       QString("%1 is now back").arg(u->name));*/
        u->idle = false;
        emit user_updated(this, u);
    } else {
        qWarning() << "got back event for unknown user id" << user_id;
    }
}

void TalkerRoom::handle_join(const QScriptValue &val) {
    QDateTime timestamp = time_from_message(val);
    TalkerUser *u = add_user(val.property("user"));
    if (u) {
        qDebug() << "user" << u->id << u->name << "joined room" << this;
        if (u->id == m_user_id) {
            return; // ignore these messages for ourselves
        }
        system_message(timestamp.toString("h:mmap"),
                       QString("%1 has joined the room").arg(u->name));
        emit user_updated(this, u);
    } else {
        qWarning() << "got join event and had trouble adding the user";
    }
}

void TalkerRoom::handle_leave(const QScriptValue &val) {
    int user_id = val.property("user").property("id").toInteger();
    QDateTime timestamp = time_from_message(val);
    TalkerUser *u = m_users.take(user_id);
    if (u) {
        qDebug() << "user left room" << u->id << u->name;
        system_message(timestamp.toString("h:mmap"),
                       QString("%1 has left the room").arg(u->name));
        delete u;
        emit users_updated(this);
    } else {
        qWarning() << "got leave event for unknown user_id" << user_id;
    }
}

void TalkerRoom::submit_message(const QString &msg) {
    if (msg.isEmpty()) {
        return; // don't send blank messages
    }
    if (m_ssl && m_ssl->isEncrypted()) {
        QString encoded = Qt::escape(msg);
        encoded = encoded.replace("\r\n", "<br/>", Qt::CaseSensitive);
        encoded = encoded.replace("\n", "<br/>", Qt::CaseSensitive);
        encoded = encoded.replace("\r", "<br/>", Qt::CaseSensitive);
        QString body = QString("{\"type\":\"message\",\"content\":\"%1\"}\r\n")
                       .arg(encoded);
        //qDebug() << "\tSENDING:" << body.toAscii();
        m_ssl->write(body.toAscii());
    } else {
        qWarning() << "tried to submit message to non-opened socket." << this;
    }
}

void TalkerRoom::on_options_changed(QSettings *s) {
    /*m_chat->setColumnHidden(0, !s->value("options/show_timestamps", true)
                            .toBool());*/
}

void TalkerRoom::on_user_updated(const TalkerUser *user) {
    emit user_updated(this, user);
}

TalkerUser *TalkerRoom::add_user(const QScriptValue &user) {
    int user_id = user.property("id").toInteger();
    if (m_users[user_id]) {
        return m_users[user_id];
    }

    TalkerUser *u = new TalkerUser(user.property("name").toString().trimmed(),
                                   user.property("email").toString().trimmed(),
                                   user_id, this);
    m_users[u->id] = u;
    u->request_avatar(m_net);
    connect(u, SIGNAL(updated(const TalkerUser*)),
            SLOT(on_user_updated(const TalkerUser*)));
    return u;
}

void TalkerRoom::system_message(const QString &time, const QString &message) {
    QStandardItem *i_time = new QStandardItem(time);
    QStandardItem *i_icon = new QStandardItem(
            QIcon(":img/icons/information.png"), "");
    QStandardItem *i_msg = new QStandardItem(message);
    i_time->setForeground(QBrush(Qt::gray));
    i_msg->setForeground(QBrush(Qt::gray));
    m_html->write(QString("<div style=\"color: #999999;\">%1</div>\n")
                  .arg(message).toUtf8());
}

QDateTime TalkerRoom::time_from_message(const QScriptValue &val) {
    int time = val.property("time").toInt32();
    QDateTime retval;
    retval.setTime_t(time);
    return retval;
}

void TalkerRoom::status_message(const QString &msg) const {
    QString text = tr("%1: %2").arg(m_name).arg(msg);
    emit new_status_message(text);
}

QDebug operator<<(QDebug dbg, const TalkerRoom &r) {
    dbg.nospace() << "<ROOM:" << r.name() << ">";
    return dbg.space();
}
