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
#ifndef TALKERROOM_H
#define TALKERROOM_H

#include <QtGui>
#include <QtNetwork>
#include <QtScript>

#include "talker_account.h"

struct TalkerUser {
    QString name;
    QString email;
    int id;
    QIcon avatar;
    bool idle;
};

class TalkerRoom : public QObject {
    Q_OBJECT
public:
    TalkerRoom(TalkerAccount *acct, const QString &room_name, const int id,
               QObject *parent = 0);
    ~TalkerRoom();

    int id() const {return m_id;}
    QString name() const {return m_name;}
    // create a socket to the given room and start chatting
    void join_room() const;
    QWidget *get_widget() const {return m_chat;}
    QMap<int, TalkerUser*> get_users() const {return m_users;}

    public slots:
        void logout();
        void stay_alive();
        void socket_encrypted();
        void socket_ssl_errors(const QList<QSslError> &errors);
        void socket_ready_read();
        void socket_disconnected();
        void socket_state_changed(QAbstractSocket::SocketState);

        void handle_users(const QScriptValue &val);
        void handle_message(const QScriptValue &val);
        void handle_idle(const QScriptValue &val);
        void handle_back(const QScriptValue &val);
        void handle_join(const QScriptValue &val);
        void handle_leave(const QScriptValue &val);
        void submit_message(const QString &msg);

        void on_avatar_loaded(QNetworkReply*);
        void on_options_changed(QSettings*);

        void system_message(const QString &time, const QString &message);

private:
    int m_id; // id of the room
    int m_user_id; // our user id we logged in with
    TalkerAccount *m_acct; // the account that has access to this room
    QString m_name; // the name of the room
    QSslSocket *m_ssl; // used for messages
    QNetworkAccessManager *m_net; // handles web requests for us
    QScriptEngine *m_engine; // used to parse JSON we get from the SSL sockets
    QTimer *m_timer; // used for keep-alives
    QTableView *m_chat; // shows messages
    QStandardItemModel *m_model; // stores messages
    QMap<int, TalkerUser*> m_users; // holds records of who is in room
    QMap<QNetworkReply*, TalkerUser*> m_avatar_requests; // keep track of
        // web requests for avatars

    TalkerUser *add_user(const QScriptValue &user);
    QDateTime time_from_message(const QScriptValue &val);

signals:
    void connected(const TalkerRoom *room);
    void disconnected(TalkerRoom *room);
    void message_received(const QString &sender, const QString &content, const TalkerRoom *room);
    void users_updated(const TalkerRoom *room);
    void user_updated(const TalkerRoom *room, const TalkerUser *user);
};

#endif // TALKERROOM_H
