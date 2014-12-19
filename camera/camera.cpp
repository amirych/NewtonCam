#include "../version.h"
#include "camera.h"
#include "temppollingthread.h"
#include "statuspollingthread.h"

//#ifdef _WIN32 || _WIN64
//    #include "atmcd32d.h"
//#else
//    #include "atmcdLXd.h"
//#endif

#include <QString>
#include <thread>
#include <chrono>

//#ifdef Q_OS_WIN
//    #include "atmcd32d.h"
//#endif

//#ifdef Q_OS_LINUX
//    #include "atmcdLXd.h"
//#endif

            /* Andor API wrapper macro definition */


#define ANDOR_API_CALL(API_FUNC, ...) { \
    lastError = API_FUNC(__VA_ARGS__); \
    if (lastError != DRV_SUCCESS ) emit CameraError(lastError); \
    if ( LogFile != nullptr ) { \
        *LogFile << TIME_STAMP; \
        *LogFile << "[Andor API] " << #API_FUNC << "(" << #__VA_ARGS__ << "): "; \
        if ( lastError == DRV_SUCCESS ) { \
            *LogFile << "OK "; \
        } else { \
            *LogFile << "error = " << lastError; \
        } \
        *LogFile << " (in " << __FILE__ << " at line " << __LINE__ << ")" << std::endl << std::flush; \
    }\
}

//#define ANDOR_API_CALL(API_FUNC, ...) { \
//    lastError = API_FUNC(__VA_ARGS__); \
//    emit CameraError(lastError); \
//    LogOutput("   [Andor API] " + #API_FUNC + "(" + #__VA_ARGS__ + "): ", true, false); \
//    if ( lastError == DRV_SUCCESS ) { \
//        LogOutput("OK ", false, false); \
//    } else { \
//        QString str; \
//        str.setNum(lastError); \
//        str.prepend("error = "); \
//        LogOutput(str,false,false); \
//    } \
//    LogOutput(" (in " + __FILE__ + " at line " + __LINE__ + ")", false); \
//}


            /********************************
            *                               *
            *  Camera class implementation  *
            *                               *
            ********************************/

            /*  constructors and destructor  */

Camera::Camera(std::ostream &log_file, long camera_index, QObject *parent):
    QObject(parent), Camera_Index(camera_index), LogFile(nullptr), lastError(DRV_NOT_INITIALIZED),
    tempMutex(nullptr),
    tempPolling(nullptr),
    tempPollingInterval(CAMERA_DEFAULT_TEMP_POLLING_INT),
    statusPolling(nullptr),
    statusPollingInterval(CAMERA_DEFAULT_STATUS_POLLING_INT),
    cameraStatus(CAMERA_STATUS_UNINITILIZED_TEXT), initPath(""),
    exp_timer(nullptr), currentExposureClock(0.0), currentExpTime(0.0),
    currentFITS_Filename(""), currentHDR_Filename(""),
    headModel(""), driverVersion("")
{
#ifdef QT_DEBUG
    qDebug() << "Create Camera";
#endif

    LogFile = &log_file;

    cameraStatus = CAMERA_STATUS_UNINITILIZED_TEXT;

    LogOutput("Start camera:\n");

    exp_timer = new QTimer(this);
    connect(exp_timer,SIGNAL(timeout()),this,SLOT(ExposureCounter()));

    tempMutex = new QMutex;
#ifdef EMULATOR_MODE
    currentStatus = DRV_SUCCESS;
#endif
}


Camera::Camera(QObject *parent): Camera(std::cerr,0,parent)
{
}


Camera::Camera(long camera_index, QObject *parent): Camera(std::cerr, camera_index, parent)
{
}


Camera::~Camera()
{
    if ( tempPolling != nullptr ) {
        tempPolling->stop();
        tempPolling->wait(2000);
        delete tempPolling;
    }

    if ( statusPolling != nullptr ) {
        statusPolling->stop();
        statusPolling->wait(2000);
        delete statusPolling;
    }

    delete tempMutex;

    ANDOR_API_CALL(ShutDown,);
    LogOutput("",false);
    LogOutput("Shutdown camera.");
//    if ( LogFile != nullptr ) {
//        *LogFile << TIME_STAMP << "Stop camera.\n";
//    }
}

            /*  public methods  */

