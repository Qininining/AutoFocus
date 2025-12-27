#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "AgeMotionDriver.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnConnect_clicked();
    void on_btnSetVel_clicked();
    void on_btnMove_clicked();
    void on_btnStop_clicked();
    void on_btnGetPos_clicked();
    void on_btnGetVel_clicked();
    void on_btnCheckError_clicked();
    void on_testbutton_clicked();

    // 新增测试槽函数
    void on_btnEnable_clicked(bool checked);
    void on_btnResetAlarm_clicked();
    void on_btnEmergencyStop_clicked();
    void on_btnMoveToLimit_clicked();
    void on_btnSetZeroOffset_clicked();
    void on_btnHoming_clicked();
    void on_btnPulsePos_clicked();

    void updateStatus(); // 定时更新状态槽函数

private:
    Ui::MainWindow *ui;
    AgeMotionDriver *m_driver;
    QTimer *m_timer;
};
#endif // MAINWINDOW_H
