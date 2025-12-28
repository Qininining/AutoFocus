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
        if (m_driver->setTargetRPM(vel)) {
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
        if (m_driver->setTargetPosition(pos)) {
            qDebug() << "Moving to" << pos << "um";
        } else {
            QMessageBox::warning(this, "Error", "Failed to move: " + m_driver->getLastError());
        }
    }
}

void MainWindow::on_btnMoveRel_clicked()
{
    bool ok;
    double delta = QInputDialog::getDouble(this, "Move Relative", "Enter distance (um):", 0, -100000, 100000, 1, &ok);
    if (ok) {
        if (m_driver->setRelativePosition(delta)) {
            qDebug() << "Moving relative by" << delta << "um";
        } else {
            QMessageBox::warning(this, "Error", "Failed to move relative: " + m_driver->getLastError());
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
    if (m_driver->getTargetRPM(vel)) {
        QMessageBox::information(this, "Velocity", QString("Target Velocity: %1 RPM").arg(vel));
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

// ==========================================
//          新增测试功能实现
// ==========================================

void MainWindow::on_btnEnable_clicked(bool checked)
{
    if (m_driver->setEnable(checked)) {
        ui->btnEnable->setText(checked ? "Enabled (Click to Disable)" : "Disabled (Click to Enable)");
    } else {
        QMessageBox::warning(this, "Error", "Failed to set enable state.");
        // 恢复按钮状态
        ui->btnEnable->setChecked(!checked);
    }
}

void MainWindow::on_btnEmergencyStop_clicked()
{
    if (m_driver->emergencyStop()) {
        QMessageBox::critical(this, "STOP", "Emergency Stop command sent!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to send Emergency Stop.");
    }
}

void MainWindow::on_btnMoveToLimit_clicked()
{
    QStringList items;
    items << "Upper Limit" << "Lower Limit";
    bool ok;
    QString item = QInputDialog::getItem(this, "Move to Limit", "Select Direction:", items, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        bool toUpper = (item == "Upper Limit");
        if (m_driver->moveToLimit(toUpper)) {
            qDebug() << "Moving to" << item;
        } else {
            QMessageBox::warning(this, "Error", "Failed to move to limit sensor.");
        }
    }
}

void MainWindow::on_btnSetZeroOffset_clicked()
{
    if (QMessageBox::question(this, "Confirm", "Set current position offset to zero?", QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {
        if (m_driver->setCurrPositionToZero()) {
            QMessageBox::information(this, "Success", "Position offset set to zero.");
        } else {
            QMessageBox::warning(this, "Error", "Failed to set position offset.");
        }
    }
}

void MainWindow::on_btnHoming_clicked()
{
    QStringList items;
    items << "To High (Positive)" << "To Low (Negative)";
    bool ok;
    QString item = QInputDialog::getItem(this, "Homing", "Select Homing Direction:", items, 0, false, &ok);
    if (ok && !item.isEmpty()) {
        bool toHigh = (item == "To High (Positive)");
        if (m_driver->findReference(toHigh)) {
            qDebug() << "Homing " << item;
        } else {
            QMessageBox::warning(this, "Error", "Failed to start homing.");
        }
    }
}

void MainWindow::on_btnPulsePos_clicked()
{
    // 1. 读取当前脉冲位置
    int currentPulses = 0;
    if (m_driver->getPulsePosition(currentPulses)) {
        // 2. 询问新位置
        bool ok;
        int newPulses = QInputDialog::getInt(this, "Pulse Position", 
            QString("Current Pulses: %1\nEnter target pulses:").arg(currentPulses), 
            currentPulses, -2147483647, 2147483647, 1, &ok);
        
        if (ok) {
            if (m_driver->setTargetPulsePosition(newPulses)) {
                qDebug() << "Target pulses set to" << newPulses;
            } else {
                QMessageBox::warning(this, "Error", "Failed to set target pulse position.");
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Failed to read pulse position.");
    }
}

void MainWindow::updateStatus()
{
    // 1. 基础运动信息
    double pos = 0;
    if (m_driver->getPosition(pos)) {
        ui->lblPosition->setText(QString("Position: %1 um").arg(pos, 0, 'f', 2));
    }

    double vel = 0;
    if (m_driver->getTargetRPM(vel)) {
        ui->lblVelocity->setText(QString("Target Velocity: %1 RPM").arg(vel, 0, 'f', 2));
    }

    double realVel = 0;
    if (m_driver->getVelocity(realVel)) {
        ui->lblRealVelocity->setText(QString("Real Velocity: %1 um/s").arg(realVel, 0, 'f', 2));
    }

    // 2. 扩展信息 (脉冲、电流、温度)
    int pulses = 0;
    if (m_driver->getPulsePosition(pulses)) {
        ui->lblPulse->setText(QString("Pulse: %1").arg(pulses));
    }

    double current = 0;
    if (m_driver->getRealTimeCurrent(current)) {
        ui->lblCurrent->setText(QString("Current: %1 A").arg(current, 0, 'f', 2));
    }

    int temp = 0;
    if (m_driver->getCpuTemperature(temp)) {
        ui->lblTemp->setText(QString("Temp: %1 C").arg(temp));
    }
    
    // 3. 状态标志位更新
    QString statusStr;
    bool upper = false, lower = false;
    if (m_driver->isLimitSensorTriggered(upper, lower)) {
        if (upper) statusStr += "[UPPER] ";
        if (lower) statusStr += "[LOWER] ";
    }

    bool isMotionDone = false;
    if (m_driver->isMotionComplete(isMotionDone)) {
        if (isMotionDone) statusStr += "[IDLE] ";
        else statusStr += "[MOVING] ";
    }

    int err = m_driver->checkError();
    if (err > 0) {
        statusStr += QString("[ERR: %1]").arg(err);
        ui->lblStatusInfo->setStyleSheet("color: red; font-weight: bold;");
    } else {
        ui->lblStatusInfo->setStyleSheet("color: blue;");
    }

    if (statusStr.isEmpty()) statusStr = "Status: OK";
    ui->lblStatusInfo->setText(statusStr);
    
    // 状态栏同步显示
    if (err > 0 || upper || lower) {
         ui->statusbar->showMessage(statusStr);
    } else {
         ui->statusbar->clearMessage();
    }
}

void MainWindow::on_btnTestResolution_clicked()
{
    unsigned int currentRes = 0;
    if (m_driver->getSingleToothResolution(currentRes)) {
        QMessageBox::information(this, "Single Tooth Resolution", 
            QString("Current Resolution: %1").arg(currentRes));
    } else {
        QMessageBox::warning(this, "Error", "Failed to read resolution.");
    }
}

void MainWindow::on_btnTestPulseStep_clicked()
{
    double currentStepUm = 0.0;
    if (m_driver->getMinStepUm(currentStepUm)) {
        bool ok;
        double newStepUm = QInputDialog::getDouble(this, "Min Step Length (um)",
            QString("Current Step Length: %1 um\nEnter new step length (um):").arg(currentStepUm),
            currentStepUm, 0.001, 1000.0, 3, &ok);

        if (ok) {
            if (m_driver->setMinStepUm(newStepUm)) {
                QMessageBox::information(this, "Success", QString("Min Step Length set to %1 um").arg(newStepUm));
            } else {
                QMessageBox::warning(this, "Error", "Failed to set min step length.");
            }
        }
    } else {
        QMessageBox::warning(this, "Error", "Failed to read min step length.");
    }
}

void MainWindow::on_btnGetTargetVelUm_clicked()
{
    double vel = 0;
    if (m_driver->getTargetVelocity(vel)) {
        QMessageBox::information(this, "Target Velocity", QString("Target Velocity: %1 um/s").arg(vel));
    } else {
        QMessageBox::warning(this, "Error", "Failed to get target velocity: " + m_driver->getLastError());
    }
}

void MainWindow::on_btnSetTargetVelUm_clicked()
{
    bool ok;
    double vel = QInputDialog::getDouble(this, "Set Target Velocity", "Enter velocity (um/s):", 1000, 0, 100000, 1, &ok);
    if (ok) {
        if (m_driver->setTargetVelocity(vel)) {
            QMessageBox::information(this, "Success", QString("Target Velocity set to %1 um/s").arg(vel));
        } else {
            QMessageBox::warning(this, "Error", "Failed to set target velocity: " + m_driver->getLastError());
        }
    }
}

void MainWindow::on_btnSetJogVel_clicked()
{
    bool ok;
    double vel  = QInputDialog::getDouble(this, "Set Jog Velocity", "Enter velocity (um/s):", 0, -100000, 100000, 1, &ok);
    if (ok) {
        if (m_driver->setVelocity(vel)) {
            if (qAbs(vel) < 0.001)
                qDebug() << "Jog Motion Stopped.";
            else
                qDebug() << "Jogging at" << vel << "um/s";
        } else {
            QMessageBox::warning(this, "Error", "Failed to set jog velocity: " + m_driver->getLastError());
        }
    }
}

void MainWindow::on_btnGetRealVel_clicked()
{
    double vel = 0;
    if (m_driver->getVelocity(vel)) {
        QMessageBox::information(this, "Real Velocity", QString("Real Velocity: %1 um/s").arg(vel));
    } else {
        QMessageBox::warning(this, "Error", "Failed to get real velocity: " + m_driver->getLastError());
    }
}


