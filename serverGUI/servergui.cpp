#include "servergui.h"

#ifdef Q_OS_WIN
    #include "../AndorSDK/atmcd32d.h"
#endif

#ifdef Q_OS_LINUX
    #include "../AndorSDK/atmcdLXd.h"
#endif
//#ifdef _WIN32 || _WIN64
//    #include "../AndorSDK/atmcd32d.h"
//#else
//    #include "../AndorSDK/atmcdLXd.h"
//#endif

#include <QDebug>
#include <QFontMetrics>

#define TEMPLAB_ROOT_TEXT    "<b>  CCD Temp: </b>"
#define COOLERLAB_ROOT_TEXT  "<b>  Cooler status: </b>"
#define ERRLAB_ROOT_TEXT     "<b>  Err: </b>"
#define NETLAB_ROOT_TEXT     "<b>  Net: </b>"


#define TEMPLAB_INIT_TEXT    TEMPLAB_ROOT_TEXT"No connection  "
#define COOLERLAB_INIT_TEXT  COOLERLAB_ROOT_TEXT"No connection  "
#define ERRLAB_INIT_TEXT     ERRLAB_ROOT_TEXT"OK  "
#define NETLAB_INIT_TEXT     NETLAB_ROOT_TEXT"OK  "
#define CAM_STATUS_INIT_TEXT "<font color=red> Unknown </font>"
//#define TEMPLAB_INIT_TEXT    "<b>  CCD Temp: </b> No connection  "
//#define COOLERLAB_INIT_TEXT  "<b>  Cooler status: </b> No connection  "
//#define ERRLAB_INIT_TEXT     "<b>  Err: </b>OK  "
//#define NETLAB_INIT_TEXT     "<b>  Net: </b> OK  "


ServerGUI::ServerGUI(int fontsize, QWidget *parent): QMainWindow(parent),
    fontSize(fontsize), statusFontSize(fontsize-2), logFontSize(fontsize)
{
    QFont font("Arial");

    font.setStyleHint(QFont::System);
    font.setHintingPreference(QFont::PreferFullHinting);

    font.setPointSize(fontSize);
    font.setBold(false);


    QFrame* cWidget = new QFrame(this);
    this->setCentralWidget(cWidget);
    cWidget->setFrameShape(QFrame::Box);
    cWidget->setFrameShadow(QFrame::Raised);
//    cWidget->setFont(font);
    this->setFont(font);

    font.setPointSize(statusFontSize);
    statusBar()->setFont(font);
    temperature_label = new QLabel(TEMPLAB_INIT_TEXT,cWidget);
    temperature_label->setMargin(3);
    statusBar()->addWidget(temperature_label);

    cooler_label = new QLabel(COOLERLAB_INIT_TEXT,cWidget);
    statusBar()->addWidget(cooler_label);

    camera_err_label = new QLabel(ERRLAB_INIT_TEXT,cWidget);
    statusBar()->addPermanentWidget(camera_err_label);

    network_err_label = new QLabel(NETLAB_INIT_TEXT,cWidget);
    statusBar()->addPermanentWidget(network_err_label);

    QVBoxLayout* main_layout = new QVBoxLayout(cWidget);

    QGroupBox* oper_control_gb = new QGroupBox("Operation Control",cWidget);

    main_layout->addWidget(oper_control_gb);

    QGroupBox* exp_progress_gb = new QGroupBox("Exposure Progress",oper_control_gb);
    QGroupBox* cam_status_gb = new QGroupBox("Camera Status",oper_control_gb);

    QVBoxLayout* exp_progress_layout = new QVBoxLayout(exp_progress_gb);
    QVBoxLayout* cam_status_layout = new QVBoxLayout(cam_status_gb);

    exp_progress = new QLCDNumber(exp_progress_gb);
    exp_progress->setSegmentStyle(QLCDNumber::Flat);
    exp_progress->setSmallDecimalPoint(true);
    exp_progress->display(0.0);
    exp_progress->setFixedHeight(40);
    exp_progress_layout->addWidget(exp_progress);

    cam_status_label = new QLabel(CAM_STATUS_INIT_TEXT,cam_status_gb);
    cam_status_label->setFrameShape(QFrame::Box);
    cam_status_label->setAlignment(Qt::AlignCenter);
    cam_status_layout->addWidget(cam_status_label);

    QHBoxLayout* oper_control_layout = new QHBoxLayout(oper_control_gb);
    oper_control_layout->addWidget(exp_progress_gb);
    oper_control_layout->addWidget(cam_status_gb);


    QGroupBox* log_gb = new QGroupBox("Server Log",cWidget);

    QVBoxLayout* log_layout = new QVBoxLayout(log_gb);

    log_window = new QTextEdit(log_gb);
    log_window->setReadOnly(true);
//    log_window->setFixedWidth(500);
    log_window->setFixedWidth(statusBarWidth());
    log_layout->addWidget(log_window);

    font.setPointSize(logFontSize);
    log_window->setFont(font);

    main_layout->addWidget(log_gb);

    this->layout()->setSizeConstraint(QLayout::SetFixedSize);
}

