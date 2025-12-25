#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "MicroScopeMotion.h"

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
    void on_btn_Connect_clicked();
    void on_btn_Disconnect_clicked();
    void on_btn_SetPos_clicked();
    void on_btn_GetPos_clicked();
    void on_btn_MoveRel_clicked();
    void on_btn_SetSpeed_clicked();
    void on_btn_GetSpeed_clicked();
    void on_btn_Stop_clicked();
    void on_btn_Home_clicked();
    void on_btn_CheckMoving_clicked();

private:
    void log(const QString& message);

    Ui::MainWindow *ui;
    MicroScopeMotion m_motion;
};
#endif // MAINWINDOW_H
