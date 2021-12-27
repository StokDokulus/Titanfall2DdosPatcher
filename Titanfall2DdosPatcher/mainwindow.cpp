#include "mainwindow.h"
#include "patchloader.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    m_dirSelected = false;
    ui->setupUi(this);

    m_patch = PatchLoader::loadPatch_StokDokulus_Titanfall2_DeltaBuf_v1();
    updateUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resetUi()
{
    ui->edFolder->setText("");
    ui->txtStatus->setText("");
    ui->lbBackup->setText("");
    ui->btApplyPatch->setEnabled(false);
    ui->btRestoreBackup->setEnabled(false);

    ui->cbForce->setCheckState(Qt::Unchecked);
    ui->cbForce->setVisible(false);
}

void MainWindow::updateUi(QString messageExtra)
{
    resetUi();

    if(!messageExtra.isEmpty())
    {
        // add separator
        messageExtra = messageExtra + "<br/><span>------------------------------</span><br/>";
    }

    if(m_patch == nullptr)
    {
        ui->txtStatus->setHtml(messageExtra +
                               "<span style='color: #F00; font-weight:bold'>Error: No patch available.</span>");
        return;
    }

    if(!m_dirSelected || !m_dir.exists())
    {
        ui->txtStatus->setHtml(messageExtra +
                               "Please select the Titanfall2 installation directory.<br/>"
                               "The installation directory is the directory that contains Titanfall2.exe");
        return;
    }

    ui->edFolder->setText(m_dir.absolutePath());

    if(!m_dir.exists("Titanfall2.exe"))
    {
        ui->txtStatus->setHtml(messageExtra +
                    "<span style='color: #F00'>Error: Titanfall not found in selected directory.</span><br/>"
                    "Please select the Titanfall2 installation directory.<br/>"
                    "The installation directory is the directory that contains Titanfall2.exe");
        return;
    }


    // "Restore backup" feature
    updateUiBackup();


    // "Apply patch" feature
    //
    Patch::Error errorDryRun = m_patch->applyPatch(m_dir, Patch::PATCH_DRY_RUN | Patch::PATCH_VERIFY_CHECKSUM);
    QString strStatus;
    switch(errorDryRun)
    {
    case Patch::ERROR_SUCCESS:
        ui->btApplyPatch->setEnabled(true);
        strStatus = "<span style='color: #080; font-weight:bold'>Unmodified game file detected.</span><br/>"
                    "<span>The patch seems to be compatible with your game and can be installed.</span><br/>"
                    "<span>However, there is no guarantee that the patch will work as intended.</span><br/>";
        break;
    case Patch::ERROR_SUCCESS_MODIFIED:
        ui->btApplyPatch->setEnabled(true);
        strStatus = "<span style='color: #F00; font-weight:bold'>Warning: Modified game file detected.</span><br/>"
                    "<span>Either your game file was modified or your version of the game does not match the version this patch was designed for.</span><br/>"
                    "<span>The patch can be applied to your game but may not be compatible.</span><br/>"
                    "<span style='color: #F00; font-weight:bold'>There might be unintended consequences. Proceed with caution and at your own risk.</span><br/>";
        break;
    case Patch::ERROR_PATCH_ALREADY_INSTALLED:
        ui->btApplyPatch->setEnabled(false);
        strStatus = "<span style='color: #00F; font-weight:bold'>The patch is already installed.</span><br/>";
        break;

    case Patch::ERROR_PATCH_INCOMPATIBLE:
        ui->btApplyPatch->setEnabled(true);
        ui->cbForce->setCheckState(Qt::Unchecked);
        ui->cbForce->setVisible(true);
        strStatus = "<span style='color: #F00; font-weight:bold'>Warning: Incompatible game file detected.</span><br/>"
                    "<span style='color: #F00'>The patch is not compatible with your game.<br/>"
                    "<span style='color: #F00'>Either your game file was modified or your version of the game does not match the version this patch was designed for.</span><br/>"
                    "<span style='color: #F00'>The code or data that would be edited by this patch does not look as expected.</span><br/>"
                    "<br/>"
                    "<span style='color: #F00'>You may force the installation of the patch but this is not recommended.</span><br/>"
                    "<span style='color: #F00'>The patch is unlikely to work and may change the game's behavior in unforseen ways.</span><br/>"
                    "<span style='color: #F00; font-weight:bold'>Do not force the installation unless you know what you are doing!</span><br/>";
        break;
    default:
        ui->btApplyPatch->setEnabled(false);
        strStatus = "<span style='color: #F00; font-weight:bold'>Failed to scan your game file:</span><br/>"
                    "<span style='color: #F00'>" + m_patch->errorString(errorDryRun) + "</span><br/>";
        break;
    }

    QString strFile = "<span>File: " + m_patch->getPathRelative() + "</span><br/>";
    strStatus = messageExtra + strFile + strStatus;
    ui->txtStatus->setHtml(strStatus);
}

