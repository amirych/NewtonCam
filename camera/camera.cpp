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

#include <fitsio.h>

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


#define CFITSIO_API_CALL(API_FUNC, ...) {\
    API_FUNC(__VA_ARGS__); \
    if ( LogFile != nullptr ) { \
        *LogFile << TIME_STAMP; \
        *LogFile << " [FITS API] " << #API_FUNC << "(" << #__VA_ARGS__ << "): "; \
        if ( !fits_status ) { \
            *LogFile << "OK "; \
        } else { \
            *LogFile << "error = " << fits_status; \
        } \
        *LogFile << " (in " << __FILE__ << " at line " << __LINE__ << ")" << std::endl << std::flush; \
    }\
    if ( fits_status ) { \
        lastError = CAMERA_FITS_ERROR_BASE + fits_status;\
        throw lastError;\
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
    headModel(""), driverVersion(""),
    currentRate(""), currentGain(""),
    currentImage_buffer(nullptr),
    startExposureTime(QDateTime()),
    isStopped(false)
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

    if ( currentImage_buffer != nullptr ) delete currentImage_buffer;
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
        if ( lastError == DRV_SUCCESS ) lastError = CAMERA_ERROR_NO_DEVICE;
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

#ifdef Q_OS_WIN
    char head_model[MAX_PATH];
#endif

// MAX_PATH was not declared in Andor header file!!!
#ifdef Q_OS_LINUX
    char head_model[1024];
#endif

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

    unsigned int dummy;

    ANDOR_API_CALL(GetHardwareVersion,&dummy,&dummy,&dummy,&dummy,&firmwareVersion,&firmwareBuild);
    str.setNum(firmwareVersion);
    LogOutput("   [CAMERA] Firmware version: " + str);
    str.setNum(firmwareBuild);
    LogOutput("   [CAMERA] Firmware build number: " + str);
#endif


    // some camera defined constants

    ANDOR_API_CALL(GetTemperatureRange,temperatureRange,temperatureRange+1);
    QString temprange;

    temprange.setNum(temperatureRange[0]);
    str.setNum(temperatureRange[1]);
    temprange = "[" + temprange + ", " + str + "]";
    LogOutput("   [CAMERA] Allowed temperature range: " + temprange);

    ANDOR_API_CALL(GetMaximumBinning,4,0,maxBinning);
    ANDOR_API_CALL(GetMaximumBinning,4,1,maxBinning+1);
    QString maxbin;

    maxbin.setNum(maxBinning[0]);
    str.setNum(maxBinning[1]);
    maxbin += "x" + str;
    LogOutput("   [CAMERA] Maximal binning: " + maxbin);

    // initial values for image mode, size and binning

    // Currently, the software supports only Single Scan in Image Mode

    ANDOR_API_CALL(SetAcquisitionMode,1); // single scan
    ANDOR_API_CALL(SetReadMode,4); // image mode! ()
    ANDOR_API_CALL(GetDetector,&currentImage_Xsize,&currentImage_Ysize);
    detector_Xsize = currentImage_Xsize;
    detector_Ysize = currentImage_Ysize;
    currentImage_startX = 1;
    currentImage_startY = 1;
    currentImage_binX = 1;
    currentImage_binY = 1;
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

    LogOutput("   [CAMERA] Initialization was complete!\n");

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
#ifndef EMULATOR_MODE
    ANDOR_API_CALL(SetExposureTime,exp_time);
#endif
}


