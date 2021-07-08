/******************************************************************************\
 * Copyright (c) 2004-2020
 *
 * Author(s):
 *  Volker Fischer
 *
 * THIS FILE WAS MODIFIED by
 *  Institut of Embedded Systems ZHAW (www.zhaw.ch/ines) - Simone Schwizer
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

#include <QCoreApplication>
#include <QDir>
#include "global.h"
#include "startup.h"
#ifndef HEADLESS
# include <QApplication>
# include <QMessageBox>
# include "clientdlg.h"
# include "serverdlg.h"
# include "startupdlg.h"
#endif
#include "settings.h"
#include "testbench.h"
#include "util.h"
#ifdef ANDROID
# include <QtAndroidExtras/QtAndroid>
#endif
#if defined ( Q_OS_MACX )
# include "mac/activity.h"
#endif


// Implementation **************************************************************
QString GetClientName(const QString& sNFiName);

int main ( int argc, char** argv )
{

    QString        strArgument;
    double         rDbleArgument;

    QCoreApplication* pApp;
    CServer* pServer;
    CClient* pClient;
    CClientDlg* pClientDlg;
    CStartup Startup;

    #if defined(SERVER_BUNDLE) && (defined(__APPLE__) || defined(__MACOSX))
    // if we are on MacOS and we are building a server bundle, starts Jamulus in
    // server mode
    Startup.bIsServer = true;
#else
    Startup.bIsServer = false;
#endif

    // initialize all flags and string which might be changed by command line
    // arguments
    bool         bUseGUI                        = true;
    bool         bCustomPortNumberGiven         = false;
    quint16      iPortNumber                    = DEFAULT_PORT_NUMBER;

    Startup.strMIDISetup                        = "";
    Startup.bNoAutoJackConnect                  = false;
    Startup.strUserName                         = "";
    Startup.strSessionName                      = "";
    Startup.strNClientName                      = APP_NAME;
    Startup.iNewMaxNumChan                      = DEFAULT_USED_NUM_CHANNELS;
    Startup.strLoggingFileName                  = "";
    Startup.strHTMLStatusFileName               = "";
    Startup.strCentralServer                    = CENTSERV_ANY_GENRE2;
    Startup.strServerInfo                       = "";
    Startup.strNewWelcomeMessage                = "";
    Startup.strRecordingDirName                 = "";
    Startup.bNDisconnectAllClientsOnQuit        = false;
    Startup.bNUseDoubleSystemFrameSize          = true; // default is 128 samples frame size
    Startup.eNLicenceType                       = LT_NO_LICENCE;
    Startup.bShowAnalyzerConsole                = false;
    Startup.bMuteStream                         = false;
    Startup.bStartMinimized                     = false;
    Startup.strIniFileName                      = "";
    Startup.iPortNumberServer                   = 22124;
    Startup.iPortNumberClient                   = 22124+10;
    Startup.bUseMultithreading                  = false;
    Startup.bDisableRecording                   = false;
    Startup.strServerPublicIP                   = "";
    Startup.strServerListFilter                 = "";
    Startup.bMuteMeInPersonalMix                = false;
    Startup.bNCentServPingServerInList          = false;

    // QT docu: argv()[0] is the program name, argv()[1] is the first
    // argument and argv()[argc()-1] is the last argument.
    // Start with first argument, therefore "i = 1"
    for ( int i = 1; i < argc; i++ )
    {
        if (GetFlagArgument(argv, i, "-s", "--server")) {
            Startup.bIsServer = true;
            qInfo() << "- server mode chosen";
            continue;
        }


        // Use GUI flag --------------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-n",
                               "--nogui" ) )
        {
            bUseGUI = false;
            qInfo() << "- no GUI mode chosen";
            Startup.CommandLineOptions << "--nogui";
            continue;
        }


        // Use licence flag ----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-L",
                               "--licence" ) )
        {
            // right now only the creative commons licence is supported
            Startup.eNLicenceType = LT_CREATIVECOMMONS;
            qInfo() << "- licence required";
            Startup.CommandLineOptions << "--licence";
            continue;
        }


        // Use 64 samples frame size mode --------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-F",
                               "--fastupdate" ) )
        {
            Startup.bNUseDoubleSystemFrameSize = false; // 64 samples frame size
            qInfo() << qUtf8Printable( QString( "- using %1 samples frame size mode" )
                .arg( SYSTEM_FRAME_SIZE_SAMPLES ) );
            Startup.CommandLineOptions << "--fastupdate";
            continue;
        }

        // Use multithreading --------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-T",
                               "--multithreading" ) )
        {
            Startup.bUseMultithreading = true;
            qInfo() << "- using multithreading";
            Startup.CommandLineOptions << "--multithreading";
            continue;
        }


        // Maximum number of channels ------------------------------------------
        if ( GetNumericArgument ( argc,
                                  argv,
                                  i,
                                  "-u",
                                  "--numchannels",
                                  1,
                                  MAX_NUM_CHANNELS,
                                  rDbleArgument ) )
        {
            Startup.iNewMaxNumChan = static_cast<int> ( rDbleArgument );

            qInfo() << qUtf8Printable( QString("- maximum number of channels: %1")
                .arg( Startup.iNewMaxNumChan ) );

            Startup.CommandLineOptions << "--numchannels";
            continue;
        }


        // Start minimized -----------------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-z",
                               "--startminimized" ) )
        {
            Startup.bStartMinimized = true;
            qInfo() << "- start minimized enabled";
            Startup.CommandLineOptions << "--startminimized";
            continue;
        }

        // Disconnect all clients on quit --------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-d",
                               "--discononquit" ) )
        {
            Startup.bNDisconnectAllClientsOnQuit = true;
            qInfo() << "- disconnect all clients on quit";
            Startup.CommandLineOptions << "--discononquit";
            continue;
        }


        // Disabling auto Jack connections -------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-j",
                               "--nojackconnect" ) )
        {
            Startup.bNoAutoJackConnect = true;
            qInfo() << "- disable auto Jack connections";
            Startup.CommandLineOptions << "--nojackconnect";
            continue;
        }


        // Show analyzer console -----------------------------------------------
        // Undocumented debugging command line argument: Show the analyzer
        // console to debug network buffer properties.
        if ( GetFlagArgument ( argv,
                               i,
                               "--showanalyzerconsole", // no short form
                               "--showanalyzerconsole" ) )
        {
            Startup.bShowAnalyzerConsole = true;
            qInfo() << "- show analyzer console";
            Startup.CommandLineOptions << "--showanalyzerconsole";
            continue;
        }


        // Controller MIDI channel ---------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--ctrlmidich", // no short form
                                 "--ctrlmidich",
                                 strArgument ) )
        {
            Startup.strMIDISetup = strArgument;
            qInfo() << qUtf8Printable( QString( "- MIDI controller settings: %1" )
                .arg( Startup.strMIDISetup ) );
            Startup.CommandLineOptions << "--ctrlmidich";
            continue;
        }


        // Use logging ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-l",
                                 "--log",
                                 strArgument ) )
        {
            Startup.strLoggingFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- logging file name: %1" )
                .arg( Startup.strLoggingFileName ) );
            Startup.CommandLineOptions << "--log";
            continue;
        }


        // Port number ---------------------------------------------------------
        if ( GetNumericArgument ( argc,
                                  argv,
                                  i,
                                  "-p",
                                  "--port",
                                  0,
                                  65535,
                                  rDbleArgument ) )
        {
            iPortNumber            = static_cast<quint16> ( rDbleArgument );
            bCustomPortNumberGiven = true;
            Startup.iPortNumberServer = iPortNumber;
            Startup.iPortNumberClient = iPortNumber + 10;
            qInfo() << qUtf8Printable( QString( "- portnumber server: %1" )
                .arg( Startup.iPortNumberServer ) );
            qInfo() << qUtf8Printable( QString( "- portnumber client: %1" )
                .arg( Startup.iPortNumberClient ) );
            Startup.CommandLineOptions << "--port";
            continue;
        }


        // HTML status file ----------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-m",
                                 "--htmlstatus",
                                 strArgument ) )
        {
            Startup.strHTMLStatusFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- HTML status file name: %1" )
                .arg( Startup.strHTMLStatusFileName ) );
            Startup.CommandLineOptions << "--htmlstatus";
            continue;
        }

        // User Name ---------------------------------------------------------
        if (GetStringArgument(argc, argv, i, "-U", "--username", strArgument)) {
            Startup.strUserName = strArgument; // QString ( APP_NAME ) + " " +
            qInfo() << "- user name: " << Startup.strUserName;
            continue;
        }

        // Session Name to use for p2p connection ----------------------------
        if (GetStringArgument(
              argc, argv, i, "-S", "--sessionname", strArgument)) {
            Startup.strSessionName = strArgument;
            qInfo() << "- session name for p2p connection: "
                    << Startup.strSessionName;
            continue;
        }


        // Client Name ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "--clientname", // no short form
                                 "--clientname",
                                 strArgument ) )
        {
            Startup.strNClientName = QString ( APP_NAME ) + " " + strArgument;
            qInfo() << qUtf8Printable( QString( "- client name: %1" )
                .arg( Startup.strNClientName ) );
            continue;
        }


        // Recording directory -------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-R",
                                 "--recording",
                                 strArgument ) )
        {
	        Startup.strRecordingDirName = strArgument;
            qInfo() << qUtf8Printable( QString("- recording directory name: %1" )
                .arg( Startup.strRecordingDirName ) );
            Startup.CommandLineOptions << "--recording";
        }


        // Disable recording on startup ----------------------------------------
       if ( GetFlagArgument ( argv,
                             i,
                              "--norecord", // no short form
                              "--norecord" ) )
       {
           Startup.bDisableRecording = true;
           qInfo() << "- recording will not be enabled";
           Startup.CommandLineOptions << "--norecord";
           continue;
       }


        // Central server ------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-e",
                                 "--centralserver",
                                 strArgument ) )
        {
            Startup.strCentralServer = strArgument;
            qInfo() << qUtf8Printable( QString( "- central server: %1" )
                .arg( Startup.strCentralServer ) );
            continue;
        }


        // Server Public IP --------------------------------------------------
       if ( GetStringArgument ( argc,
                                argv,
                                i,
                                "--serverpublicip", // no short form
                                "--serverpublicip",
                                strArgument ) )
       {
           Startup.strServerPublicIP = strArgument;
           qInfo() << qUtf8Printable( QString( "- server public IP: %1" )
               .arg( Startup.strServerPublicIP ) );
           Startup.CommandLineOptions << "--serverpublicip";
           continue;
       }

        // Server info ---------------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-o",
                                 "--serverinfo",
                                 strArgument ) )
        {
            Startup.strServerInfo = strArgument;
            qInfo() << qUtf8Printable( QString( "- server info: %1" )
                .arg( Startup.strServerInfo ) );
            Startup.CommandLineOptions << "--serverinfo";
            continue;
        }


        // Server list filter --------------------------------------------------
       if ( GetStringArgument ( argc,
                                argv,
                                i,
                                "-f",
                                "--listfilter",
                                strArgument ) )
       {
           Startup.strServerListFilter = strArgument;
           qInfo() << qUtf8Printable( QString( "- server list filter: %1" )
               .arg( Startup.strServerListFilter ) );
          Startup.CommandLineOptions << "--listfilter";
           continue;
       }


        // Server welcome message ----------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-w",
                                 "--welcomemessage",
                                 strArgument ) )
        {
            Startup.strNewWelcomeMessage = strArgument;
            qInfo() << qUtf8Printable( QString( "- welcome message: %1" )
                .arg( Startup.strNewWelcomeMessage ) );
            Startup.CommandLineOptions << "--welcomemessage";
            continue;
        }


        // Initialization file -------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-i",
                                 "--inifile",
                                 strArgument ) )
        {
            Startup.strIniFileName = strArgument;
            qInfo() << qUtf8Printable( QString( "- initialization file name: %1" )
                .arg( Startup.strIniFileName ) );
            Startup.CommandLineOptions << "--inifile";
            continue;
        }


        // Connect on startup --------------------------------------------------
        if ( GetStringArgument ( argc,
                                 argv,
                                 i,
                                 "-c",
                                 "--connect",
                                 strArgument ) )
        {
            Startup.strConnOnStartupAddress = NetworkUtil::FixAddress ( strArgument );
            qInfo() << qUtf8Printable( QString( "- connect on startup to address: %1" )
                .arg( Startup.strConnOnStartupAddress ) );
            Startup.CommandLineOptions << "--connect";
            continue;
        }


        // Mute stream on startup ----------------------------------------------
        if ( GetFlagArgument ( argv,
                               i,
                               "-M",
                               "--mutestream" ) )
        {
            Startup.bMuteStream = true;
            qInfo() << "- mute stream activated";
            Startup.CommandLineOptions << "--mutestream";
            continue;
        }


        // For headless client mute my own signal in personal mix --------------
        if ( GetFlagArgument ( argv,
                               i,
                               "--mutemyown", // no short form
                               "--mutemyown" ) )
        {
            Startup.bMuteMeInPersonalMix = true;
            qInfo() << "- mute me in my personal mix";
            Startup.CommandLineOptions << "--mutemyown";
        }


        // Version number ------------------------------------------------------
        if ( ( !strcmp ( argv[i], "--version" ) ) ||
             ( !strcmp ( argv[i], "-v" ) ) )
        {
            qCritical() << qUtf8Printable( GetVersionAndNameStr ( false ) );
            exit ( 1 );
        }


        // Help (usage) flag ---------------------------------------------------
        if ( ( !strcmp ( argv[i], "--help" ) ) ||
             ( !strcmp ( argv[i], "-h" ) ) ||
             ( !strcmp ( argv[i], "-?" ) ) )
        {
            const QString strHelp = UsageArguments ( argv );
            qInfo() << qUtf8Printable( strHelp );
            exit ( 0 );
        }

        // Ping servers in list for central server -----------------------------
        if (GetFlagArgument(argv, i, "-g", "--pingservers")) {
            Startup.bNCentServPingServerInList = true;
            qInfo() << "- ping servers in slave server list";
            continue;
        }


        // Unknown option ------------------------------------------------------
        qCritical() << qUtf8Printable( QString( "%1: Unknown option '%2' -- use '--help' for help" )
            .arg( argv[0] ).arg( argv[i] ) );

    }


    // Dependencies ------------------------------------------------------------
#ifdef HEADLESS
    if ( bUseGUI )
    {
        bUseGUI = false;
        qWarning() << "No GUI support compiled. Running in headless mode.";
    }
    //Q_UNUSED ( bStartMinimized )       // avoid compiler warnings
    //Q_UNUSED ( bShowComplRegConnList ) // avoid compiler warnings
    //Q_UNUSED ( bShowAnalyzerConsole )  // avoid compiler warnings
    //Q_UNUSED ( bMuteStream )           // avoid compiler warnings
#endif

    // the inifile is not supported for the headless server mode
    if ( Startup.bIsServer && !bUseGUI && !Startup.strIniFileName.isEmpty() )
    {
        qWarning() << "No initialization file support in headless server mode.";
    }

    // mute my own signal in personal mix is only supported for headless mode
    if ( !Startup.bIsServer && bUseGUI && Startup.bMuteMeInPersonalMix )
    {
        Startup.bMuteMeInPersonalMix = false;
        qWarning() << "Mute my own signal in my personal mix is only supported in headless mode.";
    }

    if ( !Startup.strServerPublicIP.isEmpty() )
    {
        QHostAddress InetAddr;
        if ( !InetAddr.setAddress ( Startup.strServerPublicIP ) )
        {
            qWarning() << "Server Public IP is invalid. Only plain IP addresses are supported.";
        }
        if ( Startup.strCentralServer.isEmpty() || !Startup.bIsServer )
        {
            qWarning() << "Server Public IP will only take effect when registering a server with a central server.";
        }
    }

    // per definition: if we are in "GUI" server mode and no central server
    // address is given, we use the default central server address
    if ( Startup.bIsServer && bUseGUI && Startup.strCentralServer.isEmpty() )
    {
        Startup.strCentralServer = DEFAULT_SERVER_ADDRESS;
    }

    // Application/GUI setup ---------------------------------------------------
    // Application object
#ifdef HEADLESS
    pApp = new QCoreApplication ( argc, argv );
#else
    pApp = bUseGUI ? new QApplication ( argc, argv )
                   : new QCoreApplication ( argc, argv );
#endif

    // init resources
    Q_INIT_RESOURCE(resources);


    try {
        if(!Startup.bNCentServPingServerInList)
        {
            if (Startup.strUserName.isEmpty()) {
                Startup.strUserName = GetClientName(Startup.strIniFileName);
            }

#ifndef HEADLESS
            if (bUseGUI) {

                if (Startup.strUserName.isEmpty() ||
                    Startup.strSessionName.isEmpty()) {

                    CStartupDlg StartupDlg(Startup, nullptr, Qt::Window);
                    StartupDlg.ShowDialog();
                }
            }  else
#endif
            {
                // only start application without using the GUI
                qInfo() << GetVersionAndNameStr(false);
            }

            if (Startup.strUserName.isEmpty()) {
                Startup.strUserName = "DefaultName";
                qWarning() << "No username specified - using default";
            }

            if (Startup.strSessionName.isEmpty()) {
                Startup.strSessionName = "DefaultSession";
                qWarning() << "No sessionname specified - using default";
            }

            if (Startup.bIsServer) {
                // start a server

                pServer = new CServer(  Startup.iNewMaxNumChan,
                                        Startup.strLoggingFileName,
                                        Startup.iPortNumberServer,
                                        Startup.strHTMLStatusFileName,
                                        Startup.strCentralServer,
                                        Startup.strSessionName + ";Zurich;206",
                                        Startup.strServerPublicIP,
                                        Startup.strServerListFilter,
                                        Startup.strNewWelcomeMessage,
                                        Startup.strRecordingDirName,
                                        Startup.bNDisconnectAllClientsOnQuit,
                                        Startup.bNUseDoubleSystemFrameSize,
                                        Startup.bUseMultithreading,
                                        Startup.bDisableRecording,
                                        Startup.eNLicenceType);

                // load settings from init-file
                // CServerSettings Settings ( &Server, Startup.strIniFileName );
                // Settings.Load();
                // load translation

                // update server list AFTER restoring the settings
                pServer->UpdateServerList();
            }

            pClient = new CClient(  Startup.iPortNumberClient,
                                    Startup.strSessionName,
                                    Startup.strMIDISetup,
                                    Startup.bNoAutoJackConnect,
                                    Startup.strUserName,
                                    Startup.bMuteMeInPersonalMix,
                                    Startup.strCentralServer,
                                    Startup.iNewMaxNumChan,
                                    false );

            // load settings from init-file
            /********** Muth Tempor<C3><A4>r ausschalten */
            CClientSettings Settings(pClient, Startup.strIniFileName);
            // Settings.Load(CommandLineOptions );
            qInfo() << "Init File not loaded:" << Startup.strIniFileName;

