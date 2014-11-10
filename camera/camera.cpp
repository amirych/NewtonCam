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

//time_point = std::time(nullptr); \
//*LogFile << std::asctime(std::localtime(&time_point)); \
//date_str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: "); \
//*LogFile << date_str.toUtf8().data(); \

// current time point macro. It returns a char* string
#define TIME_POINT QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ").toUtf8().data()


#define ANDOR_API_CALL(API_FUNC, ...) { \
    lastError = API_FUNC(__VA_ARGS__); \
    emit CameraError(lastError); \
    if ( LogFile != nullptr ) { \
        *LogFile << TIME_POINT; \
        *LogFile << "   [Andor API] " << #API_FUNC << "(" << #__VA_ARGS__ << "): "; \
        if ( lastError == DRV_SUCCESS ) { \
            *LogFile << "OK "; \
        } else { \
            *LogFile << "error = " << lastError; \
        } \
        *LogFile << " (in " << __FILE__ << " at line " << __LINE__ << ")" << std::endl << std::flush; \
    }\
}




            /********************************
            *                               *
            *  Camera class implementation  *
            *                               *
            ********************************/

            /*  constructors and destructor  */

Camera::Camera(std::ostream &log_file, long camera_index, QObject *parent):
    QObject(parent), Camera_Index(camera_index), LogFile(nullptr),
    tempPolling(nullptr), statusPolling(nullptr),
    tempPollingInterval(CAMERA_DEFAULT_TEMP_POLLING_INT),
    statusPollingInterval(CAMERA_DEFAULT_STATUS_POLLING_INT)
{
#ifdef QT_DEBUG
    qDebug() << "Create Camera";
#endif

    LogFile = &log_file;

    emit CameraStatus(CAMERA_STATUS_UNINITILIZED_TEXT);
//    LogFile = nullptr;

}


Camera::Camera(QObject *parent): Camera(std::cerr,0,parent)
{
}


Camera::Camera(long camera_index, QObject *parent): Camera(std::cerr, camera_index, parent)
{
}


Camera::~Camera()
{
    tempPolling->stop();
    statusPolling->stop();

    tempPolling->wait(2000);
    statusPolling->wait(2000);

    ANDOR_API_CALL(ShutDown,);
    if ( LogFile != nullptr ) {
        *LogFile << TIME_POINT << "Stop camera.\n";
//        date_str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ");
//        *LogFile << date_str.toUtf8().data() << "Stop camera.\n";
    }
}

            /*  public methods  */

void Camera::InitCamera(std::ostream &log_file, long camera_index)
{
    LogFile = &log_file;
    Camera_Index = camera_index;

    lastError = DRV_SUCCESS;

    emit CameraStatus(CAMERA_STATUS_UNINITILIZED_TEXT);

    at_32 no_cameras;
    char sdk_ver[100];

    emit CameraStatus(CAMERA_STATUS_INIT_TEXT);

    // log header
    if ( LogFile != nullptr ) {
        char* sp = "            ";
        *LogFile << "\n\n\n";
        *LogFile << sp  << "***************************************************\n";
        *LogFile << sp  << "*                                                 *\n";
        *LogFile << sp  << "* NewtonCam: Andor Newton camera control software *\n";
        *LogFile << sp  << "*                                                 *\n";
        QString str1,str2;
        str1.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
        str2.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);
        str1 += "." + str2;
        str2 = "* Version:                                        *\n";
        str2.replace(11,str1.length(),str1);
        *LogFile << sp  << str2.toUtf8().data();

        lastError = GetVersionInfo(AT_SDKVersion,sdk_ver,100);
        str1 = sdk_ver;
        str2 = "* Andor SDK version:                              *\n";
        str2.replace(21,str1.length(),str1);
        *LogFile << sp  << str2.toUtf8().data();

//        lastError = GetVersionInfo(AT_DeviceDriverVersion,sdk_ver,100);
//        str1 = sdk_ver;
//        str2 = "* Andor device driver version:                    *\n";
//        str2.replace(31,str1.length(),str1);
//        *LogFile << str2.toUtf8().data();

        *LogFile << sp  << "*                                                 *\n";
        *LogFile << sp  << "***************************************************\n";

        *LogFile << "\n";

        *LogFile << TIME_POINT << "Starting camera ...\n\n";
//        str1 = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ");
//        *LogFile << str1.toUtf8().data() << "Starting camera ...\n\n";
    }

    ANDOR_API_CALL(GetAvailableCameras,&no_cameras);
