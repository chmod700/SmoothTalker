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
#include "talker_account.h"

TalkerAccount::TalkerAccount(const QString &token, const QString &domain)
    : m_token(token)
    , m_domain(domain)
{
    qDebug() << "Account ctor:" << token << domain;
}

TalkerAccount::TalkerAccount(QSettings &s) {
    m_token = s.value("token").toString();
    m_domain = s.value("domain").toString();
    m_last_used = s.value("last_used").toDateTime();
    m_open_rooms = s.value("open_rooms").toStringList();
}

TalkerAccount::~TalkerAccount() {}

void TalkerAccount::set_token(const QString &token) {
    m_token = token;
    emit settings_changed(*this);
}

void TalkerAccount::set_domain(const QString &domain) {
    m_domain = domain;
    emit settings_changed(*this);
}

void TalkerAccount::save(QSettings &s) {
    s.setValue("token", m_token);
    s.setValue("domain", m_domain);
    s.setValue("last_used", m_last_used);
    s.setValue("open_rooms", m_open_rooms);
}
