#include "statuspollingthread.h"

StatusPollingThread::StatusPollingThread(Camera *cam, unsigned int poll_int):
    QThread(), camera(cam), polling_interval(poll_int), stop_thread(false)
//  QThread(cam), camera(cam), polling_interval(poll_int), stop_thread(false)
{

}


void StatusPollingThread::setInterval(unsigned long poll_int)
{
    polling_interval = poll_int;
    if ( isRunning() ) {
        stop();
        wait(2000);
        start();
    }
}


void StatusPollingThread::stop()
{
    stop_thread = true;
}


void StatusPollingThread::run()
{
    stop_thread = false;

#ifdef EMULATOR_MODE
    while ( (camera->currentExposureClock > 0.0) && !stop_thread ) {
        QThread::msleep(polling_interval);
    }
#else
    ret_status = GetStatus(&camera_status);
    if ( ret_status != DRV_SUCCESS ) {
        camera->lastError = ret_status;
        emit camera->CameraError(camera->lastError);
        exit(ret_status);
    }

    while ( (camera_status == DRV_ACQUIRING) && !stop_thread ) {
        ret_status = GetStatus(&camera_status);
        if ( ret_status != DRV_SUCCESS ){
            camera->lastError = ret_status;
            emit camera->CameraError(camera->lastError);
            exit(ret_status);
        }
        QThread::msleep(polling_interval);
    }

    emit Camera_IDLE();
#endif
}
