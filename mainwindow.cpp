#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // Initialize UI state
    ui->groupBox_Motion->setEnabled(false);
    ui->btn_Disconnect->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    ui->textEdit_Log->append(QString("[%1] %2").arg(timestamp, message));
}

void MainWindow::on_btn_Connect_clicked()
{
    QString port = ui->lineEdit_Port->text();
    DWORD baud = ui->comboBox_Baud->currentText().toULong();
    
    log(QString("Connecting to %1 at %2...").arg(port).arg(baud));
    
    if (m_motion.connect(port.toStdString(), baud))
    {
        log("Connected successfully.");
        ui->lbl_Status->setText("Connected");
        ui->lbl_Status->setStyleSheet("color: green; font-weight: bold;");
        ui->groupBox_Motion->setEnabled(true);
        ui->btn_Connect->setEnabled(false);
        ui->btn_Disconnect->setEnabled(true);
        // ui->groupBox_Connection->setEnabled(false); // Optional: lock connection settings
    }
    else
    {
        log("Connection failed.");
        ui->lbl_Status->setText("Connection Failed");
        ui->lbl_Status->setStyleSheet("color: red;");
    }
}

void MainWindow::on_btn_Disconnect_clicked()
{
    m_motion.disconnect();
    log("Disconnected.");
    ui->lbl_Status->setText("Disconnected");
    ui->lbl_Status->setStyleSheet("");
    ui->groupBox_Motion->setEnabled(false);
    ui->btn_Connect->setEnabled(true);
    ui->btn_Disconnect->setEnabled(false);
    ui->groupBox_Connection->setEnabled(true);
}

void MainWindow::on_btn_SetPos_clicked()
{
    int32_t pos = ui->spinBox_Position->value();
    log(QString("Setting position to %1...").arg(pos));
    if (m_motion.setPosition(pos))
    {
        log("Set Position command sent.");
    }
    else
    {
        log("Failed to set position.");
    }
}

void MainWindow::on_btn_GetPos_clicked()
{
    int32_t pos = 0;
    if (m_motion.getPosition(pos))
    {
        log(QString("Current Position: %1").arg(pos));
        ui->lbl_CurrentPos->setText(QString::number(pos));
    }
    else
    {
        log("Failed to get position.");
    }
}

void MainWindow::on_btn_MoveRel_clicked()
{
    int32_t dist = ui->spinBox_Relative->value();
    log(QString("Moving relative by %1...").arg(dist));
    if (m_motion.moveRelative(dist))
    {
        log("Move Relative command sent.");
    }
    else
    {
        log("Failed to move relative.");
    }
}

void MainWindow::on_btn_SetSpeed_clicked()
{
    int32_t speed = ui->spinBox_Speed->value();
    log(QString("Setting speed to %1...").arg(speed));
    if (m_motion.setSpeed(speed))
    {
        log("Set Speed command sent.");
    }
    else
    {
        log("Failed to set speed.");
    }
}

void MainWindow::on_btn_GetSpeed_clicked()
{
    int32_t speed = 0;
    if (m_motion.getSpeed(speed))
    {
        log(QString("Current Speed: %1").arg(speed));
        ui->lbl_CurrentSpeed->setText(QString::number(speed));
    }
    else
    {
        log("Failed to get speed.");
    }
}

void MainWindow::on_btn_Stop_clicked()
{
    log("Stopping...");
    if (m_motion.stop())
    {
        log("Stop command sent.");
    }
    else
    {
        log("Failed to stop.");
    }
}

void MainWindow::on_btn_Home_clicked()
{
    log("Homing...");
    if (m_motion.home())
    {
        log("Home command sent.");
    }
    else
    {
        log("Failed to home.");
    }
}

void MainWindow::on_btn_CheckMoving_clicked()
{
    bool moving = m_motion.isMoving();
    QString status = moving ? "Moving" : "Stopped";
    log(QString("Motion Status: %1").arg(status));
    ui->lbl_MovingStatus->setText(status);
    if (moving)
        ui->lbl_MovingStatus->setStyleSheet("color: blue; font-weight: bold;");
    else
        ui->lbl_MovingStatus->setStyleSheet("color: black;");
}