void Camera::InitCamera(QString init_path, long camera_index)
{
    LogOutput("",false);
    LogOutput("   [CAMERA] Initializing camera ...");

    initPath = init_path;
    Camera_Index = camera_index;

    // if init while acquisition is in progress then abort it

#ifdef EMULATOR_MODE
    if ( currentStatus == DRV_ACQUIRING ) StopExposure();
#else
    if ( lastError != DRV_NOT_INITIALIZED ) {
#ifdef QT_DEBUG
        qDebug() << "Re-init camera ...";
#endif
        int camera_status;
        ANDOR_API_CALL(GetStatus,&camera_status)
        if ( camera_status == DRV_ACQUIRING ) {
            LogOutput("   [CAMERA] Init while acquisition in progress! Stop current exposure ...", true, false);
            StopExposure();
            LogOutput("  OK");
        }
    }
#endif

    lastError = DRV_SUCCESS;
    emit CameraError(lastError);

    at_32 no_cameras = 0;

    cameraStatus = CAMERA_STATUS_INIT_TEXT;
    emit CameraStatus(cameraStatus);


    LogOutput("   [CAMERA] Looking for connected Andor cameras ...");

    ANDOR_API_CALL(GetAvailableCameras,&no_cameras);
//    lastError = GetAvailableCameras(&no_cameras);

    qDebug() << "last err = " << lastError;
    qDebug() << "edkedejdhekj";

#ifdef QT_DEBUG
    qDebug() << "CAMERA: Number of found cameras: " << no_cameras;
#endif

#ifdef EMULATOR_MODE
    LogOutput("   [CAMERA] EMULATOR_MODE: start emulation mode!");
#else
    if ( !no_cameras ) { // camera is not detected!
        if ( lastError == DRV_SUCCESS ) lastError = DRV_GENERAL_ERRORS; // NO GOOD IDEA!!!
        LogOutput("   [CAMERA] Cannot detect any cameras!");
        cameraStatus = CAMERA_STATUS_UNINITILIZED_TEXT;
        emit CameraStatus(cameraStatus);
        emit CameraError(lastError);
        return;
    } else {
        QString str;
        str.setNum(no_cameras);
        LogOutput("   [CAMERA] Detect " + str + " cameras");
    }

    LogOutput("   [CAMERA] Initialization path: " + initPath);
    char* path = initPath.toUtf8().data();
    LogOutput("   [CAMERA] Initialization proccess ...", true, false);

    ANDOR_API_CALL(Initialize,path);

    if ( lastError != DRV_SUCCESS ) {
        LogOutput("  Failed!",false);
        cameraStatus = CAMERA_STATUS_UNINITILIZED_TEXT;
        emit CameraStatus(cameraStatus);
        emit CameraError(lastError);
        return;
    } else {
        LogOutput("  OK!",false);
    }

    cameraStatus = CAMERA_STATUS_INIT_TEXT;
    emit CameraStatus(cameraStatus);

    std::this_thread::sleep_for(std::chrono::seconds(2)); // sleep for init proccess finishing

    char head_model[MAX_PATH];
    ANDOR_API_CALL(GetHeadModel,head_model);
    headModel = head_model;
    LogOutput("   [CAMERA] Head model: " + headModel);

    ANDOR_API_CALL(GetCameraSerialNumber,&serialNumber);
    QString str;
    str.setNum(serialNumber);
    LogOutput("   [CAMERA] Camera serial number: " + str);

    char driver_ver[100];
    ANDOR_API_CALL(GetVersionInfo,AT_DeviceDriverVersion,driver_ver,100);
    driverVersion = driver_ver;
    LogOutput("   [CAMERA] Driver version: " + driverVersion);
#endif

    int t_min, t_max;

    ANDOR_API_CALL(GetTemperatureRange,&t_min,&t_max);
#ifdef QT_DEBUG
    qDebug() << "Temperature range: [" << t_min << "," << t_max << "]";
#endif

    // initial values for image mode, size and binning

    ANDOR_API_CALL(SetReadMode,4); // image mode!
    ANDOR_API_CALL(GetDetector,&currentImage_Xsize,&currentImage_Ysize);
    currentImage_binX = 1;
    currentImage_binX = 1;
    ANDOR_API_CALL(SetImage,currentImage_binX,currentImage_binY,1,currentImage_Xsize,1,currentImage_Ysize);

#ifdef QT_DEBUG
    qDebug() << "Detector size: [" << currentImage_Xsize << "; " << currentImage_Ysize << "]";
#endif

    // set initial horizontal and vertical shift speed (i.e. read-out rate)
    SetRate(CAMERA_READOUT_SPEED2_STR);
    int rec_vspeed;
    float vspeed;
    ANDOR_API_CALL(GetFastestRecommendedVSSpeed,&rec_vspeed,&vspeed);
    ANDOR_API_CALL(SetVSSpeed,rec_vspeed); // just set the recommended fastest vertical speed

    // set initial gain factor

    SetCCDGain(CAMERA_GAIN2_STR);

    // get current CCD chip temperature and cooler status

    GetCCDTemperature(&currentTemperature,&currentCoolerStatus);
    emit TemperatureChanged(currentTemperature);
    emit CoolerStatusChanged(currentCoolerStatus);

    // start CCD chip temperature polling

    if ( tempPolling == nullptr ) {
        tempPolling = new TempPollingThread(this,tempPollingInterval);
    } else {
        tempPolling->stop();
        tempPolling->wait(2000);
    }

    tempPolling->start();

    if ( statusPolling == nullptr ) {
        statusPolling = new StatusPollingThread(this,statusPollingInterval);
    } else {
        statusPolling->stop();
        statusPolling->wait(2000);
    }


    cameraStatus = CAMERA_STATUS_READY_TEXT;
    emit CameraStatus(cameraStatus);

#ifdef QT_DEBUG
    qDebug() << "Is temperature polling started: " << tempPolling->isRunning();
    qDebug() << "Is status polling started: " << statusPolling->isRunning();
#endif

#ifdef EMULATOR_MODE
    tempSetPoint = -30.0;
    currentExpTime = 0.0;
#endif
}


