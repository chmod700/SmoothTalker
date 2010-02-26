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
#include "options_dialog.h"
#include "ui_options_dialog.h"

OptionsDialog::OptionsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    // build a model for the message limit drop down
    ui->cb_message_limit->clear(); // gid rid of what was made in the designer
    ui->cb_message_limit->addItem(tr("Unlimited"), 0);
    ui->cb_message_limit->addItem(tr("1,000"), 1000);
    ui->cb_message_limit->addItem(tr("500"), 500);
    ui->cb_message_limit->addItem(tr("100"), 100);
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

    s->setValue("total_messages_per_room", ui->cb_message_limit->itemData(
            ui->cb_message_limit->currentIndex()).toInt());
    s->setValue("flash_when_not_active", ui->cb_flash->isChecked());
    s->setValue("reopen_last_session_rooms", ui->cb_auto_join->isChecked());

    s->beginGroup("sound_files");
    s->setValue("message_received", ui->le_sound_msg_received->text());

    s->endGroup(); // sound_files
    s->endGroup(); // options
}

void OptionsDialog::load_settings(QSettings *s) {
    s->beginGroup("options");
    ui->cb_show_timestamps->setChecked(
            s->value("show_timestamps", true).toBool());
    ui->cb_flash->setChecked(
            s->value("flash_when_not_active", true).toBool());
    ui->cb_auto_join->setChecked(
            s->value("reopen_last_session_rooms", true).toBool());

    int limit = s->value("total_messages_per_room", 0).toInt();
    for (int i = 0; i < ui->cb_message_limit->count(); ++i) {
        if (ui->cb_message_limit->itemData(i, Qt::UserRole) == limit) {
            ui->cb_message_limit->setCurrentIndex(i);
            break;
        }
    }

    s->beginGroup("sound_files");
    ui->le_sound_msg_received->setText(
            s->value("message_received", QString()).toString());
    s->endGroup(); // sound_files
    s->endGroup(); // options
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
