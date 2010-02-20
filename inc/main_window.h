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
    QNetworkAccessManager *m_net; // handles web requests for us
    QSslSocket *m_ssl; // TODO: we need one of these for each room :(
    QTimer *m_timer; // used for keep-alives
    QLabel *m_status_lbl; // where status bar messages are written
    QMenu *m_tray_menu; // the context menu for right clicks on our tray icon
    QSystemTrayIcon *m_tray; // holds our handly little tray icon

    QList<TalkerAccount*> m_accounts; // list of configured accounts
    //TalkerAccount *m_acct; // current account TODO: allow multiple at once

    // single method to enable/disable GUI elements
    void set_interface_enabled(const bool &enabled);

    private slots:
        void login();
        void logout();
        void stay_alive();
        void socket_encrypted();
        void socket_ssl_errors(const QList<QSslError> &errors);
        void socket_ready_read();
        void socket_disconnected();

        void submit_message();
        void handle_message(const QScriptValue &val);

        void on_test(); // silly method hooked up to the file menu for
                        // on-demand testing

};

#endif // MAINWINDOW_H
