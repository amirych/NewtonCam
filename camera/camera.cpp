#include "../version.h"
#include "camera.h"

//#ifdef _WIN32 || _WIN64
//    #include "atmcd32d.h"
//#else
//    #include "atmcdLXd.h"
//#endif

#ifdef Q_OS_WIN
    #include "atmcd32d.h"
#endif

#ifdef Q_OS_LINUX
    #include "atmcdLXd.h"
#endif

#include <QString>

            /* Andor API wrapper macro definition */

//time_point = std::time(nullptr); \
//*LogFile << std::asctime(std::localtime(&time_point)); \

#define ANDOR_API_CALL(API_FUNC, ...) { \
    lastError = API_FUNC(__VA_ARGS__); \
    emit CameraError(lastError); \
    if ( LogFile != nullptr ) { \
        date_str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: "); \
        *LogFile << date_str.toUtf8().data(); \
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
    QObject(parent), Camera_Index(camera_index), LogFile(nullptr)
{
    LogFile = &log_file;

    at_32 no_cameras;

    if ( LogFile != nullptr ) {
        *LogFile << "\n\n\n";
        *LogFile << "***************************************************\n";
        *LogFile << "*                                                 *\n";
        *LogFile << "* NewtonCam: Andor Newton camera control software *\n";
        *LogFile << "*                                                 *\n";
        QString str1,str2;
        str1.setNum(NEWTONCAM_PACKAGE_VERSION_MAJOR);
        str2.setNum(NEWTONCAM_PACKAGE_VERSION_MINOR);
        str1 += "." + str2;
        str2 = "* Version:                                        *\n";
        str2.replace(11,str1.length(),str1);
        *LogFile << str2.toUtf8().data();

        char sdk_ver[100];
        lastError = GetVersionInfo(AT_SDKVersion,sdk_ver,100);
        str1 = sdk_ver;
        str2 = "* Andor SDK version:                              *\n";
        str2.replace(21,str1.length(),str1);
        *LogFile << str2.toUtf8().data();

        lastError = GetVersionInfo(AT_DeviceDriverVersion,sdk_ver,100);
        str1 = sdk_ver;
        str2 = "* Andor device driver version:                    *\n";
        str2.replace(31,str1.length(),str1);
        *LogFile << str2.toUtf8().data();

        *LogFile << "*                                                 *\n";
        *LogFile << "***************************************************\n";

        *LogFile << "\n";

        str1 = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ");
        *LogFile << str1.toUtf8().data() << "Starting camera ...\n\n";
    }

    ANDOR_API_CALL(GetAvailableCameras,&no_cameras);
    ANDOR_API_CALL(Initialize,"");
    std::cout << "Number of cameras: " << no_cameras << std::endl;
}


Camera::Camera(QObject *parent): Camera(std::cerr,0,parent)
{
}


Camera::Camera(long camera_index, QObject *parent): Camera(std::cerr, camera_index, parent)
{
}


Camera::~Camera()
{
    ANDOR_API_CALL(ShutDown,);
    if ( LogFile != nullptr ) {
        date_str = QDateTime::currentDateTime().toString(" dd-MM-yyyy hh:mm:ss: ");
        *LogFile << date_str.toUtf8().data() << "Stop camera.\n";
    }
}

            /*  public methods  */

void Camera::InitCamera(long camera_index)
{

}

unsigned int Camera::GetLastError() const
{
    return lastError;
}


void Camera::SetCoolerOFF()
{
    ANDOR_API_CALL(CoolerOFF,);
}


            /*  public slots  */

void Camera::StartExposure(const QString &fits_filename, const QString &hdr_filename)
{
}


void Camera::StopExposure()
{
}




            /*  private methods  */

void Camera::Call_Andor_API(unsigned int err_code, const char *file, int line)
{
    bool isSuccess = err_code == DRV_SUCCESS;
    if ( isSuccess ) {
        *LogFile << "Andor API: success " << ", in " << file << " at line " << line;
    } else {
        *LogFile << "Andor API: error = " << err_code << ", in " << file << " at line " << line;
    }
    *LogFile << std::endl << std::flush;
}

void Camera::Call_Andor_API(unsigned int err_code, const char *api_func, const char *file, int line)
{
    bool isSuccess = err_code == DRV_SUCCESS;
    if ( isSuccess ) {
        *LogFile << "Andor API: [" << api_func << "]: OK " << " (in " << file
                 << " at line " << line << ")";
    } else {
        *LogFile << "Andor API: [" << api_func << "]: error = " << err_code
                 << " (in " << file << " at line " << line << ")";
    }
    *LogFile << std::endl << std::flush;
}
