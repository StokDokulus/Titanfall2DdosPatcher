#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "patch.h"

#include <QDir>
#include <QMainWindow>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    QDir m_dir;
    bool m_dirSelected = false;
    std::shared_ptr<Patch> m_patch;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void resetUi();
    void updateUi(QString message = QString());
    void updateUiBackup();

    void setReady(bool ready);
    void setDir(QString dir);

private slots:
    void on_btSelectFolder_clicked();

    void on_btApplyPatch_clicked();

    void on_btRestoreBackup_clicked();

    void on_cbForce_stateChanged(int arg1);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
