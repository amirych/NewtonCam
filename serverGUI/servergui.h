#ifndef SERVERGUI_H
#define SERVERGUI_H

#include "servergui_global.h"

#include <QString>
#include <QMainWindow>
#include <QLCDNumber>
#include <QLabel>
#include <QStatusBar>
#include <QFrame>
#include <QTextEdit>

class SERVERGUISHARED_EXPORT ServerGUI: public QMainWindow
{

public:
    ServerGUI(QObject *parent = 0);

public slots:
    void LogMessage(QString msg);
    void TempChanged(double temp);
    void NetworkError();
    void ServerError(unsigned int err_code);

private:

};

#endif // SERVERGUI_H
