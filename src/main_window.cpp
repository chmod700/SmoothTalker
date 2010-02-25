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
#include "custom_tab_widget.h"
#include "options_dialog.h"
#include "ui_main_window.h"
#include "ui_account_edit_dialog.h"
#include "talker_account.h"
#include "talker_room.h"
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
    , m_options(new OptionsDialog(this))
    , m_connected_accounts(0)
    , m_tabs(new CustomTabWidget(this))
    , m_tab_bar(new QTabBar(this))
{
    // load up our pretty design
    ui->setupUi(this);

    // give the mainwindow a unique name so other classes can find it
    this->setObjectName("MAINWINDOW");

    // turn off the toolbar
    ui->toolBar->hide();

    // put the tab widget into the main layout and hide it until we connect
    m_tabs->setVisible(false);
    m_tabs->setTabBar(m_tab_bar);
    m_tabs->setTabsClosable(true);
    m_tabs->setTabPosition(QTabWidget::South);
    connect(m_tabs, SIGNAL(tabCloseRequested(int)), SLOT(on_tab_close(int)));
    connect(m_tabs, SIGNAL(currentChanged(int)), SLOT(on_tab_switch(int)));
    ui->vbox_main->insertWidget(1, m_tabs, 10);

    statusBar()->addPermanentWidget(m_status_lbl, 1);
    m_status_lbl->setText(tr("Not Connected"));

    //setup system tray icon
    m_tray_menu->addAction(QIcon(":img/icons/door_out.png"), tr("E&xit"),
                           this, SLOT(close()));
    m_tray->setContextMenu(m_tray_menu);
    m_tray->setIcon(QIcon(":img/icons/transmit.png"));
    m_tray->setToolTip(QCoreApplication::applicationName());
    m_tray->show();

    // make sure we have SSL access, or the whole app is worthless
    if (!QSslSocket::supportsSsl()) {
        QMessageBox::warning(this, tr("Secure Sockets Library Not Found!"),
                             tr("This tool requires an SSL library with TLSv1 "
                                "support. And one was not found on your "
                                "system. The application will now close.")
                             );
        QTimer::singleShot(0, this, SLOT(close()));
    } else {
        load_settings();
        load_accounts();
        login();
    }
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

    m_options->load_settings(m_settings);
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
    //qDebug() << "loaded" << total_accounts << "accounts from settings";
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
    ui->cb_rooms->setEnabled(enabled);
    ui->btn_join_room->setEnabled(enabled);
    m_tabs->setVisible(enabled);
    ui->lbl_not_connected->setVisible(!enabled);
}

void MainWindow::update_rooms(const TalkerAccount &acct) {
    // TODO remove rooms when an account goes away...
    //qDebug() << "HEY NEW ROOMS FOR" << acct.name();
    ui->cb_rooms->clear();
    foreach (QString name, acct.avail_rooms().keys()) {
        //qDebug() << "ROOM:" << name;
        ui->cb_rooms->addItem(QString("%1::%2").arg(acct.name()).arg(name),
                              acct.avail_rooms()[name]);
    }
}

