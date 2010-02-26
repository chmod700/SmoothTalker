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

#include "main_window.h" // to get settings
#include "talker_room.h"

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
    , m_chat(new QTableView(0))
    , m_model(new QStandardItemModel(this))
    , m_users(QMap<int, TalkerUser*>())
    , m_avatar_requests(QMap<QString, TalkerUser*>())
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
    connect(m_net, SIGNAL(finished(QNetworkReply*)),
            SLOT(on_avatar_loaded(QNetworkReply*)));

    m_chat->horizontalHeader()->setStretchLastSection(true);
    m_chat->horizontalHeader()->show();
    m_chat->verticalHeader()->hide();
    m_chat->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_chat->setWordWrap(true);
    m_chat->setShowGrid(false);
    m_chat->setAlternatingRowColors(true);
    m_chat->setIconSize(QSize(24, 24));
    m_chat->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_chat->setStyleSheet("QTableView {border: 0px;}");
    m_chat->setModel(m_model);
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

    // make our widget ready to rock...
    m_model->clear();
    QStringList labels;
    labels << tr("Time") << tr("User") << tr("Message");
    m_model->setHorizontalHeaderLabels(labels);
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
    qDebug() << QString("server said: (%1)").arg(reply);


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
    if (!u) {
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
    content = content.replace("&lt;", "<", Qt::CaseInsensitive);
    content = content.replace("&gt;", ">", Qt::CaseInsensitive);
    content = content.replace("&quot;", "\"", Qt::CaseInsensitive);
    content = content.replace("<br/>", "\n", Qt::CaseSensitive);

    QDateTime timestamp;
    timestamp.setTime_t(time);

    qDebug() << "got message from:" << m_users[sender_id]->name
            << "MSG:" << content;

    // is this another message from the same user who sent the last message?
    QModelIndex last_msg = m_model->index(m_model->rowCount()-1, 1);
    bool append_mode = false;
    int last_sender_id = -1;
    if (last_msg.isValid()) {
        last_sender_id = m_model->data(last_msg, Qt::UserRole).toInt();
        if (sender_id == last_sender_id) {
            append_mode = true;
        }
    }

    if (append_mode) {
        QStandardItem *last_msg = m_model->item(m_model->rowCount()-1, 2);
        last_msg->setText(QString("%1\n%2").arg(last_msg->text()).arg(content));
    } else {
        QStandardItem *i_sender = new QStandardItem(u->name);
        i_sender->setData(u->id, Qt::UserRole);
        if (!u->avatar.isNull()) {
            i_sender->setIcon(u->avatar);
        }
        QStandardItem *i_content = new QStandardItem(content);
        i_content->setData(val.property("id").toString(), Qt::UserRole);
        QStandardItem *i_time = new QStandardItem(timestamp.toString("h:mmap"));
        m_model->appendRow(QList<QStandardItem*>() << i_time << i_sender
                           << i_content);

        i_time->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        i_sender->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
        i_content->setTextAlignment(Qt::AlignLeft | Qt::AlignTop);
    }
    m_chat->resizeColumnToContents(0);
    m_chat->resizeColumnToContents(1);
    m_chat->resizeRowsToContents();
    m_chat->scrollToBottom();

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
        qDebug() << "\tSENDING:" << body.toAscii();
        m_ssl->write(body.toAscii());
    } else {
        qWarning() << "tried to submit message to non-opened socket." << this;
    }
}

void TalkerRoom::on_avatar_loaded(QNetworkReply *r) {
    if (!r || !m_avatar_requests.contains(r->url().toString())) {
        qWarning() << "unknown avatar request completed...";
        return;
    }
    TalkerUser *u = m_avatar_requests.take(r->url().toString());
    if (u) {
        /*
        qDebug() << "avatar request for user" << u->name << "completed!";
        foreach(QByteArray header, r->rawHeaderList()) {
            qDebug() << "\t" << header << ":" << r->rawHeader(header);
        }
        */

        QImage img = QImage::fromData(r->readAll(), r->rawHeader("Content-Type"));
        QPixmap avatar = QPixmap::fromImage(img);
        if (!avatar.isNull()) {
            u->avatar = QIcon(avatar);
            emit user_updated(this, u);
        }
    } else {
        qWarning() << "avatar request for user was lost!"
                << r->url().toString();
    }
    r->deleteLater();
}

void TalkerRoom::on_options_changed(QSettings *s) {
    m_chat->setColumnHidden(0, !s->value("options/show_timestamps", true)
                            .toBool());
}

TalkerUser *TalkerRoom::add_user(const QScriptValue &user) {
    int user_id = user.property("id").toInteger();
    if (m_users[user_id]) {
        return m_users[user_id];
    }

    TalkerUser *u = new TalkerUser();
    u->name = user.property("name").toString().trimmed();
    u->email = user.property("email").toString().trimmed();
    u->id = user_id;
    u->idle = false;
    m_users[u->id] = u;

    qDebug() << "new user" << u->name << "email" << u->email << "id" << u->id;

    QCryptographicHash email_hash(QCryptographicHash::Md5);
    email_hash.addData(u->email.toAscii());
    QString hash(email_hash.result().toHex());
    QString size("48"); // TODO: move this to options
    //QString default_type("identicon"); // TODO: move this to options
    QString default_type("wavatar"); // TODO: move this to options
    QUrl img_url(QString("http://www.gravatar.com/avatar/%1?s=%2&d=%3")
                 .arg(hash)
                 .arg(size)
                 .arg(default_type));
    qDebug() << "requesting image for" << u->name << "from"
            << img_url.toString();
    QNetworkRequest req(img_url);
    QNetworkReply *r = m_net->get(req);
    Q_UNUSED(r);
    m_avatar_requests.insert(img_url.toString(), u);
    return u;
}

void TalkerRoom::system_message(const QString &time, const QString &message) {
    QStandardItem *i_time = new QStandardItem(time);
    QStandardItem *i_icon = new QStandardItem(
            QIcon(":img/icons/information.png"), "");
    QStandardItem *i_msg = new QStandardItem(message);
    i_time->setForeground(QBrush(Qt::gray));
    i_msg->setForeground(QBrush(Qt::gray));
    m_model->appendRow(QList<QStandardItem*>() << i_time << i_icon << i_msg);
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
