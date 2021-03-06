/***********************myAvrTerminal*****************************************************
**
**  This program is based on the work of
**  Denis Shienkov <denis.shienkov@gmail.com>
**  Laszlo Papp <lpapp@kde.org>
**
**  lines of this code are part of the QtSerialPort module of the Qt Toolkit.
**
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "console.h"
#include "settingsdialog.h"

#include <QMessageBox>
#include <QLabel>
#include <QtSerialPort/QSerialPort>
#include <QTextCodec>
#include <QDebug>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{

    ui->setupUi(this);
    console = new Console;
    console->setEnabled(false);
    setCentralWidget(console);

    serial = new QSerialPort(this);

    settings = new SettingsDialog;

    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionQuit->setEnabled(true);
    ui->actionConfigure->setEnabled(true);

    status = new QLabel;
    ui->statusBar->addWidget(status);

    initActionsConnections();
    showStatusMessage(tr("Disconnected!"));


    connect(serial, &QSerialPort::errorOccurred,this,&MainWindow::handleError);

    connect(serial, &QSerialPort::readyRead, this, &MainWindow::readData);
    connect(serial, &QSerialPort::bytesWritten, this,&MainWindow::bytesSendToPort);

    connect(console, &Console::getData, this, &MainWindow::writeData);

    connect(settings,&SettingsDialog::changeBaudrate,this,&MainWindow::baudRateChanged);

}


MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
}


void MainWindow::openSerialPort()
{
    SettingsDialog::Settings p = settings->settings();
    serial->setPortName(p.name);
    serial->setBaudRate(p.baudRate);
    serial->setDataBits(p.dataBits);
    serial->setParity(p.parity);
    serial->setStopBits(p.stopBits);
    serial->setFlowControl(p.flowControl);
    if (serial->open(QIODevice::ReadWrite)) {
        console->setEnabled(true);
        console->setLocalEchoEnabled(p.localEchoEnabled);
        ui->actionConnect->setEnabled(false);
        ui->actionDisconnect->setEnabled(true);
        ui->actionConfigure->setEnabled(false);
        showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
                          .arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
                          .arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
    } else {
        QMessageBox::critical(this, tr("Error"), serial->errorString());

        showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort()
{
    if (serial->isOpen())
        serial->close();
    console->setEnabled(false);
    ui->actionConnect->setEnabled(true);
    ui->actionDisconnect->setEnabled(false);
    ui->actionConfigure->setEnabled(true);
    showStatusMessage(tr("Disconnected"));
}


void MainWindow::about()
{
    QMessageBox::about(this, tr("About myAvr Terminal"),
                       tr("The <b>myAvr Terminal</b> is aimed to connect "
                          "a mySmartUSB MK2 MK3 to Linux. "
                          "With this terminal you can use myMode commands"));
}

void MainWindow::writeData(const QByteArray &data)
{
    serial->write(data);
    serial->waitForBytesWritten(-1);
}

void MainWindow::bytesSendToPort(qint64 bytes)
{
    qDebug() << "Bytes send" << bytes;
}

void MainWindow::readData()
{
    QByteArray data = serial->readAll();
    console->putData(data);

   // QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
    // out.setCodec("Windows-1252");
    //QByteArray output = codec->toUnicode(data);
    qDebug() << "Data: " << data << " " << progmode;
    if (data.length() > 1){
        if (progmode)
        {
            qDebug() << "Data2: " << "  " << data << data.at(2);

            if (QString(data.at(2)) == "p")
                console->putData(QByteArray("Programmer mode is set"));
            else if (QString(data.at(2)) == "d")
                console->putData(QByteArray("Data-bypass mode is set"));
            else if (QString(data.at(2)) == "q")
                console->putData(QByteArray("Quite mode is set"));
        }
        progmode=false;
    }

}

void MainWindow::baudRateChanged(qint32 baudrate)
{
    serial->setBaudRate(baudrate);
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
    if (error != QSerialPort::NoError){
        QMessageBox::critical(this, tr("Critical Error"), serial->errorString());
        if (serial->isOpen()) {
            closeSerialPort();
        }
    }
}


void MainWindow::initActionsConnections()
{
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::openSerialPort);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::closeSerialPort);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::close);
    connect(ui->actionConfigure, &QAction::triggered, settings, &SettingsDialog::show);
    connect(ui->actionClear, &QAction::triggered, console, &Console::clear);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::about);
    connect(ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);

    connect(ui->actionProgMode,&QAction::triggered,this,&MainWindow::actionSetProgMode);
    connect(ui->actionDataMode,&QAction::triggered,this,&MainWindow::actionSetDataMode);
    connect(ui->actionQuiteMode,&QAction::triggered,this,&MainWindow::actionSetQuiteMode);
    connect(ui->actionResetBoard,&QAction::triggered,this,&MainWindow::actionResetBoard);
    connect(ui->actionReset_Programmer,&QAction::triggered,this,&MainWindow::actionReasetProgrammer);
    connect(ui->actionGetStatus,&QAction::triggered,this,&MainWindow::actionGetStatus);
}

void MainWindow::showStatusMessage(const QString &message)
{
    status->setText(message);
}

void MainWindow::actionSetProgMode(){
     progmode = true;
    setMode("p");
}
void MainWindow::actionSetDataMode(){
    progmode = true;
    setMode("d");
}
void MainWindow::actionSetQuiteMode(){
    progmode = true;
    setMode("q");
}
void MainWindow::actionResetBoard(){
    progmode = true;
    setMode("R");
}
void MainWindow::actionReasetProgrammer(){
    progmode = true;
    setMode("r");
}
void MainWindow::actionGetStatus(){
    progmode = true;
    setMode("i");
}

void MainWindow::setMode(const QString &mode)
{
    QString protocol;
    protocol ="æµº¹²³©";

    protocol.append(mode);

    protocol.append("\r\n");

    QTextCodec *codec = QTextCodec::codecForName("Windows-1252");
    QByteArray setmode = codec->fromUnicode(protocol);

    /**
      *ToDo: check errors
      **/
    serial->clear();

     console->clear();
    qDebug() << setmode;
    writeData(setmode);

}
