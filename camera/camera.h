#ifndef CAMERA_H
#define CAMERA_H

#include "camera_global.h"


#include<iostream>
#include<QObject>
#include <QDateTime>
#include <QString>
#include <QDebug>
#include <QMutex>
#include <QTimer>

#ifdef Q_OS_WIN
    #include "../AndorSDK/atmcd32d.h"
    typedef WORD buffer_t;
#endif

#ifdef Q_OS_LINUX
    #include "../AndorSDK/atmcdLXd.h"
    typedef unsigned short buffer_t;
#endif



            /******************************************
            *                                         *
            *  Andor Newton camera API wrapper class  *
            *                                         *
            ******************************************/

#define CAMERA_FITS_ERROR_BASE 700 // displacement for the FITS error codes

#define CAMERA_STATUS_UNINITILIZED_TEXT "<font color=red>Uninitialized</font>"
#define CAMERA_STATUS_INIT_TEXT         "<font color=red>Initialization ...</font>"
#define CAMERA_STATUS_READY_TEXT        "Ready"
#define CAMERA_STATUS_ACQUISITION_TEXT  "Acquisition ..."
#define CAMERA_STATUS_ABORT_TEXT        "<font color=red>Abort!</font>"
#define CAMERA_STATUS_READING_TEXT      "Reading Image ..."
#define CAMERA_STATUS_SAVING_TEXT       "Saving Image ..."
#define CAMERA_STATUS_FAILURE_TEXT      "<font color=red>Failure</font>"


#define CAMERA_DEFAULT_TEMP_POLLING_INT 10 // in seconds, default interval for CCD chip temperature polling
#define CAMERA_DEFAULT_STATUS_POLLING_INT 100 // in milliseconds, default interval for Newton-camera status polling


#define CAMERA_TIMER_RESOLUTION 1.0 // resolution of the camera exposure timer (in seconds)

            /*  READ-OUT RATE MNEMONIC CONSTANTS (FOR ANDOR NEWTON CAMERA ONLY!!!) */
#define CAMERA_READOUT_SPEED0_STR "FAST"  // API index 0: at 3Mhz (horizontal shift speed)
#define CAMERA_READOUT_SPEED1_STR "NORM"  // API index 1: at 1 MHz
#define CAMERA_READOUT_SPEED2_STR "SLOW"  // API index 2: at 0.05 MHz

            /*  GAIN MNEMONIC CONSTANTS (FOR ANDOR NEWTON CAMERA ONLY!!!)  */
#define CAMERA_GAIN0_STR "LOW"   // API index 0
#define CAMERA_GAIN1_STR "NORM"  // API index 1
#define CAMERA_GAIN2_STR "HIGH"  // API index 2


            /*   Default shutter control parameters   */
#define CAMERA_DEFAULT_SHUTTER_CLOSINGTIME 100 // shutter closing time in millisecs
#define CAMERA_DEFAULT_SHUTTER_OPENINGTIME 100 // shutter opening time in millisecs
#define CAMERA_DEFAULT_SHUTTER_TTL_SIGNAL  1   // output TTL signal to open shutter (1 - high, 0 - low)
#define CAMERA_DEFAULT_SHUTTER_MODE 0          // shutter mode (0 - Auto, 1 - OPen, 2 - Close)

// current time point macro. It returns a char* string
#define TIME_STAMP QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ").toUtf8().data()


// forward definitions
class TempPollingThread;
class StatusPollingThread;

class CAMERASHARED_EXPORT Camera: public QObject
{
    Q_OBJECT

    friend class TempPollingThread;
    friend class StatusPollingThread;
public:
    enum CameraErrorCode {CAMERA_ERROR_OK, CAMERA_ERROR_BADALLOC = 100, CAMERA_ERROR_FITS_FILENAME, CAMERA_ERROR_NO_DEVICE};

    explicit Camera(QObject *parent = 0);
    Camera(long camera_index, QObject *parent = 0);
    Camera(std::ostream &log_file, long camera_index = 0, QObject *parent = 0);

    virtual ~Camera();

    void InitCamera(long camera_index = 0);
    void InitCamera(QString init_path, long camera_index = 0);

    // general functions

    unsigned int GetLastError() const;
    QString GetCameraSatus() const;

    void SetPollingIntervals(const unsigned long temp_int, const unsigned long status_int);

    // Temperature and cooler control

    void SetCoolerON();
    void SetCoolerOFF();

    void SetFanState(const QString state);

    void SetCCDTemperature(const int temp);

    void GetCCDTemperature(double *temp, unsigned int *cooler_stat);

    // frame control

    void SetFrame(const int hbin, const int vbin,
                  const int xstart, const int ystart, const int xsize, const int ysize);

    // acquisition control
    void SetExpTime(const double exp_time);

    // read-out speed
    void SetRate(const QString rate_str);

    // gain control
    void SetCCDGain(const QString gain_str);

    // shutter control

    void ShutterControl(int TTL_signal, int mode, int ctime, int otime);
    void ShutterOpen();
    void ShutterClose();

signals:
    void CameraStatus(QString status);
    void CameraError(unsigned int err_code);
    void TemperatureChanged(double temp);
    void CoolerStatusChanged(unsigned int status);
    void ExposureClock(double clock);

public slots:
    void StartExposure(const QString &fits_filename, const QString &hdr_filename = "");
    void StopExposure();

protected slots:
    void ExposureCounter();

    void SaveFITS();

protected:
    long Camera_Index;
    std::ostream *LogFile;

    unsigned int lastError;

    double currentTemperature;
    unsigned int currentCoolerStatus;
    QMutex *tempMutex;
    TempPollingThread *tempPolling;
    unsigned long tempPollingInterval;

    StatusPollingThread *statusPolling;
    unsigned long statusPollingInterval;

    QString cameraStatus;

    QString initPath;

    QTimer *exp_timer;

    double currentExposureClock;

    QString currentFITS_Filename;
    QString currentHDR_Filename;

    QString headModel;
    QString driverVersion;

    int serialNumber;

    unsigned int firmwareVersion;
    unsigned int firmwareBuild;

    QString currentRate;
    QString currentGain;

    int detector_Xsize; // detector dimensions
    int detector_Ysize;

    int currentImage_startX;  // holds start X-coordinate in term of detector pixels (starts from 1)
    int currentImage_startY;  // holds start Y-coordinate in term of detector pixels (starts from 1)
    int currentImage_Xsize;   // holds size of image along X-coordinate in term of binned pixels
    int currentImage_Ysize;   // holds size of image along Y-coordinate in term of binned pixels
    int currentImage_binX;    // bin X
    int currentImage_binY;    // bin Y

    int maxBinning[2];
    int temperatureRange[2];

    buffer_t *currentImage_buffer;

    QDateTime startExposureTime;

    int shutterTTL_signal;
    int shutterMode;
    int shutterClosingTime;
    int shutterOpeningTime;

    void LogOutput(QString log_str, bool time_stamp = true, bool new_line = true);
    void LogOutput(QStringList &log_strs);

//#ifdef EMULATOR_MODE
    double tempSetPoint;

    unsigned int currentStatus;

    double currentExpTime;

    bool isStopped;
//#endif
};

#endif // CAMERA_H