void Camera::InitCamera(long camera_index)
{
    InitCamera(initPath,camera_index);
}

unsigned int Camera::GetLastError() const
{
    return lastError;
}

QString Camera::GetCameraSatus() const
{
    return cameraStatus;
}

void Camera::SetPollingIntervals(const unsigned long temp_int, const unsigned long status_int)
{
    tempPollingInterval = temp_int;
    statusPollingInterval = status_int;

    if ( tempPolling != nullptr ) {
        tempPolling->setPollingInterval(tempPollingInterval);
    }

    if ( statusPolling != nullptr ) {
        statusPolling->setInterval(statusPollingInterval);
    }
}


void Camera::SetCoolerOFF()
{
    unsigned int cool;
    double temp;

    ANDOR_API_CALL(CoolerOFF,);

    GetCCDTemperature(&temp,&cool);
    emit TemperatureChanged(temp);
    emit CoolerStatusChanged(cool);
}


void Camera::SetCoolerON()
{
    unsigned int cool;
    double temp;

    ANDOR_API_CALL(CoolerON,);

    GetCCDTemperature(&temp,&cool);
    emit TemperatureChanged(temp);
    emit CoolerStatusChanged(cool);
}


void Camera::SetFanState(const QString state)
{
    QString fan_state = state.toUpper().trimmed();

    if ( fan_state.isEmpty() ) {
        lastError = DRV_P1INVALID;
        return;
    }

    if ( fan_state == "FULL" ) {
        ANDOR_API_CALL(SetFanMode,0);
    } else if ( fan_state == "LOW" ) {
        ANDOR_API_CALL(SetFanMode,1);
    } else if ( fan_state == "OFF" ) {
        ANDOR_API_CALL(SetFanMode,2);
    } else { // unknown mode
        lastError = DRV_P1INVALID;
    }
}


void Camera::SetCCDTemperature(const int temp)
{
#ifdef EMULATOR_MODE
    if ( LogFile != nullptr ) {
        tempSetPoint = temp;
        *LogFile << TIME_STAMP << "EMULATOR_MODE: Set CCD temperature: " << temp << " degrees\n";
    }
#else
    ANDOR_API_CALL(SetTemperature,temp);
#endif
}


void Camera::GetCCDTemperature(double *temp, unsigned int *cooler_stat)
{
//    QMutexLocker lock(tempMutex);

#ifdef EMULATOR_MODE
    *temp = currentTemperature;
    *cooler_stat = currentCoolerStatus;
#else
    float curr_temp;
    unsigned int st;
//    currentCoolerStatus = GetTemperatureF(&curr_temp);
    st = GetTemperatureF(&curr_temp);
    switch ( st ) {
        case DRV_NOT_INITIALIZED:
        case DRV_ACQUIRING:
        case DRV_ERROR_ACK: {
//            lastError = currentCoolerStatus;
            tempMutex->lock();
            lastError = st;
            tempMutex->unlock();
            emit CameraError(lastError);
            break;
        }
        default: { // here one has real temperature value
            tempMutex->lock();
//            qDebug() << "[CAMERA] lock tempMutex";
            currentTemperature = static_cast<double>(curr_temp);
            currentCoolerStatus = st;
            *cooler_stat = st;
            *temp = currentTemperature;
            tempMutex->unlock();
        }
    }
#endif
//    qDebug() << "[CAMERA] unlock tempMutex";
}