#ifndef HEADLESS
            if (bUseGUI) {
                // GUI object
                pClientDlg = new CClientDlg(pClient,
                                    &Settings,
                                    "", // strSessionName
                                    Startup.strMIDISetup, // new string
                                    false, // not needed for UMUTE :Startup.bNewShowComplRegConnList,
                                    Startup.bShowAnalyzerConsole,
                                    Startup.bMuteStream,
                                    nullptr, // server,
                                    nullptr); // qwidgetParent

                // show client dialog
                // qDebug() << "showing clientdlg";
                pClientDlg->setModal(true);
                pClientDlg->show();
            }
#endif
            if (Startup.bIsServer) {
                QObject::connect(pServer,
                                &CServer::ServerRegisteredSuccessfully,
                                pClient,
                                &CClient::OnServerRegisteredSuccessfully);

                // update serverlist
                pServer->UpdateServerList();
            }
            pApp->exec(); // nogui
        } else {
            qInfo() << "Central Server Mode";
            pServer = new CServer(  Startup.iNewMaxNumChan,
                                    Startup.strLoggingFileName,
                                    Startup.iPortNumberServer,
                                    Startup.strHTMLStatusFileName,
                                    Startup.strCentralServer,
                                    Startup.strSessionName + ";Zurich;206",
                                    Startup.strServerPublicIP,
                                    Startup.strServerListFilter,
                                    Startup.strNewWelcomeMessage,
                                    Startup.strRecordingDirName,
                                    Startup.bNDisconnectAllClientsOnQuit,
                                    Startup.bNUseDoubleSystemFrameSize,
                                    Startup.bUseMultithreading,
                                    Startup.bDisableRecording,
                                    Startup.eNLicenceType);

            // load settings from init-file
            // CServerSettings Settings ( &Server, Startup.strIniFileName );
            // Settings.Load();
            // load translation

            // update server list AFTER restoring the settings
            pServer->UpdateServerList();

            pApp->exec();
        }
    } catch (const CGenErr& generr) {
        qCritical() << "CRITICAL Error: Signal catched"
                    << generr.GetErrorText();

        // show generic error
#ifndef HEADLESS
        if ( bUseGUI )
        {
        QMessageBox::critical ( nullptr,
                                APP_NAME,
                                generr.GetErrorText(),
                                "Quit",
                                nullptr );
        }
#endif
    } catch (...) {
        qCritical() << "CRITICAL Error: Unknown Signal catched";
    }

    // qDebug() << "End of Main";
    return 0;
}

