# -------------------------------------------------
# Project created by QtCreator 2010-01-08T11:21:09
# -------------------------------------------------
QT += network \
    script \
    webkit \
    xml \
    multimedia
TARGET = SmoothTalker
TEMPLATE = app
SOURCES += src/main.cpp \
    src/main_window.cpp \
    src/talker_account.cpp \
    src/talker_room.cpp
HEADERS += inc/main_window.h \
    inc/talker_account.h \
    inc/talker_room.h
FORMS += ui/main_window.ui \
    ui/account_edit_dialog.ui
RESOURCES += resources/icons.qrc
