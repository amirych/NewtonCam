#include "servergui.h"

#include <QDebug>


ServerGUI::ServerGUI(int fontsize, QWidget *parent): QMainWindow(parent),
    fontSize(fontsize)
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
    cWidget->setFont(font);

    temperature_label = new QLabel("<b>  CCD Temp: </b> No connection  ",cWidget);
    statusBar()->addWidget(temperature_label);

    cooler_label = new QLabel("<b>  Cooler status: </b> No connection  ",cWidget);
    statusBar()->addWidget(cooler_label);

    camera_err_label = new QLabel("<b>  Err: </b>OK  ",cWidget);
    statusBar()->addPermanentWidget(camera_err_label);

    network_err_label = new QLabel("<b>  Net: </b> OK  ",cWidget);
    statusBar()->addPermanentWidget(network_err_label);

    statusBar()->setFont(font);

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

    cam_status_label = new QLabel("<font color=red> Ready </font>",cam_status_gb);
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
    log_window->setFixedWidth(500);
    log_layout->addWidget(log_window);

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


void ServerGUI::LogMessage(QString msg)
{
    log_window->append(msg);
}
