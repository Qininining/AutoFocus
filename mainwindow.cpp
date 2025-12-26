#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_driver(new AgeMotionDriver())
    , m_timer(new QTimer(this))
{
    ui->setupUi(this);

    // 设置定时器，每 200ms 更新一次状态
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateStatus);
}

MainWindow::~MainWindow()
{
    m_timer->stop();
    delete m_driver;
    delete ui;
}

void MainWindow::updateStatus()
{
    double pos = 0;
    double vel = 0;

    // 只有连接成功后才更新
    if (m_driver->getPosition(pos)) {
        ui->lblPosition->setText(QString("Position: %1 um").arg(pos, 0, 'f', 2));
    }

    if (m_driver->getVelocity(vel)) {
        ui->lblVelocity->setText(QString("Velocity: %1 RPM").arg(vel, 0, 'f', 2));
    }
}

void MainWindow::on_btnConnect_clicked()
{
    if (m_driver->connectDevice()) {
        QMessageBox::information(this, "Success", "Device connected successfully!");
        // 连接成功后启动定时器
        if (!m_timer->isActive()) {
            m_timer->start(200); 
        }
    } else {
        QMessageBox::critical(this, "Error", "Failed to connect: " + m_driver->getLastError());
    }
}

void MainWindow::on_btnSetVel_clicked()
{
    bool ok;
    double vel = QInputDialog::getDouble(this, "Set Velocity", "Enter velocity (RPM):", 60, 0, 3000, 1, &ok);
    if (ok) {
        if (m_driver->setTargetVelocity(vel)) {
            QMessageBox::information(this, "Success", QString("Velocity set to %1 RPM").arg(vel));
        } else {
            QMessageBox::warning(this, "Error", "Failed to set velocity: " + m_driver->getLastError());
        }
    }
}

void MainWindow::on_btnMove_clicked()
{
    bool ok;
    double pos = QInputDialog::getDouble(this, "Move to Position", "Enter position (um):", 0, -100000, 100000, 1, &ok);
    if (ok) {
        if (m_driver->moveToPosition(pos)) {
            qDebug() << "Moving to" << pos << "um";
        } else {
            QMessageBox::warning(this, "Error", "Failed to move: " + m_driver->getLastError());
        }
    }
}

void MainWindow::on_btnStop_clicked()
{
    if (m_driver->stopMotion()) {
        qDebug() << "Motion stopped.";
    } else {
        QMessageBox::warning(this, "Error", "Failed to stop: " + m_driver->getLastError());
    }
}

void MainWindow::on_btnGetPos_clicked()
{
    double pos = 0;
    if (m_driver->getPosition(pos)) {
        QMessageBox::information(this, "Position", QString("Current Position: %1 um").arg(pos));
    } else {
        QMessageBox::warning(this, "Error", "Failed to get position: " + m_driver->getLastError());
    }
}

void MainWindow::on_btnGetVel_clicked()
{
    double vel = 0;
    if (m_driver->getVelocity(vel)) {
        QMessageBox::information(this, "Velocity", QString("Current Velocity: %1 RPM").arg(vel));
    } else {
        QMessageBox::warning(this, "Error", "Failed to get velocity: " + m_driver->getLastError());
    }
}

void MainWindow::on_btnCheckError_clicked()
{
    int err = m_driver->checkError();
    if (err == 0) {
        QMessageBox::information(this, "Status", "No Error.");
    } else if (err > 0) {
        QMessageBox::warning(this, "Error", QString("Error Code: %1").arg(err));
    } else {
        QMessageBox::warning(this, "Error", "Communication failed.");
    }
}

void MainWindow::on_testbutton_clicked()
{

}

