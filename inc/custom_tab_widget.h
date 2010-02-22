#ifndef CUSTOM_TAB_WIDGET_H
#define CUSTOM_TAB_WIDGET_H

#include <QtGui>

/**
  * Needed so we can set QVariant data on each tab
  */
class CustomTabWidget: public QTabWidget {
    Q_OBJECT
public:
    CustomTabWidget(QWidget *parent = 0)
        : QTabWidget(parent)
    {}
    virtual ~CustomTabWidget(){}

    void setTabBar(QTabBar *tb) {
        QTabWidget::setTabBar(tb);
    }
};

#endif // CUSTOM_TAB_WIDGET_H