void MainWindow::updateUiBackup()
{
    bool backupExists = m_patch->backupExists(m_dir);
    if(backupExists)
    {
        ui->lbBackup->setText("");
        ui->btRestoreBackup->setEnabled(true);
    }
    else
    {
        ui->lbBackup->setText("No backup available.");
    }
}



void MainWindow::setReady(bool ready)
{
    ui->btApplyPatch->setEnabled(ready);
    ui->btRestoreBackup->setEnabled(ready);
}

void MainWindow::setDir(QString strDir)
{
    resetUi();
    m_dir = strDir;
    if(strDir.isEmpty() || !m_dir.exists())
    {
        ui->edFolder->setText("");
        m_dirSelected = false;
    }
    else
    {
        ui->edFolder->setText(m_dir.absolutePath());
        m_dirSelected = true;
    }
    updateUi();
}


void MainWindow::on_btSelectFolder_clicked()
{
    QString strDir = QFileDialog::getExistingDirectory(this, "Select Titanfall2 installation directory");
    if(!strDir.isEmpty())
        setDir(strDir);
}

void MainWindow::on_btApplyPatch_clicked()
{
    if(m_patch == nullptr)
    {
        updateUi();
        return;
    }

    if(!m_patch->backupExists(m_dir))
    {
        QMessageBox::StandardButton result = QMessageBox::question(
                    this,
                    "Create backup of game files?",
                    "Do you want to create a backup of your game files before installing the patch?\n"
                    "The backups will be stored in the game installation folder next to the patched files.",
                    QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                    QMessageBox::Yes);
        if(result == QMessageBox::Yes)
        {
            bool ok = m_patch->backupCreate(m_dir);
            if(!ok)
            {
                updateUi("<span style='color: #F00; font-weight:bold'>Error: Failed to create backup.</span><br/>"
                         "<span>Patch installation aborted.</span><br/>"
                         "<span>Your game files were not modified.</span><br/>");
                return;
            }
            updateUiBackup();
        }
        else if(result == QMessageBox::Cancel)
        {
            updateUi("<span>Patch installation cancelled.</span><br/>"
                     "<span>Your game files were not modified.</span><br/>");
            return;
        }
    }

    Patch::PatchFlags flags = 0;
    if(ui->cbForce->isChecked())
    {
        flags = flags | Patch::PATCH_FORCE;
    }

    Patch::Error error = m_patch->applyPatch(m_dir, flags);

    if(error == Patch::ERROR_SUCCESS || error == Patch::ERROR_SUCCESS_MODIFIED)
    {
        updateUi("<span style='color: #080; font-weight:bold'>Patch installed successfully.</span><br/>"
                 "<span>You may now close the patcher.</span><br/>");
    }
    else
    {
        updateUi("<span style='color: #F00; font-weight:bold'>Patch installation failed.</span><br/>"
                 "<span style='color: #F00'>" + m_patch->errorString(error) + "</span><br/>");
    }
}

void MainWindow::on_btRestoreBackup_clicked()
{
    if(m_patch == nullptr)
    {
        updateUi();
        return;
    }

    bool ok = m_patch->backupRestore(m_dir);
    if(ok)
    {
        updateUi("<span style='font-weight:bold'>Backup restored successfully.</span>");
    }
    else
    {
        updateUi("<span style='color: #F00; font-weight:bold'>Failed to restore backup.</span>");
    }
}

void MainWindow::on_cbForce_stateChanged(int state)
{
    if(state != Qt::Unchecked)
    {
        QMessageBox::StandardButton result = QMessageBox::warning(
                    this,
                    "Force installation of patch?",
                    "WARNING: Forcing the installation of this patch is likely to break things or have unforseen consequences.\n"
                    "Do you want to proceed?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);
        if(result != QMessageBox::Yes)
        {
            ui->cbForce->setCheckState(Qt::Unchecked);
        }
    }
}
