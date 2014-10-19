#ifndef CAMERA_H
#define CAMERA_H

#include "camera_global.h"

#include<iostream>
#include<QObject>
#include<ctime>


            /******************************************
            *                                         *
            *  Andor Newton camera API wrapper class  *
            *                                         *
            ******************************************/

class CAMERASHARED_EXPORT Camera: public QObject
{
    Q_OBJECT
public:
    explicit Camera(QObject *parent = 0);
    Camera(long camera_index, QObject *parent = 0);
    Camera(std::ostream &log_file, long camera_index = 0, QObject *parent = 0);

    ~Camera();

    void InitCamera(long camera_index = 0);
    void InitCamera(std::ostream &log_file, long camera_index = 0);

    unsigned int GetLastError() const;

    // Temperature and cooler control

    void SetCoolerON();
    void SetCoolerOFF();

    void SetTemperature(const int temp);

    void GetTemperature(float *temp, unsigned int *cooler_stat);

    // frame control

    void SetImage(const int hbin, const int vbin,
                  const int xstart, const int xend, const int ystart, const int yend);

    // acquisition control

    // read-out speed

    // gain control


    // shutter control

    void ShutterOpen();
    void ShutterClose();

signals:
    void CameraError(unsigned int err_code);

public slots:
    void StartExposure(const QString &fits_filename, const QString &hdr_filename = "");
    void StopExposure();

private:
    long Camera_Index;
    std::ostream *LogFile;

    unsigned int lastError;


    time_t time_point;

    inline void Call_Andor_API(unsigned int err_code, const char *file, int line);
    inline void Call_Andor_API(unsigned int err_code, const char *api_func,
                               const char *file, int line);
};

/* Andor API wrapper macro definition */
#define API_CALL(err_code) { Call_Andor_API((err_code), __FILE__, __LINE__); }

//#define ANDOR_API_CALL(API_FUNC, ...) { \
//    unsigned int lastError = API_FUNC(__VA_ARGS__); \
//    Call_Andor_API(lastError,#API_FUNC,__FILE__,__LINE__); \
//}

#endif // CAMERA_H