QByteArray FromBase64ToByteArray ( const QString strIn )
        { return QByteArray::fromBase64 ( strIn.toLatin1() ); }

QString FromBase64ToString ( const QString strIn )
        { return QString::fromUtf8 ( FromBase64ToByteArray ( strIn ) ); }

QString GetClientName(const QString& sNFiName)
{
    /* Get file path */
    QString strFileName = sNFiName;

    if ( strFileName.isEmpty() )
    {
        // we use the Qt default setting file paths for the different OSs by
        // utilizing the QSettings class
        const QString sConfigDir = QFileInfo ( QSettings ( QSettings::IniFormat,
                                                           QSettings::UserScope,
                                                           APP_NAME,
                                                           APP_NAME ).fileName() ).absolutePath();

        // make sure the directory exists
        if ( !QFile::exists ( sConfigDir ) )
        {
            QDir().mkpath ( sConfigDir );
        }

        // append the actual file name
        strFileName = sConfigDir + "/" + DEFAULT_INI_FILE_NAME;
    }

    /* Read file */
    QFile file ( strFileName );
    QDomDocument XMLDocument;

    if ( file.open ( QIODevice::ReadOnly ) )
    {
        XMLDocument.setContent ( QTextStream ( &file ).readAll(), false );
        file.close();
    }

    /*Get client name*/
    QString sResult("DefaultName");
    QDomElement xmlSection = XMLDocument.firstChildElement ( "client" );
    if ( !xmlSection.isNull() )
    {
        // get key
        QDomElement xmlKey = xmlSection.firstChildElement ( "name_base64" );
        if ( !xmlKey.isNull() )
        {
            // get value
            sResult = FromBase64ToString (xmlKey.text());
        }
    }
    return sResult;
}


