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
#include <QtNetwork>
#include "talker_user.h"

TalkerUser::TalkerUser(const QString &name, const QString &email,
                       const int &id, QObject *parent)
    : QObject(parent)
    , name(name)
    , email(email)
    , id(id)
    , avatar(QIcon())
    , idle(false)
    , avatar_requested(false)
    , m_avatar_url(QUrl())
{
    qDebug() << "new user" << name << "email" << email << "id" << id;
}

QUrl TalkerUser::avatar_url(const QString &size) {
    if (!m_avatar_url.isValid()) {
        QCryptographicHash email_hash(QCryptographicHash::Md5);
        email_hash.addData(email.toAscii());
        QString hash(email_hash.result().toHex());

        // optional settings for avatar
        //QString default_type("identicon"); // TODO: move this to options
        QString default_type("wavatar"); // TODO: move this to options

        // TODO: move avatar url to defines...
        m_avatar_url = QUrl(
                QString("http://www.gravatar.com/avatar/%1?s=%2&d=%3")
                    .arg(hash).arg(size).arg(default_type));
    }
    return m_avatar_url;
}

void TalkerUser::request_avatar(QNetworkAccessManager *net) {
    if (avatar_requested) // we already sent a request...
        return;
    QUrl url = avatar_url();
    qDebug() << "requesting image for" << name << "from" << url.toString();
    QNetworkRequest req(url);
    QNetworkReply *r = net->get(req);
    connect(r, SIGNAL(finished()), SLOT(on_avatar_loaded()));
    avatar_requested = true;
}

void TalkerUser::on_avatar_loaded() {
    QNetworkReply *r = qobject_cast<QNetworkReply*>(QObject::sender());
    if (!r) {
        qWarning() << "ERROR: avatar request completed, but we lost the "
                << "response";
    } else if (r->error()) {
        qWarning() << "ERROR: avatar request for" << name << r->errorString();
    } else {
        qDebug() << "AVATAR: request for user" << name << "completed!";
        /*
        foreach(QByteArray header, r->rawHeaderList()) {
            qDebug() << "\t" << header << ":" << r->rawHeader(header);
        }
        */
        QPixmap av = QPixmap::fromImage(
                QImage::fromData(r->readAll(), r->rawHeader("Content-Type")));
        if (!av.isNull()) {
            avatar = av;
            emit updated(this);
        }
    }
    if (r)
        r->deleteLater();
}
