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

#include <QObject>
#include <QtNetwork>
#include <QtScript>

#include "talker_account.h"

class TalkerRoom : public QObject {
    Q_OBJECT
public:
    TalkerRoom(TalkerAccount *acct, const QString &room_name,
               QObject *parent = 0);
    ~TalkerRoom() {}

    // create a socket to the given room and start chatting
    void join_room();

    public slots:
        void logout();
        void stay_alive();
        void socket_encrypted();
        void socket_ssl_errors(const QList<QSslError> &errors);
        void socket_ready_read();
        void socket_disconnected();

        void handle_message(const QScriptValue &val);

private:
    TalkerAccount *m_acct; // the account that has access to this room
    QString m_name; // the name of the room
    QSslSocket *m_ssl; // used for messages
    QScriptEngine *m_engine; // used to parse JSON we get from the SSL sockets
    QTimer *m_timer; // used for keep-alives

signals:

};

#endif // TALKERROOM_H