void MainWindow::join_room() {
    int room_id = ui->cb_rooms->itemData(ui->cb_rooms->currentIndex()).toInt();
    foreach(TalkerAccount *a, this->m_accounts) {
        QMap<QString, int> rooms = a->avail_rooms();
        foreach(QString name, rooms.keys()) {
            int id = rooms[name];
            if (id == room_id) {
                // are we already connected to this room?
                bool open_already = false;
                for(int i = 0; i < m_tabs->count(); ++i) {
                    if (m_tab_bar->tabData(i).toInt() == id) {
                        open_already = true;
                        break;
                    }
                }
                if (!open_already)
                    a->open_room(id);
            }
        }
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
        connect(m_accounts.at(0), SIGNAL(room_connected(const TalkerRoom*)),
                SLOT(on_room_connected(const TalkerRoom*)));
        connect(m_accounts.at(0), SIGNAL(room_disconnected(const int)),
                SLOT(on_room_disconnected(const int)));
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
    QString msg = ui->le_chat_entry->text();
    ui->le_chat_entry->clear();
    //qDebug() << "submitting message:" << msg;

    // find the active room...
    int current_room_id = m_tab_bar->tabData(m_tab_bar->currentIndex()).toInt();
    foreach(TalkerAccount *a, m_accounts) {
        foreach(TalkerRoom *r, a->active_rooms()) {
            if (r->id() == current_room_id) {
                r->submit_message(msg);
            }
        }
    }
    ui->le_chat_entry->setFocus();
}

void MainWindow::on_room_connected(const TalkerRoom *room) {
    m_connected_accounts++;

    connect(room, SIGNAL(message_received(QString,QString,const TalkerRoom*)),
            SLOT(on_message_received(const QString&, const QString&,
                                     const TalkerRoom*)));
    connect(room, SIGNAL(users_updated(const TalkerRoom*)),
            SLOT(on_users_updated(const TalkerRoom*)));
    connect(room, SIGNAL(user_updated(const TalkerRoom*, const TalkerUser*)),
            SLOT(on_user_updated(const TalkerRoom*, const TalkerUser*)));
    connect(this, SIGNAL(options_changed(QSettings*)), room,
            SLOT(on_options_changed(QSettings*)));
    emit options_changed(m_settings); // to propogate to this new room

    // draw a tab for this dude.
    QWidget *w = room->get_widget();
    m_tabs->addTab(w, room->name());
    m_tab_bar->setTabData(m_tabs->indexOf(w), room->id());
    set_interface_enabled(m_connected_accounts);
}

void MainWindow::on_room_disconnected(const int room_id) {
    qDebug() << "room disconnected" << room_id << "removing tab";
    m_connected_accounts--;

    // hide any tabs, and show the label
    for(int i = 0; i < m_tab_bar->count(); ++i) {
        if (m_tab_bar->tabData(i).toInt() == room_id) {
            // TODO: nuke widget
            m_tabs->removeTab(i);
        }
    }
    set_interface_enabled(m_connected_accounts);
}

void MainWindow::on_message_received(const QString &sender,
                                     const QString &content,
                                     const TalkerRoom *room) {
    //qDebug() << "message received by room" << room << "from"
    //    << sender << "(" << content << ")";
    QString path = m_settings->value("options/sound_files/message_received",
                                     QString()).toString();
    if (!path.isEmpty() && QFile::exists(path)) {
        QSound::play(path);
    }

    if (!isActiveWindow()) {
        m_tray->showMessage(QString("message from %1").arg(sender), content,
                            QSystemTrayIcon::Information, 2000);
        activateWindow();
    }
}

void MainWindow::on_users_updated(const TalkerRoom *room) {
    int active_room_id = m_tab_bar->tabData(m_tabs->currentIndex()).toInt();
    if (active_room_id != room->id()) {
        return; // ignore this...
    }
    QMap<int, TalkerUser*> users = room->get_users();

    ui->tbl_users->clearContents();
    ui->tbl_users->setRowCount(users.count());
    int i = 0;
    foreach(TalkerUser *u, users.values()) {
        QTableWidgetItem *name_item = new QTableWidgetItem(u->name);
        name_item->setData(Qt::UserRole, u->id);
        if (!u->avatar.isNull()) {
            name_item->setIcon(u->avatar);
        }
        ui->tbl_users->setItem(i, 0, name_item);
        ++i;
    }
}

void MainWindow::on_user_updated(const TalkerRoom *room,
                                 const TalkerUser *user) {
    qDebug() << "in mainwindow, got user_updated for" << user->name;
    int active_room_id = m_tab_bar->tabData(m_tabs->currentIndex()).toInt();
    if (active_room_id != room->id()) {
        return; // ignore this...
    }
    bool found = false;
    for (int i = 0; i < ui->tbl_users->rowCount(); ++i) {
        QTableWidgetItem *item = ui->tbl_users->item(i, 0);
        if (item->data(Qt::UserRole).toInt() == user->id) {
            item->setText(user->name);
            item->setIcon(user->avatar);
            found = true;
            break;
        }
    }
    if (!found) {
        QTableWidgetItem *name_item = new QTableWidgetItem(user->name);
        name_item->setData(Qt::UserRole, user->id);
        if (!user->avatar.isNull()) {
            name_item->setIcon(user->avatar);
        }
        ui->tbl_users->setItem(ui->tbl_users->rowCount(), 0, name_item);
    }
}

void MainWindow::on_tab_close(int tab_idx) {
    qDebug() << "request to close tab index:" << tab_idx;
    int room_id = m_tab_bar->tabData(tab_idx).toInt();
    foreach(TalkerAccount *a, m_accounts) {
        a->close_room(room_id);
    }
}

void MainWindow::on_tab_switch(int new_idx) {
    qDebug() << "request to switch to tab index:" << new_idx;
    int room_id = m_tab_bar->tabData(new_idx).toInt();
    foreach(TalkerAccount *a, m_accounts) {
        foreach(TalkerRoom *r, a->active_rooms()) {
            if (r->id() == room_id) {
                on_users_updated(r);
            }
        }
    }
}

void MainWindow::on_options_activated() {
    if (m_options->exec()) { // accepted
        m_options->save_settings(m_settings);
        emit options_changed(m_settings);
    } else { // cancel
        m_options->load_settings(m_settings);
    }
}