//
// Parameters: hbin, vbin - binning along X and Y axes, accordingly
//             xstart, ystart - upper-left conner of the readout region in term of sensor pixels
//             xsize, ysize - size of readout region (width and height) in term of binned pixels!
//
void Camera::SetFrame(const int hbin, const int vbin, const int xstart, const int ystart, const int xsize, const int ysize)
{
    int startX = xstart;
    int startY = ystart;

    if ( startX < 1 ) startX = 1;
    if ( startY < 1 ) startY = 1;

    int endX = startX + hbin*xsize - 1; // include last pixel
    int endY = startY + vbin*ysize - 1;

    if ( endX > detector_Xsize ) endX = detector_Xsize - ((detector_Xsize - startX + 1) % hbin);
    if ( endY > detector_Ysize ) endY = detector_Ysize - ((detector_Ysize - startY + 1) % vbin);

    currentImage_startX = startX;
    currentImage_startY = startY;


    if ( hbin < 1 ) currentImage_binX = 1; else currentImage_binX = hbin;
    if ( vbin < 1 ) currentImage_binY = 1; else currentImage_binY = vbin;

    currentImage_Xsize = (endX - startX)/currentImage_binX + 1;
    currentImage_Ysize = (endY - startY)/currentImage_binY + 1;

    ANDOR_API_CALL(SetImage,currentImage_binX,currentImage_binY,xstart,endX,ystart,endY);
}


void Camera::SetCCDGain(const QString gain_str)
{
    currentGain = gain_str.trimmed().toUpper();

    int idx;
    if ( currentGain == CAMERA_GAIN0_STR ) {
        idx = 0;
    } else if ( currentGain == CAMERA_GAIN1_STR ) {
        idx = 1;
    } else if ( currentGain == CAMERA_GAIN2_STR ) {
        idx = 2;
    } else {        
        currentGain = CAMERA_GAIN2_STR;
        idx = 2;
    }

    ANDOR_API_CALL(SetPreAmpGain,idx);
}


void Camera::SetRate(const QString rate_str)
{
    currentRate = rate_str.trimmed().toUpper();

    int idx;

    if ( currentRate == CAMERA_READOUT_SPEED0_STR ) {
        idx = 0;
    } else if ( currentRate == CAMERA_READOUT_SPEED1_STR ) {
        idx = 1;
    } else if ( currentRate == CAMERA_READOUT_SPEED2_STR ) {
        idx = 2;
    } else {
        currentRate = CAMERA_READOUT_SPEED2_STR;
        idx = 2;
    }

    ANDOR_API_CALL(SetHSSpeed,0,idx);
}


            /*  public slots  */

void Camera::StartExposure(const QString &fits_filename, const QString &hdr_filename)
{
#ifdef QT_DEBUG
    qDebug() << "[CAMERA] Start exposure! FITS file: " << fits_filename;
#endif
    if ( fits_filename.isEmpty() ) {
#ifdef QT_DEBUG
        qDebug() << "[CAMERA] StartExposure: empty FITS filename!";
#endif
        lastError = CAMERA_ERROR_FITS_FILENAME;
        emit CameraError(lastError);
        return;
    }

    currentFITS_Filename = fits_filename;
    currentHDR_Filename = hdr_filename;

    isStopped = false;
    startExposureTime = QDateTime::currentDateTimeUtc();

#ifdef EMULATOR_MODE
    currentStatus = DRV_ACQUIRING;
    lastError = DRV_SUCCESS;
#else
    ANDOR_API_CALL(StartAcquisition,);
#endif
    if ( lastError == DRV_SUCCESS ) {
        connect(statusPolling,SIGNAL(Camera_IDLE()),this,SLOT(SaveFITS()));
        statusPolling->start();
        cameraStatus = CAMERA_STATUS_ACQUISITION_TEXT;
        emit CameraStatus(cameraStatus);
        exp_timer->start(CAMERA_TIMER_RESOLUTION*1000);
    }
}


