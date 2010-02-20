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
