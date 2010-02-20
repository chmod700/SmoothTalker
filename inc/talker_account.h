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