void Camera::StopExposure()
{
#ifdef EMULATOR_MODE
    currentStatus = DRV_SUCCESS;
#else
    ANDOR_API_CALL(AbortAcquisition,);
#endif
    isStopped = true;
    exp_timer->stop();
    statusPolling->stop();
    statusPolling->disconnect();
    emit ExposureClock(0.0);
    cameraStatus = CAMERA_STATUS_ABORT_TEXT;
    emit CameraStatus(cameraStatus);
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

    statusPolling->disconnect();

    cameraStatus = CAMERA_STATUS_READING_TEXT;
    emit CameraStatus(cameraStatus);

    LogOutput("   [CAMERA] Reading image ...");

    // allocate memory for image buffer
    unsigned long buffer_size = currentImage_Xsize*currentImage_Ysize;

    fitsfile* fptr = NULL;
    int fits_status = 0;

    try {
        if ( currentImage_buffer != nullptr ) delete currentImage_buffer;
#ifdef QT_DEBUG
        qDebug() << "[CAMERA] Current image size: [" << currentImage_Xsize << ", " << currentImage_Ysize << "]";
        qDebug() << "[CAMERA] Current start pixel: [" << currentImage_startX << ", " << currentImage_startY << "]";
        qDebug() << "[CAMERA] Allocate buffer for " << buffer_size << " pixels";
#endif
        currentImage_buffer = new buffer_t[buffer_size];

        ANDOR_API_CALL(GetAcquiredData16,currentImage_buffer,buffer_size);
        if ( lastError != DRV_SUCCESS ) throw lastError;

        cameraStatus = CAMERA_STATUS_SAVING_TEXT;
        emit CameraStatus(cameraStatus);

        LogOutput("   [CAMERA] Saving FITS image ...");

        long naxis = 2; // According to the requirements specification the only 2-dim FITS images should be written
        long naxes[2];
        long start_x = currentImage_startX;
        long start_y = currentImage_startY;

        if ( currentFITS_Filename.isEmpty() ) {
#ifdef QT_DEBUG
            qDebug() << "[CAMERA] FITS filename is empty!";
#endif
            lastError = CAMERA_ERROR_FITS_FILENAME;
            throw lastError;
        }

        QString filename = "!" + currentFITS_Filename; // add '!' to rewrite existing file (see CFITSIO library)

        // first, try to open disk file ...
        CFITSIO_API_CALL(fits_create_file,&fptr,filename.toUtf8().data(),&fits_status);

        naxes[0] = currentImage_Xsize;
        naxes[1] = currentImage_Ysize;

        CFITSIO_API_CALL(fits_create_img,fptr,USHORT_IMG,naxis,naxes,&fits_status);

        // write image to FITS file
        long fpix[2] = {1,1};
        LONGLONG nelems = naxes[0]*naxes[1];

        CFITSIO_API_CALL(fits_write_pix,fptr,TUSHORT,fpix,nelems,currentImage_buffer,&fits_status);

        delete currentImage_buffer;
        currentImage_buffer = nullptr;

        // write FITS header keywords
        char str[FLEN_VALUE];
        QByteArray arr;

        // "DATE-OBS" and "DATE" FITS keywords in format required by FITS date/time format
        arr = startExposureTime.toString("yyyy-MM-ddThh-mm-ss.zzz").toUtf8(); // in ISO format
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"DATE-OBS",arr.data(),"Start of the exposure in UTC",&fits_status);

        // "ORIGIN" FITS keyword
#ifdef Q_OS_WIN
        sprintf_s(str,"%s, v%d.%d",NEWTONCAM_PACKAGE_VERSION_STR,NEWTONCAM_PACKAGE_VERSION_MAJOR,NEWTONCAM_PACKAGE_VERSION_MINOR);
#endif
#ifdef Q_OS_LINUX
        sprintf(str,"%s, v%d.%d",NEWTONCAM_PACKAGE_VERSION_STR,NEWTONCAM_PACKAGE_VERSION_MAJOR,NEWTONCAM_PACKAGE_VERSION_MINOR);
#endif
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"ORIGIN",str,"Acquisition system",&fits_status);

        // "CRVALx" keywords
        CFITSIO_API_CALL(fits_update_key,fptr,TLONG,"CRVAL1",&start_x,"Start X-coordinate in sensor pixels",&fits_status);
        CFITSIO_API_CALL(fits_update_key,fptr,TLONG,"CRVAL2",&start_y,"Start Y-coordinate in sensor pixels",&fits_status);

        // "EXPTIME" keyword
        // if exposure wass stopped then save CurrentExposureClock variable instead of user-defined value
        if ( !isStopped ) {
            CFITSIO_API_CALL(fits_update_key,fptr,TDOUBLE,"EXPTIME",&currentExpTime,"Exposure time in seconds",&fits_status);
        } else {
            double real_exp = currentExpTime - currentExposureClock;
            CFITSIO_API_CALL(fits_update_key,fptr,TDOUBLE,"EXPTIME",&real_exp,"Exposure time in seconds (aborted)",&fits_status);
        }

        // BINNING keyword
