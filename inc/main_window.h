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
class TalkerAccount;

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
    QSettings *m_settings;
    QNetworkAccessManager *m_net;
    QSslSocket *m_ssl;
    QScriptEngine *m_engine;
    QString m_token;
    QTimer *m_timer;
    QLabel *m_status_lbl;
    QString m_room_to_join;
    QMenu *m_tray_menu;
    QSystemTrayIcon *m_tray;

    QList<TalkerAccount*> m_accounts;
    TalkerAccount *m_acct;

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

    void rooms_request_finished(QNetworkReply*);
    void on_test();

};

#endif // MAINWINDOW_H