#ifdef QT_DEBUG
    qDebug() << "CAMERA: Number of found cameras: " << no_cameras;
#endif

#ifdef EMULATOR
    if ( LogFile != nullptr ) {
        *LogFile << TIME_POINT << "EMULATOR: start emulation mode!\n";
    }
#else
    if ( !no_cameras || (lastError != DRV_SUCCESS) ) { // camera is not detected!
        if ( LogFile != nullptr ) {
            *LogFile << TIME_POINT << "Cannot detect any cameras!\n";
            lastError = DRV_GENERAL_ERRORS;
            return;
        }
    }

    ANDOR_API_CALL(Initialize,"");
    std::this_thread::sleep_for(std::chrono::seconds(2)); // wait for init proccess finished

    ANDOR_API_CALL(GetVersionInfo,AT_DeviceDriverVersion,sdk_ver,100);
#endif

    // start CCD chip temperaturepolling

    if ( tempPolling == nullptr ) {
        tempPolling = new TempPollingThread(this,tempPollingInterval);
    } else {
        tempPolling->stop();
    }

    tempPolling->start();

    if ( statusPolling == nullptr ) {
        statusPolling = new StatusPollingThread(this,statusPollingInterval);
    }

    emit CameraStatus(CAMERA_STATUS_READY_TEXT);

#ifdef EMULATOR
    tempSetPoint = -30.0;
#endif
}


void Camera::InitCamera(long camera_index)
{
    InitCamera(std::cerr,camera_index);
}

unsigned int Camera::GetLastError() const
{
    return lastError;
}


void Camera::SetPollingIntervals(const unsigned long temp_int, const unsigned long status_int)
{
    tempPollingInterval = temp_int;
    statusPollingInterval = status_int;
}


void Camera::SetCoolerOFF()
{
    ANDOR_API_CALL(CoolerOFF,);
}


void Camera::SetCoolerON()
{
    ANDOR_API_CALL(CoolerON,);
}


void Camera::SetCCDTemperature(const int temp)
{
#ifdef EMULATOR
    if ( LogFile != nullptr ) {
        tempSetPoint = temp;
        *LogFile << TIME_POINT << "EMULATOR: Set CCD temperature: " << temp << " degrees\n";
    }
#else
    ANDOR_API_CALL(SetTemperature,temp);
#endif
}


void Camera::GetCCDTemperature(double *temp, unsigned int *cooler_stat)
{
    QMutexLocker lock(&tempMutex);

#ifdef EMULATOR
    *temp = currentTemperature;
    *cooler_stat = currentCoolerStatus;
#else
    float curr_temp;
    currentCoolerStatus = GetTemperatureF(&curr_temp);
    currentTemperature = static_cast<double>(curr_temp);
    cooler_stat = currentCoolerStatus;
    *temp = currentTemperature;
#endif
}

void Camera::SetExpTime(const double exp_time)
{
#ifdef EMULATOR
    currentExpTime = exp_time;
#else
    ANDOR_API_CALL(SetExposureTime,exp_time);
#endif
}

            /*  public slots  */

void Camera::StartExposure(const QString &fits_filename, const QString &hdr_filename)
{
#ifdef EMULATOR
    currentStatus = DRV_ACQUIRING;
#else
    ANDOR_API_CALL(StartAcquisition,);
#endif
    if ( lastError == DRV_SUCCESS ) {
        statusPolling->start();
    }
}


void Camera::StopExposure()
{
    ANDOR_API_CALL(AbortAcquisition,);
    statusPolling->stop();
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
