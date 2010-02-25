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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtNetwork>
#include <QScriptEngine>

namespace Ui {
    class MainWindow;
}
// forward declarations
class TalkerAccount;
class TalkerRoom;
struct TalkerUser;
class CustomTabWidget;
class OptionsDialog;

/**
  * The core of the whole app. Handles choosing accounts, and showing of the
  * main GUI window
  */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void save_settings();
    void load_settings();

    public slots:
        void save_accounts();
        int load_accounts();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::MainWindow *ui;
    QSettings *m_settings; // manages app settings
    QLabel *m_status_lbl; // where status bar messages are written
    QMenu *m_tray_menu; // the context menu for right clicks on our tray icon
    QSystemTrayIcon *m_tray; // holds our handly little tray icon
    OptionsDialog *m_options; // options dialog menu

    QList<TalkerAccount*> m_accounts; // list of configured accounts
    int m_connected_accounts; // holds how many accounts are logged in
    CustomTabWidget *m_tabs;
    QTabBar *m_tab_bar;

    // single method to enable/disable GUI elements
    void set_interface_enabled(const bool &enabled);

    private slots:
        void login();
        void logout();
        void submit_message();
        void join_room();
        void update_rooms(const TalkerAccount&);
        void on_room_connected(const TalkerRoom*);
        void on_room_disconnected(const int room_id);
        void on_tab_switch(int new_idx);
        void on_tab_close(int tab_idx);
        void on_message_received(const QString &sender, const QString &content,
                                 const TalkerRoom *room);
        void on_users_updated(const TalkerRoom*);
        void on_user_updated(const TalkerRoom*, const TalkerUser*);
        void on_options_activated(); // user clicked options menu item

signals:
        void options_changed(QSettings*);

};

#endif // MAINWINDOW_H
