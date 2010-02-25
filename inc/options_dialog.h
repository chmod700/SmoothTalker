#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include <QtGui>

namespace Ui {
    class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT
public:
    OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

    void save_settings(QSettings *s);
    void load_settings(QSettings *s);

protected:
    void changeEvent(QEvent *e);

private:
    Ui::OptionsDialog *ui;

    private slots:
    void on_btn_sound_msg_received_clicked();
    void on_btn_test_sound_msg_received_clicked();
};

#endif // OPTIONS_DIALOG_H
