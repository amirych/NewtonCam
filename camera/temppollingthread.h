#ifndef TEMPPOLLINGTHREAD_H
#define TEMPPOLLINGTHREAD_H

#include "camera.h"

#include <QThread>
#include <QMutexLocker>

class TempPollingThread : public QThread
{
public:
    TempPollingThread(Camera* cam, unsigned long poll_int = CAMERA_DEFAULT_TEMP_POLLING_INT, QObject *parent = 0);

    void setPollingInterval(unsigned long poll_int);
    void run();
    void stop();

private:
    unsigned long polling_interval; // in seconds
    Camera *camera;
    bool stop_thread;
};

#endif // TEMPPOLLINGTHREAD_H
