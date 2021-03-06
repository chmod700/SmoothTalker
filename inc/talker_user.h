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
#ifndef TALKER_USER_H
#define TALKER_USER_H

#include <QObject>
#include <QIcon>

class QNetworkAccessManager;

class TalkerUser : public QObject
{
Q_OBJECT
public:
    explicit TalkerUser(const QString &name, const QString &email,
                        const int &id, QObject *parent = 0);
    virtual ~TalkerUser(){}

    QString name;
    QString email;
    int id;
    QIcon avatar;
    bool idle;

    bool valid() const {return id != -1;}

public slots:
    void request_avatar(QNetworkAccessManager *net); // get avatar from web
    void on_avatar_loaded(); // when an avatar is finished loading

private:
    bool avatar_requested;

signals:
    void updated(const TalkerUser*);
};

#endif // TALKER_USER_H