/******************************************************************************\
* Command Line Argument Parsing                                                *
\******************************************************************************/
QString UsageArguments ( char **argv )
{
    return
        "Usage: " + QString ( argv[0] ) + " [option] [optional argument]\n"
        "\nRecognized options:\n"
        "  -h, -?, --help        display this help text and exit\n"
        "  -i, --inifile         initialization file name (not\n"
        "                        supported for headless server mode)\n"
        "  -n, --nogui           disable GUI\n"
        "  -p, --port            set your local port number\n"
        "  -t, --notranslation   disable translation (use English language)\n"
        "  -v, --version         output version information and exit\n"
        "\nServer only:\n"
        "  -d, --discononquit    disconnect all clients on quit\n"
        "  -e, --centralserver   address of the server list on which to register\n"
        "                        (or 'localhost' to be a server list)\n"
        "  -f, --listfilter      server list whitelist filter in the format:\n"
        "                        [IP address 1];[IP address 2];[IP address 3]; ...\n"
        "  -F, --fastupdate      use 64 samples frame size mode\n"
        "  -l, --log             enable logging, set file name\n"
        "  -L, --licence         show an agreement window before users can connect\n"
        "  -m, --htmlstatus      enable HTML status file, set file name\n"
        "  -o, --serverinfo      infos of this server in the format:\n"
        "                        [name];[city];[country as QLocale ID]\n"
        "  -R, --recording       sets directory to contain recorded jams\n"
        "      --norecord        disables recording (when enabled by default by -R)\n"
        "  -s, --server          start server\n"
        "  -T, --multithreading  use multithreading to make better use of\n"
        "                        multi-core CPUs and support more clients\n"
        "  -u, --numchannels     maximum number of channels\n"
        "  -w, --welcomemessage  welcome message on connect\n"
        "  -z, --startminimized  start minimizied\n"
        "      --serverpublicip  specify your public IP address when\n"
        "                        running a slave and your own central server\n"
        "                        behind the same NAT\n"
        "\nClient only:\n"
        "  -M, --mutestream      starts the application in muted state\n"
        "      --mutemyown       mute me in my personal mix (headless only)\n"
        "  -c, --connect         connect to given server address on startup\n"
        "  -j, --nojackconnect   disable auto Jack connections\n"
        "      --ctrlmidich      MIDI controller channel to listen\n"
        "      --clientname      client name (window title and jack client name)\n"
        "\nExample: " + QString ( argv[0] ) + " -s --inifile myinifile.ini\n";
}

