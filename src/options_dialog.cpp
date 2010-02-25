#include "options_dialog.h"
#include "ui_options_dialog.h"

OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);
}

OptionsDialog::~OptionsDialog() {
    delete ui;
}

void OptionsDialog::changeEvent(QEvent *e) {
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void OptionsDialog::save_settings(QSettings *s) {
    s->beginGroup("options");
    s->setValue("show_timestamps", ui->cb_show_timestamps->isChecked());

    s->beginGroup("sound_files");
    s->setValue("message_received", ui->le_sound_msg_received->text());
    s->endGroup();
    s->endGroup();
}

void OptionsDialog::load_settings(QSettings *s) {
    s->beginGroup("options");
    ui->cb_show_timestamps->setChecked(
            s->value("show_timestamps", true).toBool());

    s->beginGroup("sound_files");
    ui->le_sound_msg_received->setText(
            s->value("message_received", QString()).toString());
    s->endGroup();
    s->endGroup();
}

void OptionsDialog::on_btn_sound_msg_received_clicked() {
    QString path = QFileDialog::getOpenFileName(this, tr("Select Sound"),
                                                QDir::currentPath(),
                                                "Sounds (*.wav)");
    if (QFile::exists(path)) {
        ui->le_sound_msg_received->setText(path);
    }
}

void OptionsDialog::on_btn_test_sound_msg_received_clicked() {
    QString path = ui->le_sound_msg_received->text();
    if (!path.isEmpty() && QFile::exists(path)) {
        QSound::play(path);
    }
}
