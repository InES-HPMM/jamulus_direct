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

#pragma once

#include <QLabel>
#include <QString>
#include <QLineEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QWhatsThis>
#include <QTimer>
#include <QSlider>
#include <QRadioButton>
#include <QMenuBar>
#include <QLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
# include <QVersionNumber>
#endif
#include "global.h"
#include "settings.h"
#include "multicolorled.h"
#include "audiomixerboard.h"
#include "chatdlg.h"
#include "connectdlg.h"
#include "analyzerconsole.h"
#include "ui_startupdlgbase.h"
#include "clientdlg.h"
#include "startup.h"
#include <QCloseEvent>

/* Classes ********************************************************************/

class CStartupDlg : public QDialog, private Ui_CStartupDlgBase
{
    Q_OBJECT

public:
    CStartupDlg ( CStartup&        CStartup,
                  QWidget*         parent = nullptr,
                  Qt::WindowFlags  f = nullptr );


    void ShowDialog();
    void StartClient();
    void StartSession();


protected:
    QWidget*        qwidgetParent;
    CStartup&       Startup;
    virtual void    closeEvent     ( QCloseEvent*     Event );


public slots:
    void OnConnectStartClient();
    void OnConnectStartSession();
};
