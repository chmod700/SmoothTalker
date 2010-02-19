#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtNetwork>
#include <QScriptEngine>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void save_settings();
    void load_settings();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::MainWindow *ui;
    QSslSocket *m_ssl;
    QScriptEngine *m_engine;
    QString m_token;
    QTimer *m_timer;
    QLabel *m_status_lbl;
    QMenu *m_tray_menu;
    QSound *m_msg_sound;
    QSystemTrayIcon *m_tray;

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

};

#endif // MAINWINDOW_H