void Camera::SetExpTime(const double exp_time)
{
    currentExpTime = exp_time;
    StartExposure("","");
#ifndef EMULATOR_MODE
    ANDOR_API_CALL(SetExposureTime,exp_time);
#endif
}


void Camera::SetCCDGain(const QString gain_str)
{
    QString str = gain_str.trimmed().toUpper();

    int idx;
    if ( str == CAMERA_GAIN0_STR ) {
        idx = 0;
    } else if ( str == CAMERA_GAIN1_STR ) {
        idx = 1;
    } else if ( str == CAMERA_GAIN2_STR ) {
        idx = 2;
    } else {
        idx = 2;
    }

    ANDOR_API_CALL(SetPreAmpGain,idx);
}


void Camera::SetRate(const QString rate_str)
{
    QString str = rate_str.trimmed().toUpper();

    int idx;

    if ( str == CAMERA_READOUT_SPEED0_STR ) {
        idx = 0;
    } else if ( str == CAMERA_READOUT_SPEED1_STR ) {
        idx = 1;
    } else if ( str == CAMERA_READOUT_SPEED2_STR ) {
        idx = 2;
    } else {
        idx = 2;
    }

    ANDOR_API_CALL(SetHSSpeed,0,idx);
}


            /*  public slots  */

void Camera::StartExposure(const QString &fits_filename, const QString &hdr_filename)
{
#ifdef EMULATOR_MODE
    currentStatus = DRV_ACQUIRING;
    lastError = DRV_SUCCESS;
#else
    ANDOR_API_CALL(StartAcquisition,);
#endif
    if ( lastError == DRV_SUCCESS ) {
        statusPolling->start();
    }
    cameraStatus = CAMERA_STATUS_ACQUISITION_TEXT;
    emit CameraStatus(cameraStatus);
    exp_timer->start(CAMERA_TIMER_RESOLUTION*1000);
}


void Camera::StopExposure()
{
#ifdef EMULATOR_MODE
    currentStatus = DRV_SUCCESS;
#else
    ANDOR_API_CALL(AbortAcquisition,);
#endif
    exp_timer->stop();
    statusPolling->stop();
    currentExposureClock = 0.0;
    emit ExposureClock(currentExposureClock);
}



            /*  protected methods  */

void Camera::LogOutput(QString log_str, bool time_stamp, bool new_line)
{
    if ( LogFile != nullptr ) {
        if ( time_stamp ) *LogFile << QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ").toUtf8().data();
        *LogFile << log_str.toUtf8().data();
        if ( new_line ) *LogFile << std::endl;
        *LogFile << std::flush;
    }
}


void Camera::LogOutput(QStringList &log_strs)
{
    if ( LogFile != nullptr ) {
        foreach (QString str, log_strs) {
            LogOutput(str,false);
        }
        LogOutput("",false);
    }
}


            /*  protected slots  */

void Camera::ExposureCounter()
{
    currentExposureClock -= CAMERA_TIMER_RESOLUTION;
    if ( currentExposureClock >= 0 ) { // protect aganist negative value
        if ( currentExposureClock < CAMERA_TIMER_RESOLUTION ) currentExposureClock = 0.0;
    } else  currentExposureClock = 0.0;
    emit ExposureClock(currentExposureClock);
    if ( currentExposureClock == 0.0 ) {
        exp_timer->stop();
        cameraStatus = CAMERA_STATUS_READING_TEXT;
        emit CameraStatus(cameraStatus);
    }

}


void Camera::SaveFITS()
{
    exp_timer->stop();
    emit ExposureClock(0.0);

    int xsize, ysize;

    LogOutput("   [CAMERA] Save FITS image");
    ANDOR_API_CALL(GetDetector,&xsize,&ysize);


}

            /*  private methods  */

//void Camera::Call_Andor_API(unsigned int err_code, const char *file, int line)
//{
//    bool isSuccess = err_code == DRV_SUCCESS;
//    if ( isSuccess ) {
//        *LogFile << "Andor API: success " << ", in " << file << " at line " << line;
//    } else {
//        *LogFile << "Andor API: error = " << err_code << ", in " << file << " at line " << line;
//    }
//    *LogFile << std::endl << std::flush;
//}

//void Camera::Call_Andor_API(unsigned int err_code, const char *api_func, const char *file, int line)
//{
//    bool isSuccess = err_code == DRV_SUCCESS;
//    if ( isSuccess ) {
//        *LogFile << "Andor API: [" << api_func << "]: OK " << " (in " << file
//                 << " at line " << line << ")";
//    } else {
//        *LogFile << "Andor API: [" << api_func << "]: error = " << err_code
//                 << " (in " << file << " at line " << line << ")";
//    }
//    *LogFile << std::endl << std::flush;
//}

