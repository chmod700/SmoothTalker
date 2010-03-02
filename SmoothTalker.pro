# -------------------------------------------------
# Project created by QtCreator 2010-01-08T11:21:09
# -------------------------------------------------
# QT libs we need
QT += network \
    script \
    webkit

# basic app config
TARGET = SmoothTalker
TEMPLATE = app
debug:DESTDIR = bin/debug
release:DESTDIR = bin/release

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
    talker_room.cpp \
    src/options_dialog.cpp \
    src/talker_user.cpp
HEADERS += main_window.h \
    talker_account.h \
    talker_room.h \
    inc/custom_tab_widget.h \
    inc/options_dialog.h \
    inc/defines.h \
    inc/talker_user.h
FORMS += main_window.ui \
    account_edit_dialog.ui \
    ui/options_dialog.ui \
    ui/about_dialog.ui
RESOURCES += icons.qrc
OTHER_FILES += resources/templates/message.txt \
    resources/templates/stylesheet.txt