ServerGUI::ServerGUI(QWidget *parent): ServerGUI(SERVERGUI_DEFAULT_FONTSIZE,parent)
{
}


ServerGUI::~ServerGUI()
{
    qDebug() << "ServerGUI is destroyed!";
}


void ServerGUI::SetFonts(int fontsize, int status_fontsize, int log_fontsize)
{
    fontSize = fontsize;
    statusFontSize = status_fontsize;
    logFontSize = log_fontsize;

    QFont font = this->font();

    font.setPointSize(fontSize);
    this->setFont(font);

    font.setPointSize(statusFontSize);
    statusBar()->setFont(font);

    font.setPointSize(logFontSize);
    log_window->setFont(font);

    log_window->setFixedWidth(statusBarWidth());

    this->repaint();
}


void ServerGUI::Reset()
{
    temperature_label->setText(TEMPLAB_INIT_TEXT);
    cooler_label->setText(COOLERLAB_INIT_TEXT);
    cam_status_label->setText(CAM_STATUS_INIT_TEXT);
    camera_err_label->setText(ERRLAB_INIT_TEXT);
    network_err_label->setText(NETLAB_INIT_TEXT);
    exp_progress->display(0.0);
}


void ServerGUI::LogMessage(QString msg)
{
    log_window->append(msg);
}


void ServerGUI::TempChanged(double temp)
{
    QString temp_str;
    temp_str.setNum(temp,'f',2);
    temp_str.prepend(TEMPLAB_ROOT_TEXT);

    temperature_label->setText(temp_str);
}


void ServerGUI::CoolerStatusChanged(unsigned int status)
{
    QString str = COOLERLAB_ROOT_TEXT;

    switch ( status ) {
        case DRV_NOT_INITIALIZED: {
            str += "<b><font color=red> NOT INIT </font></b>";
            break;
        }
        case DRV_ACQUIRING: {
            str += "<b><font color=red> ACQUIRING </font></b>";
            break;
        }
        case DRV_ERROR_ACK: {
            str += "<b><font color=red> DEVICE ERROR </font></b>";
            break;
        }
        case DRV_TEMP_OFF: {
            str += "<b><font color=red> OFF </font></b>";
            break;
        }
        case DRV_TEMP_STABILIZED: {
            str += "<b><font color=green> STABILIZED </font></b>";
            break;
        }
        case DRV_TEMP_NOT_REACHED: {
            str += "<b><font color=red> NOT REACHED </font></b>";
            break;
        }
        case DRV_TEMP_DRIFT: {
            str += "<b><font color=brown> DRIFT </font></b>";
            break;
        }
        case DRV_TEMP_NOT_STABILIZED: {
            str += "<b><font color=brown> NOT STABILIZED </font></b>";
            break;
        }
        default: {
            str += "<font color=red> UNKNOWN </font>";
            break;
        }
    }

    cooler_label->setText(str);
}


void ServerGUI::ServerError(unsigned int err_code)
{
    QString str;

    if ( err_code != DRV_SUCCESS ) {
        str.setNum(err_code);
    } else str = "OK";

    str.prepend(ERRLAB_ROOT_TEXT);

    camera_err_label->setText(str);
}


void ServerGUI::NetworkError(QAbstractSocket::SocketError err)
{
    QString str;
    if ( err < 1000 ) str.setNum(err); else str = " OK";

    str.prepend(NETLAB_ROOT_TEXT);

    network_err_label->setText(str);
}


void ServerGUI::ServerStatus(QString status)
{
    cam_status_label->setText(status);
}


void ServerGUI::ExposureProgress(double progress)
{
    exp_progress->display(progress);
}

int ServerGUI::statusBarWidth()
{
    QFont font = statusBar()->font();

    QFontMetrics fm(font);

    int width = fm.width(TEMPLAB_INIT_TEXT);
    width += fm.width(COOLERLAB_INIT_TEXT);
    width += fm.width(ERRLAB_INIT_TEXT);
    width += fm.width(NETLAB_INIT_TEXT);
    width /= 1.3;
//    qDebug() << "width = " << width;

    return width;
}
