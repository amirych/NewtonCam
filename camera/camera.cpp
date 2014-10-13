#include "camera.h"

#include "atmcdLXd.h"


            /* Andor API wrapper macro definition */

#define ANDOR_API_CALL(API_FUNC, ...) { \
    lastError = API_FUNC(__VA_ARGS__); \
    emit CameraError(lastError); \
    time_point = std::time(nullptr); \
    *LogFile << std::asctime(std::localtime(&time_point)); \
    *LogFile << "   [Andor API] " << #API_FUNC << "(" << #__VA_ARGS__ << "): "; \
    if ( lastError == DRV_SUCCESS ) { \
        *LogFile << "OK "; \
    } else { \
        *LogFile << "error = " << lastError; \
    } \
    *LogFile << " (in " << __FILE__ << " at line " << __LINE__ << ")" << std::endl << std::flush; \
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

    int no_cameras;

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
    ANDOR_API_CALL(ShutDown);
}

            /*  public methods  */

unsigned int Camera::GetLastError() const
{
    return lastError;
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
