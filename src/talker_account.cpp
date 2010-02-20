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
#include <QtScript>

#include "talker_account.h"
#include "ui_account_edit_dialog.h"

TalkerAccount::TalkerAccount(const QString &name, const QString &token,
                             const QString &domain, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_token(token)
    , m_domain(domain)
    , m_last_used(QDateTime::currentDateTime())
    , m_open_rooms(QStringList())
    , m_avail_rooms(QMap<QString, int>())
    , m_net(new QNetworkAccessManager(this))
    , m_ssl(new QSslSocket(this))
    , m_engine(new QScriptEngine(this))
{
    qDebug() << "Account ctor:" << token << domain;
}

TalkerAccount::~TalkerAccount() {}

void TalkerAccount::load_settings(QSettings &s) {
    m_name = s.value("name").toString();
    m_token = s.value("token").toString();
    m_domain = s.value("domain").toString();
    m_last_used = s.value("last_used").toDateTime();
    m_open_rooms = s.value("open_rooms").toStringList();
}

void TalkerAccount::set_name(const QString &name) {
    m_name = name;
    emit settings_changed(*this);
}

void TalkerAccount::set_token(const QString &token) {
    m_token = token;
    emit settings_changed(*this);
}

void TalkerAccount::set_domain(const QString &domain) {
    m_domain = domain;
    emit settings_changed(*this);
}

void TalkerAccount::save(QSettings &s) {
    s.setValue("name", m_name);
    s.setValue("token", m_token);
    s.setValue("domain", m_domain);
    s.setValue("last_used", m_last_used);
    s.setValue("open_rooms", m_open_rooms);
}

void TalkerAccount::logout() {
    qDebug() << to_str() << "logging out";
    if (m_ssl->isEncrypted() && m_ssl->isWritable()) {
        m_ssl->write("{\"type\":\"close\"}\r\n");
    }
    m_ssl->close();
}

void TalkerAccount::get_available_rooms() {
    // get rooms...
    QUrl url(QString("https://%1.talkerapp.com/rooms.json").arg(m_domain));

    QNetworkRequest req(url);
    req.setRawHeader(QByteArray("Accept"), QByteArray("application/json"));
    req.setHeader(QNetworkRequest::ContentTypeHeader,
                  QByteArray("application/json"));
    req.setRawHeader(QByteArray("X-Talker-Token"), m_token.toAscii());

    qDebug() << "sending request" << req.url();
    foreach(QByteArray hdr, req.rawHeaderList()) {
        qDebug() << "HEADER:" << hdr << ":" << req.rawHeader(hdr);
    }
    connect(m_net, SIGNAL(finished(QNetworkReply*)),
            SLOT(rooms_request_finished(QNetworkReply*)));
    m_net->get(req);
}

void TalkerAccount::rooms_request_finished(QNetworkReply *r) {
    m_net->disconnect(this, SLOT(rooms_request_finished(QNetworkReply*)));
    QString reply(r->readAll());

    qDebug() << "SERVER SAID:" << reply;
    if (r->error() == QNetworkReply::NoError) { // YAY!
        QScriptValue val = m_engine->evaluate(QString("(%1)").arg(reply));
        if (m_engine->hasUncaughtException()) {
            qWarning() << "SCRIPT EXCEPTION" << m_engine->uncaughtException().toString();
            QMessageBox::warning(NULL, tr("Communication Error!"),
                                 tr("Failed to parse response from server:\n\n%1")
                                 .arg(reply));
            logout();
            return;
        }
        qDebug() << "Evaluated response:" << val.toString();
        QString response_type = val.property("type").toString();

        if (val.isArray()) {
            QScriptValueIterator it(val);
            while(it.hasNext()) {
                it.next();
                QScriptValue room = it.value();
                QString room_name = room.property("name").toString();
                int room_id = room.property("id").toInteger();
                m_avail_rooms.insert(room_name, room_id);
            }

            /*m_room_to_join = QInputDialog::getItem(this, tr("Choose which room to join"),
                                                         tr("Select a room"), rooms.keys(), 0, false);
            qDebug() << "will join" << m_room_to_join << rooms.value(m_room_to_join);
            login();*/
        } else if (val.isObject() && response_type == "error") {
            QString msg = val.property("message").toString();
            qWarning() << "SERVER SENT ERROR:" << msg;
            if (msg == "Please login") {
                int want_to_edit = QMessageBox::question(
                        NULL, tr("Unauthorized Login"),
                        tr("The credentials you supplied for account '%1' are "
                           "invalid. Would you like to edit this account?")
                        .arg(m_name),
                        QMessageBox::Yes, QMessageBox::No
                );
                if (want_to_edit == QMessageBox::Yes && edit()) {
                    // try again
                    get_available_rooms();
                }
            } else {
                QMessageBox::warning(
                        NULL, tr("Server Error!"),
                        tr("Server sent the following error:\n\n%1").arg(msg)
                );
            }
        }
    } else {
        qWarning() << "ROOM REQUEST ERROR:" << r->errorString() << r->error();
    }

    //[{"name": "Main", "id": 497}, {"name": "Second Room", "id": 512}]
    r->deleteLater();
}

TalkerAccount* TalkerAccount::create_new(QObject *account_owner,
                                         QWidget *dialog_parent) {
    TalkerAccount *acct = new TalkerAccount("", "", "", account_owner);

    if (!acct->edit()) { // if they cancel delete the temp object we made
        acct->deleteLater();
        acct = NULL; // give back nothing
    }

    return acct;
}

bool TalkerAccount::edit() {
    QDialog *d = new QDialog(NULL);
    Ui::AccountEditDialog ui;
    ui.setupUi(d); // paint the dialog
    ui.le_name->setText(m_name);
    ui.le_token->setText(m_token);
    ui.le_domain->setText(m_domain);
    bool accepted = d->exec();
    if (accepted) { // only make an account if they accepted the dialog
        m_name = ui.le_name->text();
        m_token = ui.le_token->text();
        m_domain = ui.le_domain->text();
    }
    d->deleteLater();
    return accepted;
}
