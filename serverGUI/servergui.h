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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>


#define SERVERGUI_DEFAULT_FONTSIZE 12

class SERVERGUISHARED_EXPORT ServerGUI: public QMainWindow
{

public:
    ServerGUI(QWidget *parent = 0);
    ServerGUI(int fontsize, QWidget *parent = 0);

public slots:
    void LogMessage(QString msg);
    void TempChanged(double temp);
    void CoolerStatusChanged(unsigned int status);
    void NetworkError();
    void ServerError(unsigned int err_code);

private:
    int fontSize;

    QLCDNumber* exp_progress;
    QLabel* cam_status_label;
    QTextEdit* log_window;
    QLabel* temperature_label;
    QLabel* cooler_label;
    QLabel* camera_err_label;
    QLabel* network_err_label;

};

#endif // SERVERGUI_H
