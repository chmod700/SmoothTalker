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
#include "main_window.h"
#include "ui_main_window.h"
#include "ui_account_edit_dialog.h"
#include "talker_account.h"
#include <QtGui>
#include <QtNetwork>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(new QSettings(QSettings::IniFormat, QSettings::UserScope,
                QCoreApplication::organizationName(),
                QCoreApplication::applicationName(), this))
    , m_status_lbl(new QLabel(this))
    , m_tray_menu(new QMenu(this))
    , m_tray(new QSystemTrayIcon(this))
{
    // load up our pretty design
    ui->setupUi(this);

    // turn off the toolbar
    ui->toolBar->hide();

    // make sure we have SSL access, or the whole app is worthless
    if (!QSslSocket::supportsSsl()) {
        QMessageBox::warning(this, tr("Secure Sockets Library Not Found!"),
                             tr("This tool requires an SSL library with TLSv1 "
                                "support. And one was not found on your system")
                             );
        close();
    }

    statusBar()->addPermanentWidget(m_status_lbl, 1);
    m_status_lbl->setText(tr("Not Connected"));

    //setup system tray icon
    m_tray_menu->addAction(QIcon(":img/icons/door_out.png"), tr("E&xit"),
                           this, SLOT(close()));
    m_tray->setContextMenu(m_tray_menu);
    m_tray->setIcon(QIcon(":img/icons/transmit.png"));
    m_tray->setToolTip(QCoreApplication::applicationName());
    m_tray->show();

    load_settings();
    load_accounts();
    login();
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::changeEvent(QEvent *e) {
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::save_settings() {
    m_settings->beginGroup("geometry");
    m_settings->setValue("size", size());
    m_settings->setValue("pos", pos());
    m_settings->endGroup();
    save_accounts();
}

void MainWindow::load_settings() {
    m_settings->beginGroup("geometry");
    resize(m_settings->value("size", QSize(500, 600)).toSize());
    move(m_settings->value("pos", QPoint(200, 200)).toPoint());
    m_settings->endGroup();
}

int MainWindow::load_accounts() {
    int total_accounts = m_settings->beginReadArray("accounts");
    for (int i = 0; i < total_accounts; ++i) {
        m_settings->setArrayIndex(i);
        TalkerAccount *a = new TalkerAccount("", "", "", this);
        a->load_settings(*m_settings);
        connect(a, SIGNAL(new_rooms_available(const TalkerAccount&)),
                SLOT(update_rooms(const TalkerAccount&)));
        this->m_accounts.append(a);
    }
    m_settings->endArray();
    qDebug() << "loaded" << total_accounts << "accounts from settings";
    return total_accounts;
}

void MainWindow::save_accounts() {
    m_settings->beginWriteArray("accounts", m_accounts.size());
    for (int i = 0; i < m_accounts.size(); ++i) {
        m_settings->setArrayIndex(i);
        m_accounts.at(i)->save(*m_settings);
    }
    m_settings->endArray();
}

void MainWindow::closeEvent(QCloseEvent *e) {
    qDebug() << "window closing...";
    save_settings();
    logout();
}

void MainWindow::set_interface_enabled(const bool &enabled) {
    ui->action_login->setEnabled(!enabled);
    ui->action_logout->setEnabled(enabled);
    ui->btn_chat_submit->setEnabled(enabled);
    ui->le_chat_entry->setEnabled(enabled);
}

void MainWindow::update_rooms(const TalkerAccount &acct) {
    qDebug() << "HEY NEW ROOMS FOR" << acct.name();
    foreach (QString name, acct.avail_rooms().keys()) {
        qDebug() << "ROOM:" << name;
        ui->cb_rooms->addItem(QString("%1::%2").arg(acct.name()).arg(name),
                              name);
    }

}

void MainWindow::login() {
    // show a list of configured accounts, and allow multiple selection for
    // which ones to login to (ONLY IF THERE IS MORE THAN 1 ACCOUNT CONFIGURED)
    int total_accounts = m_accounts.length();
    if (total_accounts < 1) {
        if (QMessageBox::question(
                this, tr("No Account Configured"),
                tr("You have not yet configured an account, would you like to "
                   "do so now?"), QMessageBox::Yes, QMessageBox::No
                ) == QMessageBox::Yes) {

            // they want to make a new account, open the dialog
            TalkerAccount *acct = TalkerAccount::create_new(this, this);
            if (acct) { // they accepted the dialog
                m_accounts.append(acct); // hold on to it for this session
                save_accounts(); // write it to disk
                login(); // try again but this time we should have an account
            }
        }
    } else if (total_accounts == 1) {
        // get a room list for this dude...
        m_accounts.at(0)->get_available_rooms();
    } else {
        // show a list of all accounts and let them login to any of them
    }


}

void MainWindow::logout() {
    qDebug() << "logging out of all accounts...";
    foreach(TalkerAccount *a, m_accounts) {
        if (a)
            a->logout();
    }
}

void MainWindow::submit_message() {
    /*
    QString msg = ui->le_chat_entry->text();
    ui->le_chat_entry->clear();
    qDebug() << "submitting message:" << msg;
    QString body("{\"type\":\"message\",\"content\":\"%1\"}\r\n");
    m_ssl->write(body.arg(msg).toAscii());
    ui->le_chat_entry->setFocus();
    */
}

void MainWindow::on_test() {
    qDebug() << "test called";
    m_tray->showMessage("test1", "test2", QSystemTrayIcon::Warning, 10000);
}
