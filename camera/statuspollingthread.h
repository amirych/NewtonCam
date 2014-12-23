#ifndef STATUSPOLLINGTHREAD_H
#define STATUSPOLLINGTHREAD_H

#include "camera.h"

#include <QThread>


class StatusPollingThread : public QThread
{
    Q_OBJECT
public:
    StatusPollingThread(Camera *cam, unsigned int poll_int = CAMERA_DEFAULT_STATUS_POLLING_INT);

    void setInterval(unsigned long poll_int);

    void stop();
    void run();

signals:
    void Camera_IDLE();

private:
    Camera *camera;
    unsigned long polling_interval;
    bool stop_thread;

    unsigned int ret_status;
    int camera_status;
};

#endif // STATUSPOLLINGTHREAD_H