bool GetFlagArgument ( char**  argv,
                       int&    i,
                       QString strShortOpt,
                       QString strLongOpt )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool GetStringArgument ( int          argc,
                         char**       argv,
                         int&         i,
                         QString      strShortOpt,
                         QString      strLongOpt,
                         QString&     strArg )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        if ( ++i >= argc )
        {
            qCritical() << qUtf8Printable( QString( "%1: '%2' needs a string argument." )
                .arg( argv[0] ).arg( strLongOpt ) );
            exit ( 1 );
        }

        strArg = argv[i];

        return true;
    }
    else
    {
        return false;
    }
}

bool GetNumericArgument ( int          argc,
                          char**       argv,
                          int&         i,
                          QString      strShortOpt,
                          QString      strLongOpt,
                          double       rRangeStart,
                          double       rRangeStop,
                          double&      rValue )
{
    if ( ( !strShortOpt.compare ( argv[i] ) ) ||
         ( !strLongOpt.compare ( argv[i] ) ) )
    {
        QString errmsg = "%1: '%2' needs a numeric argument between '%3' and '%4'.";
        if ( ++i >= argc )
        {
            qCritical() << qUtf8Printable( errmsg
                .arg( argv[0] ).arg( strLongOpt ).arg( rRangeStart ).arg( rRangeStop ) );
            exit ( 1 );
        }

        char *p;
        rValue = strtod ( argv[i], &p );
        if ( *p ||
             ( rValue < rRangeStart ) ||
             ( rValue > rRangeStop ) )
        {
            qCritical() << qUtf8Printable( errmsg
                .arg( argv[0] ).arg( strLongOpt ).arg( rRangeStart ).arg( rRangeStop ) );
            exit ( 1 );
        }

        return true;
    }
    else
    {
        return false;
    }
}
