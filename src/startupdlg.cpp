/******************************************************************************\
 * Copyright (c) 2021
 *
 * Author(s):
 *  Simone Schwizer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/

#include "startupdlg.h"

CStartupDlg::CStartupDlg(CStartup& CStartup, QWidget* parent, Qt::WindowFlags f)
  : QDialog(parent, f)
  , qwidgetParent(parent)
  , Startup(CStartup)

{

    qInfo() << "Startup dialog constructor";
    setupUi(this);

    // Connections -------------------------------------------------------------
    // push buttons

    QObject::connect(butConnect,
                     &QPushButton::clicked,
                     this,
                     &CStartupDlg::OnConnectStartClient);

    QObject::connect(butOpen,
                     &QPushButton::clicked,
                     this,
                     &CStartupDlg::OnConnectStartSession);


    butConnect->setStyleSheet("QPushButton { background-color: rgb(0, 85, "
                                  "127); color: white; font:bold;}");
    butOpen->setStyleSheet("QPushButton { background-color: rgb(0, 85, "
                               "127); color: white; font:bold;}");
    edtName->setStyleSheet(
          "QLineEdit { background-color: rgb(0, 85, 127); color: white;}");
    edtSessionName->setStyleSheet(
          "QLineEdit { background-color: rgb(0, 85, 127); color: white;}");

    edtName->setText(Startup.strUserName);
    edtSessionName->setText(Startup.strSessionName);
    setWindowTitle ( "Jamulus UnMute" );

}

void
CStartupDlg::ShowDialog()
{
    edtName->text() = Startup.strUserName;
    edtSessionName->text() = Startup.strSessionName;

    show();
    exec();
}

void
CStartupDlg::OnConnectStartClient()
{
    Startup.strUserName = edtName->text();
    Startup.strSessionName = edtSessionName->text();
    Startup.bIsServer = false;

    hide();
}

void
CStartupDlg::OnConnectStartSession()
{
    Startup.strUserName = edtName->text();
    Startup.strSessionName = edtSessionName->text();
    Startup.bIsServer = true;

    hide();
}


void
CStartupDlg::closeEvent ( QCloseEvent* event )
{

    // default implementation of this event handler routine
    event->accept();

    //end application if startup dialog is closed
    exit(0);
}
