#include "temppollingthread.h"

TempPollingThread::TempPollingThread(Camera *cam, unsigned long poll_int, QObject *parent):
    QThread(parent), camera(cam), polling_interval(poll_int), stop_thread(false)
{
    if ( polling_interval == 0 ) ++polling_interval;
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

void TempPollingThread::run()
{
    QMutexLocker lock(&camera->tempMutex); // lock mutex

    stop_thread = false;

    float temp;
    unsigned int cool;

    while ( !stop_thread ) {
        cool = GetTemperatureF(&temp);

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
