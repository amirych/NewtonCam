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
#include <QAbstractSocket>


#define SERVERGUI_DEFAULT_FONTSIZE 12

class SERVERGUISHARED_EXPORT ServerGUI: public QMainWindow
{
    Q_OBJECT
public:
    ServerGUI(QWidget *parent = 0);
    ServerGUI(int fontsize, QWidget *parent = 0);

    void SetFonts(int fontsize, int status_fontsize, int log_fontsize);

    void Reset(); // reset GUI widgets to default "unconnected" state

    ~ServerGUI();

public slots:
    void LogMessage(QString msg);
    void TempChanged(double temp);
    void CoolerStatusChanged(unsigned int status);
    void NetworkError(QAbstractSocket::SocketError err);
    void ServerError(unsigned int err_code);
    void ServerStatus(QString status);
    void ExposureProgress(double progress);

private:
    int fontSize;
    int statusFontSize;
    int logFontSize;

    QLCDNumber* exp_progress;
    QLabel* cam_status_label;
    QTextEdit* log_window;
    QLabel* temperature_label;
    QLabel* cooler_label;
    QLabel* camera_err_label;
    QLabel* network_err_label;

    int statusBarWidth();
};

#endif // SERVERGUI_H
