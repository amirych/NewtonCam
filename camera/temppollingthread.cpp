#include "temppollingthread.h"

#ifdef EMULATOR
#include<cmath>
#endif

TempPollingThread::TempPollingThread(Camera *cam, unsigned long poll_int):
    QThread(cam), camera(cam), polling_interval(poll_int), stop_thread(false)
{
    if ( polling_interval == 0 ) ++polling_interval;

#ifdef EMULATOR
    camera->currentTemperature = 20.0;
#endif
}


void TempPollingThread::setPollingInterval(unsigned long poll_int)
{
    polling_interval = (poll_int == 0 ) ? 1: poll_int;
    stop();
    start();
}


void TempPollingThread::stop()
{
    stop_thread = true;
}

// The Camera::GetTemperature function is not used here to avoid massive logging (see Camera::GetTemperature implementation)
void TempPollingThread::run()
{
    QMutexLocker lock(&camera->tempMutex); // lock mutex

    stop_thread = false;

    float temp;
    unsigned int cool;

    while ( !stop_thread ) {
#ifdef EMULATOR
        temp = camera->currentTemperature;

        double diff = abs(temp - camera->tempSetPoint);

        if ( temp > (camera->tempSetPoint+1.5) ) {
            cool = DRV_TEMP_NOT_REACHED;
            temp -= 4.7;
        } else if ( (diff <= 1.5) && (diff >= 0.5) ) {
            cool = DRV_TEMP_NOT_STABILIZED;
            temp -= 0.1;
        } else if ( (diff < 0.5) && (diff > 0.1) ) {
            cool = DRV_TEMP_DRIFT;
            temp -= 0.05;
        } else {
            cool = DRV_TEMP_STABILIZED;
            temp = camera->tempSetPoint;
        }
#else
        cool = GetTemperatureF(&temp);
#endif

        if ( camera->currentTemperature != temp ) {
            camera->currentTemperature = temp;
            emit camera->TemperatureChanged(camera->currentTemperature);
        }
        if ( cool != camera->currentCoolerStatus ) {
            camera->currentCoolerStatus  = cool;
            emit camera->CoolerStatusChanged(cool);
        }

        // wait for polling interval
        for ( unsigned int tick = 1; (tick <= polling_interval) && !stop_thread; ++tick) QThread::sleep(1);
    }
}
