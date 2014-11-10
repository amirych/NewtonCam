#ifndef CAMERA_H
#define CAMERA_H

#include "camera_global.h"


#include<iostream>
#include<ctime>
#include<QObject>
#include <QDateTime>
#include <QString>
#include <QDebug>
#include <QMutex>

#ifdef Q_OS_WIN
    #include "../AndorSDK/atmcd32d.h"
#endif

#ifdef Q_OS_LINUX
    #include "../AndorSDK/atmcdLXd.h"
#endif



            /******************************************
            *                                         *
            *  Andor Newton camera API wrapper class  *
            *                                         *
            ******************************************/

#define CAMERA_STATUS_UNINITILIZED_TEXT "Uninitialized"
#define CAMERA_STATUS_INIT_TEXT         "Init ..."
#define CAMERA_STATUS_READY_TEXT        "Ready"
#define CAMERA_STATUS_ACQUISITION_TEXT  "Acquisition ..."
#define CAMERA_STATUS_READING_TEXT      "Reading Image ..."
#define CAMERA_STATUS_SAVING_TEXT       "Saving Image ..."
#define CAMERA_STATUS_FAILURE_TEXT      "Failure"


#define CAMERA_DEFAULT_TEMP_POLLING_INT 10 // in seconds, default interval for CCD chip temperature polling
#define CAMERA_DEFAULT_STATUS_POLLING_INT 100 // in milliseconds, default interval for Newton-camera status polling

// forward definitions
class TempPollingThread;
class StatusPollingThread;

class CAMERASHARED_EXPORT Camera: public QObject
{
    Q_OBJECT

    friend class TempPollingThread;
public:
    explicit Camera(QObject *parent = 0);
    Camera(long camera_index, QObject *parent = 0);
    Camera(std::ostream &log_file, long camera_index = 0, QObject *parent = 0);

    ~Camera();

    void InitCamera(long camera_index = 0);
    void InitCamera(std::ostream &log_file, long camera_index = 0);

    // general functions

    unsigned int GetLastError() const;

    void SetPollingIntervals(const unsigned long temp_int, const unsigned long status_int);

    // Temperature and cooler control

    void SetCoolerON();
    void SetCoolerOFF();

    void SetCCDTemperature(const int temp);

    void GetCCDTemperature(double *temp, unsigned int *cooler_stat);

    // frame control

    void SetImage(const int hbin, const int vbin,
                  const int xstart, const int xend, const int ystart, const int yend);

    // acquisition control
    void SetExpTime(const double exp_time);

    // read-out speed

    // gain control


    // shutter control

    void ShutterOpen();
    void ShutterClose();

signals:
    void CameraStatus(QString status);
    void CameraError(unsigned int err_code);
    void TemperatureChanged(double temp);
    void CoolerStatusChanged(unsigned int status);
    void ExposureCounter(double counter);

public slots:
    void StartExposure(const QString &fits_filename, const QString &hdr_filename = "");
    void StopExposure();

protected:
    long Camera_Index;
    std::ostream *LogFile;

    unsigned int lastError;

    double currentTemperature;
    unsigned int currentCoolerStatus;
    QMutex tempMutex;
    TempPollingThread *tempPolling;
    unsigned long tempPollingInterval;

    StatusPollingThread *statusPolling;
    unsigned long statusPollingInterval;

    time_t time_point;
    QString date_str;

#ifdef EMULATOR
    double tempSetPoint;

    unsigned int currentStatus;

    double currentExpTime;
#endif
};
#endif // CAMERA_H
