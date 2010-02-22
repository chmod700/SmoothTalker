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
#ifndef TALKERACCOUNT_H
#define TALKERACCOUNT_H

#include <QtGui>
#include <QtNetwork>
#include <QtScript>
// forward declarations

class TalkerRoom;

class TalkerAccount : public QObject {
    Q_OBJECT
public:
    TalkerAccount(const QString &name, const QString &token,
                  const QString &domain, QObject *parent);

    ~TalkerAccount();

    void load_settings(QSettings &s);
    QString to_str() const {
        return QString("<ACCT: %1 %2.talkerapp.com>").arg(m_name).arg(m_domain);
    }
    QString name() const {return m_name;}
    QString token() const {return m_token;}
    QString domain() const {return m_domain;}
    QMap<QString, int> avail_rooms() const {return m_avail_rooms;}

    void set_name(const QString &name);
    void set_token(const QString &token);
    void set_domain(const QString &domain);
    void open_room(const int room_id);
    void close_room(const int room_id);
    void save(QSettings &s);

    /**
      * Static method that shows a configuration dialog for a user-account token
      * as well as the talkerapp.com subdomain the user wants to connect to.
      * After the dialog is accepted the new account object is created on the
      * heap and passed back to the caller.
      */
    static TalkerAccount* create_new(QObject *account_owner,
                                     QWidget *dialog_parent=0);

    public slots:
        // open a connection to this account on talkerapp.com
        void logout();
        void get_available_rooms(); // send a request for the list of rooms
        bool edit();

private:
    QString m_name; // string used to identify this account
    QString m_token; // api token (32char hex)
    QString m_domain; // the first part of XXX.talkerapp.com
    QDateTime m_last_used; // last time this account logged in
    QMap<QString, QVariant> m_open_rooms; // which rooms (ids) were open on this
                                          // account last
    QMap<QString, int> m_avail_rooms; // which rooms can be joined (name, id)
    QList<TalkerRoom*> m_active_rooms; // rooms we're connected to
    QNetworkAccessManager *m_net; // used to for web requests
    QScriptEngine *m_engine; // used to parse JSON we get from the SSL sockets

    void setup_network(); // make the object we need to list rooms, and chat

    private slots:
        void rooms_request_finished(QNetworkReply *r);
        void on_room_connected(const TalkerRoom *room);
        void on_room_disconnected(TalkerRoom *room);

signals:
    void settings_changed(const TalkerAccount &acct);
    void new_rooms_available(const TalkerAccount &acct);
    void room_connected(const TalkerRoom *room);
    void room_disconnected(const TalkerRoom *room);
};

#endif // TALKERACCOUNT_H