#ifdef Q_OS_WIN
        sprintf_s(str,"%dx%d",currentImage_binX,currentImage_binY);
#endif
#ifdef Q_OS_LINUX
        sprintf(str,"%dx%d",currentImage_binX,currentImage_binY);
#endif
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"BINNING",str,"Binning (XBINxYBIN)",&fits_status);


        // add camera info
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"HEADMOD",headModel.toUtf8().data(),"Camera head model",&fits_status);
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"DRIVER",driverVersion.toUtf8().data(),"Camera driver version",&fits_status);
        start_x = serialNumber;
        CFITSIO_API_CALL(fits_update_key,fptr,TLONG,"SERNUM",&start_x,"Camera serial number",&fits_status);
        start_x = firmwareVersion;
        CFITSIO_API_CALL(fits_update_key,fptr,TLONG,"FIRMVER",&start_x,"Camera firmware version",&fits_status);
        start_x = firmwareBuild;
        CFITSIO_API_CALL(fits_update_key,fptr,TLONG,"FIRMNUM",&start_x,"Camera firmware build number",&fits_status);


        // temperature and cooler status
        QString cooling_status_str;
        tempMutex->lock();
        CFITSIO_API_CALL(fits_update_key,fptr,TDOUBLE,"CCDTEMP",&currentTemperature,"CCD chip temperature in Celsius",&fits_status);
        switch ( currentCoolerStatus ) {
            case DRV_ACQUIRING: {
                cooling_status_str = "ACQUIRING";
                break;
            }
            case DRV_TEMP_OFF: {
                cooling_status_str = "OFF";
                break;
            }
            case DRV_TEMP_STABILIZED: {
                cooling_status_str = "STABILIZED";
                break;
            }
            case DRV_TEMP_NOT_REACHED: {
                cooling_status_str = "NOT REACHED";
                break;
            }
            case DRV_TEMP_DRIFT: {
                cooling_status_str = "DRIFT";
                break;
            }
            case DRV_TEMP_NOT_STABILIZED: {
                cooling_status_str = "NOT STABILIZED";
                break;
            }
            default: {
                cooling_status_str = "UNKNOWN";
                break;
            }
        }
        tempMutex->unlock();
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"COOLER",cooling_status_str.toUtf8().data(),"Cooler status",&fits_status);

        // readout rate and gain
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"RATE",currentRate.toUtf8().data(),"Readout rate",&fits_status);
        CFITSIO_API_CALL(fits_update_key,fptr,TSTRING,"GAIN",currentGain.toUtf8().data(),"CCD gain",&fits_status);

        // add user FITS keywords
        if ( !currentHDR_Filename.isEmpty() ) {
            CFITSIO_API_CALL(fits_write_key_template,fptr,currentHDR_Filename.toUtf8().data(),&fits_status);
        }

        CFITSIO_API_CALL(fits_close_file,fptr,&fits_status);

        cameraStatus = CAMERA_STATUS_READY_TEXT;
        emit CameraStatus(cameraStatus);
    } catch (std::bad_alloc &ex) {
        currentImage_buffer = nullptr;
        lastError = Camera::CAMERA_ERROR_BADALLOC;
        emit CameraError(lastError);
    } catch (unsigned int &err) {
#ifdef QT_DEBUG
        qDebug() << "[CAMERA] Catching error: " << err;
#endif
        emit CameraError(lastError);
        cameraStatus = CAMERA_STATUS_FAILURE_TEXT;
        emit CameraStatus(cameraStatus);
        delete currentImage_buffer;
        currentImage_buffer = nullptr;
        fits_close_file(fptr,&fits_status);
    }
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

