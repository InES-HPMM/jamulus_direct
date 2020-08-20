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
#include <QString>
#include "client.h"
#include "settings.h"
#include "serverdlg.h"


class CStartup
{
public:

    QList<QString>      CommandLineOptions;

    // client data
    quint16             iPortNumberClient;
    QString             strConnOnStartupAddress;
    QString             strMIDISetup;
    bool                bNoAutoJackConnect;
    QString             strNClientName;
    bool                bMuteMeInPersonalMix;
    QString             strUserName;
    QString             strSessionName;
    bool                bIsServer;
    bool                bP2P;

    // client settings
    QString             strIniFileName;

    // clientdlg data
    CClient*            pClient;
    CClientSettings*    pNSetClient;
    bool                bShowAnalyzerConsole;
    bool                bMuteStream;

    // server data
    int                 iNewMaxNumChan;
    QString             strLoggingFileName;
    quint16             iPortNumberServer;
    QString             strHTMLStatusFileName;
    QString             strCentralServer;
    QString             strServerInfo;
    QString             strNewWelcomeMessage;
    QString             strRecordingDirName;
    bool                bNDisconnectAllClientsOnQuit;
    bool                bNUseDoubleSystemFrameSize;
    ELicenceType        eNLicenceType;
    bool                bUseMultithreading;
    bool                bDisableRecording;
    QString             strServerPublicIP;
    QString             strServerListFilter;

    // central server
    bool                bNCentServPingServerInList;

    // serverdlg data
    CServer*            pServer;
    CServerSettings*    pNSetServer;
    bool                bStartMinimized;
};
