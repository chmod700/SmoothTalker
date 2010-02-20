# -------------------------------------------------
# Project created by QtCreator 2010-01-08T11:21:09
# -------------------------------------------------
# QT libs we need
QT += network \
    script \
    webkit \
    xml \
    multimedia

# basic app config
TARGET = SmoothTalker
TEMPLATE = app
debug {
    DESTDIR = bin/debug
}
release {
    DESTDIR = bin/release
}

# where to put all the temporary crap
OBJECTS_DIR = bin/temp
MOC_DIR = bin/temp
RCC_DIR = bin/temp
UI_HEADERS_DIR = bin/temp

# where to find files
DEPENDPATH += inc \
    src \
    ui \
    resources
INCLUDEPATH += inc

# what files to find
SOURCES += main.cpp \
    main_window.cpp \
    talker_account.cpp \
    talker_room.cpp
HEADERS += main_window.h \
    talker_account.h \
    talker_room.h
FORMS += main_window.ui \
    account_edit_dialog.ui
RESOURCES += icons.qrc

