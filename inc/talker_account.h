#ifndef TALKERACCOUNT_H
#define TALKERACCOUNT_H

#include <QtGui>

class TalkerAccount : public QObject {
    Q_OBJECT
public:
    TalkerAccount(const QString &token, const QString &domain);
    TalkerAccount(QSettings &s);
    ~TalkerAccount();

    QString token() const {return m_token;}
    QString domain() const {return m_domain;}

    void set_token(const QString &token);
    void set_domain(const QString &domain);
    void open_room(const QString &room_name);
    void close_room(const QString &room_name);
    void save(QSettings &s);

private:
    QString m_token;
    QString m_domain;
    QStringList m_open_rooms;
    QDateTime m_last_used;

signals:
    void settings_changed(const TalkerAccount &acct);
};

#endif // TALKERACCOUNT_H
