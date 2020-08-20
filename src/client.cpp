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

#include "client.h"


/* Implementation *************************************************************/
CClient::CClient ( const quint16  iPortNumber,
                   const QString& strConnOnStartupAddress,
                   const QString& strMIDISetup,
                   const bool     bNoAutoJackConnect,
                   const QString& strNClientName,
                   const bool     bNMuteMeInPersonalMix,
                   const QString& strCentralServer,
                   const int      iNewMaxNumChan,
                   const bool     localServer ) :
    ChannelInfo                      ( strNClientName ),
    strClientName                    ( "Jamulus" ),
    Channel                          ( false, false ), /* we need a client channel -> "false" */
    CurOpusEncoder                   ( nullptr ),
    CurOpusDecoder                   ( nullptr ),
    eAudioCompressionType            ( CT_OPUS ),
    iCeltNumCodedBytes               ( OPUS_NUM_BYTES_MONO_LOW_QUALITY ),
    iOPUSFrameSizeSamples            ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ),
    eAudioQuality                    ( AQ_NORMAL ),
    eAudioChannelConf                ( CC_MONO ),
    iNumAudioChannels                ( 1 ),
    bIsInitializationPhase           ( true ),
    bMuteOutStream                   ( false ),
    fMuteOutStreamGain               ( 1.0f ),
    Socket                           ( this , &Channel, iPortNumber ),
    Sound                            ( AudioCallback, this, strMIDISetup, bNoAutoJackConnect, strNClientName ),
    iAudioInFader                    ( AUD_FADER_IN_MIDDLE ),
    bReverbOnLeftChan                ( false ),
    iReverbLevel                     ( 0 ),
    iSndCrdPrefFrameSizeFactor       ( FRAME_SIZE_FACTOR_DEFAULT ),
    iSndCrdFrameSizeFactor           ( FRAME_SIZE_FACTOR_DEFAULT ),
    bSndCrdConversionBufferRequired  ( false ),
    iSndCardMonoBlockSizeSamConvBuff ( 0 ),
    bFraSiFactPrefSupported          ( false ),
    bFraSiFactDefSupported           ( false ),
    bFraSiFactSafeSupported          ( false ),
    eGUIDesign                       ( GD_ORIGINAL ),
    bEnableOPUS64                    ( false ),
    bJitterBufferOK                  ( true ),
    bNuteMeInPersonalMix             ( bNMuteMeInPersonalMix ),
    iServerSockBufNumFrames          ( DEF_NET_BUF_SIZE_NUM_BL ),
    pSignalHandler                   ( CSignalHandler::getSingletonP() ),
    strStartupAddress                ( strConnOnStartupAddress ),
    strCentralServerAddressClient    ( strCentralServer ),
    p2pEnabled                       ( false ),
    iMaxNumChannels                  ( iNewMaxNumChan ),
    bLocalServer                     ( localServer ),
    iPort                            ( iPortNumber ),
    serverNameChanged                ( false )
{
    int iOpusError;
    int i;


    // P2P: enable all channels (all channel must be enabled the
    // entire life time of the software)
    // set to bIsServer to false
    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        p2pChannels[i].SetIsServer( false );
        p2pChannels[i].SetP2pType( true );
    }

    OpusMode = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                         DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES,
                                         &iOpusError );

    Opus64Mode = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                           SYSTEM_FRAME_SIZE_SAMPLES,
                                           &iOpusError );

    // init audio encoders and decoders
    OpusEncoderMono     = opus_custom_encoder_create ( OpusMode,   1, &iOpusError ); // mono encoder legacy
    OpusDecoderMono     = opus_custom_decoder_create ( OpusMode,   1, &iOpusError ); // mono decoder legacy
    OpusEncoderStereo   = opus_custom_encoder_create ( OpusMode,   2, &iOpusError ); // stereo encoder legacy
    OpusDecoderStereo   = opus_custom_decoder_create ( OpusMode,   2, &iOpusError ); // stereo decoder legacy
    Opus64EncoderMono   = opus_custom_encoder_create ( Opus64Mode, 1, &iOpusError ); // mono encoder OPUS64
    Opus64DecoderMono   = opus_custom_decoder_create ( Opus64Mode, 1, &iOpusError ); // mono decoder OPUS64
    Opus64EncoderStereo = opus_custom_encoder_create ( Opus64Mode, 2, &iOpusError ); // stereo encoder OPUS64
    Opus64DecoderStereo = opus_custom_decoder_create ( Opus64Mode, 2, &iOpusError ); // stereo decoder OPUS64

    // we require a constant bit rate
    opus_custom_encoder_ctl ( OpusEncoderMono,     OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo,   OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_VBR ( 0 ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_VBR ( 0 ) );

    // for 64 samples frame size we have to adjust the PLC behavior to avoid loud artifacts
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_PACKET_LOSS_PERC ( 35 ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_PACKET_LOSS_PERC ( 35 ) );

    // we want as low delay as possible
    opus_custom_encoder_ctl ( OpusEncoderMono,     OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo,   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( Opus64EncoderMono,   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
    opus_custom_encoder_ctl ( Opus64EncoderStereo, OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

    // set encoder low complexity for legacy 128 samples frame size
    opus_custom_encoder_ctl ( OpusEncoderMono,   OPUS_SET_COMPLEXITY ( 1 ) );
    opus_custom_encoder_ctl ( OpusEncoderStereo, OPUS_SET_COMPLEXITY ( 1 ) );

    // P2P initialisations

    // create OPUS encoder/decoder for each channel (must be done before
    // enabling the channels), create a mono and stereo encoder/decoder
    // for each channel
    for ( i = 0; i < iMaxNumChannels; i++ )
    {
        // init OPUS -----------------------------------------------------------
        p2pOpusMode[i] = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                                DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES,
                                                &iOpusError );

        p2pOpus64Mode[i] = opus_custom_mode_create ( SYSTEM_SAMPLE_RATE_HZ,
                                                  SYSTEM_FRAME_SIZE_SAMPLES,
                                                  &iOpusError );

        // init audio encoders and decoders
        p2pOpusEncoderMono[i]     = opus_custom_encoder_create ( p2pOpusMode[i],   1, &iOpusError ); // mono encoder legacy
        p2pOpusDecoderMono[i]     = opus_custom_decoder_create ( p2pOpusMode[i],   1, &iOpusError ); // mono decoder legacy
        p2pOpusEncoderStereo[i]   = opus_custom_encoder_create ( p2pOpusMode[i],   2, &iOpusError ); // stereo encoder legacy
        p2pOpusDecoderStereo[i]   = opus_custom_decoder_create ( p2pOpusMode[i],   2, &iOpusError ); // stereo decoder legacy
        p2pOpus64EncoderMono[i]   = opus_custom_encoder_create ( p2pOpus64Mode[i], 1, &iOpusError ); // mono encoder OPUS64
        p2pOpus64DecoderMono[i]   = opus_custom_decoder_create ( p2pOpus64Mode[i], 1, &iOpusError ); // mono decoder OPUS64
        p2pOpus64EncoderStereo[i] = opus_custom_encoder_create ( p2pOpus64Mode[i], 2, &iOpusError ); // stereo encoder OPUS64
        p2pOpus64DecoderStereo[i] = opus_custom_decoder_create ( p2pOpus64Mode[i], 2, &iOpusError ); // stereo decoder OPUS64

        // we require a constant bit rate
        opus_custom_encoder_ctl ( p2pOpusEncoderMono[i],     OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( p2pOpusEncoderStereo[i],   OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( p2pOpus64EncoderMono[i],   OPUS_SET_VBR ( 0 ) );
        opus_custom_encoder_ctl ( p2pOpus64EncoderStereo[i], OPUS_SET_VBR ( 0 ) );

        // for 64 samples frame size we have to adjust the PLC behavior to avoid loud artifacts
        opus_custom_encoder_ctl ( p2pOpus64EncoderMono[i],   OPUS_SET_PACKET_LOSS_PERC ( 35 ) );
        opus_custom_encoder_ctl ( p2pOpus64EncoderStereo[i], OPUS_SET_PACKET_LOSS_PERC ( 35 ) );

        // we want as low delay as possible
        opus_custom_encoder_ctl ( p2pOpusEncoderMono[i],     OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( p2pOpusEncoderStereo[i],   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( p2pOpus64EncoderMono[i],   OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );
        opus_custom_encoder_ctl ( p2pOpus64EncoderStereo[i], OPUS_SET_APPLICATION ( OPUS_APPLICATION_RESTRICTED_LOWDELAY ) );

        // set encoder low complexity for legacy 128 samples frame size
        opus_custom_encoder_ctl ( p2pOpusEncoderMono[i],   OPUS_SET_COMPLEXITY ( 1 ) );
        opus_custom_encoder_ctl ( p2pOpusEncoderStereo[i], OPUS_SET_COMPLEXITY ( 1 ) );
    }

    // Connections -------------------------------------------------------------
    // connections for the protocol mechanism
    QObject::connect ( &Channel, &CChannel::MessReadyForSending,
        this, &CClient::OnSendProtMessage );

    QObject::connect ( &Channel, &CChannel::DetectedCLMessage,
        this, &CClient::OnDetectedCLMessage );

    QObject::connect ( &Channel, &CChannel::ReqJittBufSize,
        this, &CClient::OnReqJittBufSize );

    QObject::connect ( &Channel, &CChannel::JittBufSizeChanged,
        this, &CClient::OnJittBufSizeChanged );

    QObject::connect ( &Channel, &CChannel::ReqChanInfo,
        this, &CClient::OnReqChanInfo );

    QObject::connect ( &Channel, &CChannel::ConClientListMesReceived,
        this, &CClient::ConClientListMesReceived );

    QObject::connect ( &Channel, &CChannel::ConClientListMesReceived,
        this, &CClient::OnConClientListMesReceived );

    QObject::connect ( &Channel, &CChannel::Disconnected,
        this, &CClient::Disconnected );

    QObject::connect ( &Channel, &CChannel::NewConnection,
        this, &CClient::OnNewConnection );

    QObject::connect ( &Channel, &CChannel::ChatTextReceived,
        this, &CClient::ChatTextReceived );

    QObject::connect ( &Channel, &CChannel::ClientIDReceived,
        this, &CClient::OnClientIDReceived );

    QObject::connect ( &Channel, &CChannel::MuteStateHasChangedReceived,
        this, &CClient::MuteStateHasChangedReceived );

    QObject::connect ( &Channel, &CChannel::LicenceRequired,
        this, &CClient::LicenceRequired );

    QObject::connect ( &Channel, &CChannel::VersionAndOSReceived,
        this, &CClient::VersionAndOSReceived );

    QObject::connect ( &Channel, &CChannel::RecorderStateReceived,
        this, &CClient::RecorderStateReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLMessReadyForSending,
        this, &CClient::OnSendCLProtMessage );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLServerListReceived,
        this, &CClient::CLServerListReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLRedServerListReceived,
        this, &CClient::CLRedServerListReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLServerIpReceived,
        this, &CClient::OnCLServerIpReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPublicIpRec,
        this, &CClient::OnCLPublicIpRec );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLConnClientsListMesReceived,
        this, &CClient::CLConnClientsListMesReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingReceived,
        this, &CClient::OnCLPingReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLPingWithNumClientsReceived,
        this, &CClient::OnCLPingWithNumClientsReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLDisconnection ,
        this, &CClient::OnCLDisconnection );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLVersionAndOSReceived,
        this, &CClient::CLVersionAndOSReceived );

    QObject::connect ( &ConnLessProtocol, &CProtocol::CLChannelLevelListReceived,
        this, &CClient::CLChannelLevelListReceived );

    // other
    QObject::connect ( &Sound, &CSound::ReinitRequest,
        this, &CClient::OnSndCrdReinitRequest );

    QObject::connect ( &Sound, &CSound::ControllerInFaderLevel,
        this, &CClient::OnControllerInFaderLevel );

    QObject::connect ( &Sound, &CSound::ControllerInPanValue,
        this, &CClient::OnControllerInPanValue );

    QObject::connect ( &Sound, &CSound::ControllerInFaderIsSolo,
        this, &CClient::OnControllerInFaderIsSolo );

    QObject::connect ( &Sound, &CSound::ControllerInFaderIsMute,
        this, &CClient::OnControllerInFaderIsMute );

    QObject::connect ( &Socket, &CHighPrioSocket::InvalidPacketReceived,
        this, &CClient::OnInvalidPacketReceived );

    QObject::connect ( pSignalHandler, &CSignalHandler::HandledSignal,
        this, &CClient::OnHandledSignal );

    // start timer so that elapsed time works
    PreciseTime.start();

    // timer
    QObject::connect ( &TimerClientReReqServList, &QTimer::timeout,
        this, &CClient::OnTimerClientReReqServList );

    QObject::connect ( &TimerPingP2pClients, &QTimer::timeout,
        this, &CClient::OnTimerPingP2pClients );

    QObject::connect ( this, &CClient::Stopped,
        &JamController, &recorder::CJamController::Stopped );

    QObject::connect ( QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
        this, &CClient::OnAboutToQuit );

    // QObject::connect ( &JamController, &recorder::CJamController::RestartRecorder,
    //     this, &CClient::RestartRecorder ); --> seems to do nothing

    // QObject::connect ( &JamController, &recorder::CJamController::StopRecorder,
    //     this, &CClient::StopRecorder ); --> updates recorder state in serverdlg

    // QObject::connect ( &JamController, &recorder::CJamController::RecordingSessionStarted,
    //     this, &CServer::RecordingSessionStarted );

    // QObject::connect ( &JamController, &recorder::CJamController::EndRecorderThread,
    //     this, &CServer::EndRecorderThread );

    qRegisterMetaType<CVector<int16_t>> ( "CVector<int16_t>" );
    QObject::connect ( this, &CClient::AudioFrame,
        &JamController, &recorder::CJamController::AudioFrame );

    bRecorderEnabled = false;
    bStopRecorder = false;

    // start the socket (it is important to start the socket after all
    // initializations and connections)
    Socket.Start();

    // If there is no local server started, instantly connect to server
    // else wait untli localserver has successfully registered
    if ( !bLocalServer )
    {
        // do an immediate start if a server address is given
        if ( !strConnOnStartupAddress.isEmpty() )
        {
            if ( strConnOnStartupAddress == "127.0.0.1" )
            {
                SetServerAddr ( strConnOnStartupAddress );
                Start();
            }
            else
            {
                // Start waiting for server list from central server
                waitingForIp = true;
                TimerClientReReqServList.start( 1000 ); //ms
            }
        }
    }
}

CClient::~CClient()
{
    // if we were running, stop sound device
    if ( Sound.IsRunning() )
    {
        Sound.Stop();
    }

    // free audio encoders and decoders
    opus_custom_encoder_destroy ( OpusEncoderMono );
    opus_custom_decoder_destroy ( OpusDecoderMono );
    opus_custom_encoder_destroy ( OpusEncoderStereo );
    opus_custom_decoder_destroy ( OpusDecoderStereo );
    opus_custom_encoder_destroy ( Opus64EncoderMono );
    opus_custom_decoder_destroy ( Opus64DecoderMono );
    opus_custom_encoder_destroy ( Opus64EncoderStereo );
    opus_custom_decoder_destroy ( Opus64DecoderStereo );

    // free audio modes
    opus_custom_mode_destroy ( OpusMode );
    opus_custom_mode_destroy ( Opus64Mode );

    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
       // free audio encoders and decoders
        opus_custom_encoder_destroy ( p2pOpusEncoderMono[i] );
        opus_custom_decoder_destroy ( p2pOpusDecoderMono[i] );
        opus_custom_encoder_destroy ( p2pOpusEncoderStereo[i] );
        opus_custom_decoder_destroy ( p2pOpusDecoderStereo[i] );
        opus_custom_encoder_destroy ( p2pOpus64EncoderMono[i] );
        opus_custom_decoder_destroy ( p2pOpus64DecoderMono[i] );
        opus_custom_encoder_destroy ( p2pOpus64EncoderStereo[i] );
        opus_custom_decoder_destroy ( p2pOpus64DecoderStereo[i] );

        // free audio modes
        opus_custom_mode_destroy ( p2pOpusMode[i] );
        opus_custom_mode_destroy ( p2pOpus64Mode[i] );
    }
}

void CClient::OnSendProtMessage ( CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, Channel.GetAddress() );
}

void CClient::OnSendCLProtMessage ( CHostAddress     InetAddr,
                                    CVector<uint8_t> vecMessage )
{
    // the protocol queries me to call the function to send the message
    // send it through the network
    Socket.SendPacket ( vecMessage, InetAddr );
}

void CClient::OnInvalidPacketReceived ( CHostAddress RecHostAddr )
{
    // message could not be parsed, check if the packet comes
    // from the server we just connected -> if yes, send
    // disconnect message since the server may not know that we
    // are not connected anymore
    if ( Channel.GetAddress() == RecHostAddr )
    {
        ConnLessProtocol.CreateCLDisconnection ( RecHostAddr );
    }
}

void CClient::OnDetectedCLMessage ( CVector<uint8_t> vecbyMesBodyData,
                                    int              iRecID,
                                    CHostAddress     RecHostAddr )
{
    // connection less messages are always processed
    ConnLessProtocol.ParseConnectionLessMessageBody ( vecbyMesBodyData,
                                                      iRecID,
                                                      RecHostAddr );
}

void CClient::OnJittBufSizeChanged ( int iNewJitBufSize )
{
    // we received a jitter buffer size changed message from the server,
    // only apply this value if auto jitter buffer size is enabled
    if ( GetDoAutoSockBufSize() )
    {
        // Note: Do not use the "SetServerSockBufNumFrames" function for setting
        // the new server jitter buffer size since then a message would be sent
        // to the server which is incorrect.
        iServerSockBufNumFrames = iNewJitBufSize;
    }
}

void CClient::OnNewConnection()
{
    // a new connection was successfully initiated, send infos and request
    // connected clients list
    Channel.SetRemoteInfo ( ChannelInfo );

    // We have to send a connected clients list request since it can happen
    // that we just had connected to the server and then disconnected but
    // the server still thinks that we are connected (the server is still
    // waiting for the channel time-out). If we now connect again, we would
    // not get the list because the server does not know about a new connection.
    // Same problem is with the jitter buffer message.
    Channel.CreateReqConnClientsList();
    CreateServerJitterBufferMessage();

// TODO needed for compatibility to old servers >= 3.4.6 and <= 3.5.12
Channel.CreateReqChannelLevelListMes();
}

void CClient::CreateServerJitterBufferMessage()
{
    // per definition in the client: if auto jitter buffer is enabled, both,
    // the client and server shall use an auto jitter buffer
    if ( GetDoAutoSockBufSize() )
    {
        // in case auto jitter buffer size is enabled, we have to transmit a
        // special value
        Channel.CreateJitBufMes ( AUTO_NET_BUF_SIZE_FOR_PROTOCOL );
    }
    else
    {
        Channel.CreateJitBufMes ( GetServerSockBufNumFrames() );
    }
}

void CClient::OnCLPingReceived ( CHostAddress InetAddr,
                                 int          iMs )
{
    // make sure we are running and the server address is correct
    if ( IsRunning() && ( InetAddr == Channel.GetAddress() ) )
    {
        // take care of wrap arounds (if wrapping, do not use result)
        const int iCurDiff = EvaluatePingMessage ( iMs );
        if ( iCurDiff >= 0 )
        {
            emit PingTimeReceived ( iCurDiff );
        }
    }
}

void CClient::OnCLPingWithNumClientsReceived ( CHostAddress InetAddr,
                                               int          iMs,
                                               int          iNumClients )
{
    // take care of wrap arounds (if wrapping, do not use result)
    const int iCurDiff = EvaluatePingMessage ( iMs );
    if ( iCurDiff >= 0 )
    {
        emit CLPingTimeWithNumClientsReceived ( InetAddr,
                                                iCurDiff,
                                                iNumClients );
    }
}

int CClient::PreparePingMessage()
{
    // transmit the current precise time (in ms)
    return PreciseTime.elapsed();
}

int CClient::EvaluatePingMessage ( const int iMs )
{
    // calculate difference between received time in ms and current time in ms
    return PreciseTime.elapsed() - iMs;
}

void CClient::SetDoAutoSockBufSize ( const bool bValue )
{
    // first, set new value in the channel object
    Channel.SetDoAutoSockBufSize ( bValue );

    // inform the server about the change
    CreateServerJitterBufferMessage();
}

void CClient::SetRemoteChanGain ( const int   iId,
                                  const float fGain,
                                  const bool  bIsMyOwnFader,
                                  const bool  bDoServerUpdate,
                                  const bool  bDoClientUpdate )
{
    if ( bDoClientUpdate )
    {
        // if this gain is for my own channel, apply the value for the Mute Myself function
        if ( bIsMyOwnFader )
        {
            fMuteOutStreamGain = fGain;
        }

        for ( int i = 0; i < p2pNumClientIps; i++ )
        {
            if ( p2pChannels[i].GetChannelID() == iId )
            {
                p2pChannels[i].SetP2pGain(fGain);
            }
            break;
        }
    }

    if ( bDoServerUpdate )
    {
        Channel.SetRemoteChanGain ( iId, fGain );
    }
}

bool CClient::SetServerAddr ( QString strNAddr )
{
    CHostAddress HostAddress;
    if ( NetworkUtil().ParseNetworkAddress ( strNAddr,
                                             HostAddress ) )
    {
        // apply address to the channel
        Channel.SetAddress ( HostAddress );

        return true;
    }
    else
    {
        return false; // invalid address
    }
}

bool CClient::GetAndResetbJitterBufferOKFlag()
{
    // get the socket buffer put status flag and reset it
    const bool bSocketJitBufOKFlag = Socket.GetAndResetbJitterBufferOKFlag();

    if ( !bJitterBufferOK )
    {
        // our jitter buffer get status is not OK so the overall status of the
        // jitter buffer is also not OK (we do not have to consider the status
        // of the socket buffer put status flag)

        // reset flag before returning the function
        bJitterBufferOK = true;
        return false;
    }

    // the jitter buffer get (our own status flag) is OK, the final status
    // now depends on the jitter buffer put status flag from the socket
    // since per definition the jitter buffer status is OK if both the
    // put and get status are OK
    return bSocketJitBufOKFlag;
}

void CClient::SetSndCrdPrefFrameSizeFactor ( const int iNewFactor )
{
    // first check new input parameter
    if ( ( iNewFactor == FRAME_SIZE_FACTOR_PREFERRED ) ||
         ( iNewFactor == FRAME_SIZE_FACTOR_DEFAULT ) ||
         ( iNewFactor == FRAME_SIZE_FACTOR_SAFE ) )
    {
        // init with new parameter, if client was running then first
        // stop it and restart again after new initialization
        const bool bWasRunning = Sound.IsRunning();
        if ( bWasRunning )
        {
            Sound.Stop();
        }

        // set new parameter
        iSndCrdPrefFrameSizeFactor = iNewFactor;

        // init with new block size index parameter
        Init();

        if ( bWasRunning )
        {
            // restart client
            Sound.Start();
        }
    }
}

void CClient::SetEnableOPUS64 ( const bool eNEnableOPUS64 )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    bEnableOPUS64 = eNEnableOPUS64;
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

void CClient::SetAudioQuality ( const EAudioQuality eNAudioQuality )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    eAudioQuality = eNAudioQuality;
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

void CClient::SetAudioChannels ( const EAudChanConf eNAudChanConf )
{
    // init with new parameter, if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    // set new parameter
    eAudioChannelConf = eNAudChanConf;
    Init();

    if ( bWasRunning )
    {
        Sound.Start();
    }
}

QString CClient::SetSndCrdDev ( const QString strNewDev )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    const QString strError = Sound.SetDev ( strNewDev );

    // init again because the sound card actual buffer size might
    // be changed on new device
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }

    // in case of an error inform the GUI about it
    if ( !strError.isEmpty() )
    {
        emit SoundDeviceChanged ( strError );
    }

    return strError;
}

void CClient::SetSndCrdLeftInputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetLeftInputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdRightInputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetRightInputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdLeftOutputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetLeftOutputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::SetSndCrdRightOutputChannel ( const int iNewChan )
{
    // if client was running then first
    // stop it and restart again after new initialization
    const bool bWasRunning = Sound.IsRunning();
    if ( bWasRunning )
    {
        Sound.Stop();
    }

    Sound.SetRightOutputChannel ( iNewChan );
    Init();

    if ( bWasRunning )
    {
        // restart client
        Sound.Start();
    }
}

void CClient::OnSndCrdReinitRequest ( int iSndCrdResetType )
{
    QString strError = "";

    // audio device notifications can come at any time and they are in a
    // different thread, therefore we need a mutex here
    MutexDriverReinit.lock();
    {
        // in older QT versions, enums cannot easily be used in signals without
        // registering them -> workaroud: we use the int type and cast to the enum
        const ESndCrdResetType eSndCrdResetType =
            static_cast<ESndCrdResetType> ( iSndCrdResetType );

        // if client was running then first
        // stop it and restart again after new initialization
        const bool bWasRunning = Sound.IsRunning();
        if ( bWasRunning )
        {
            Sound.Stop();
        }

        // perform reinit request as indicated by the request type parameter
        if ( eSndCrdResetType != RS_ONLY_RESTART )
        {
            if ( eSndCrdResetType != RS_ONLY_RESTART_AND_INIT )
            {
                // reinit the driver if requested
                // (we use the currently selected driver)
                strError = Sound.SetDev ( Sound.GetDev() );
            }

            // init client object (must always be performed if the driver
            // was changed)
            Init();
        }

        if ( bWasRunning )
        {
            // restart client
            Sound.Start();
        }
    }
    MutexDriverReinit.unlock();

    // inform GUI about the sound card device change
    emit SoundDeviceChanged ( strError );
}

void CClient::OnAboutToQuit()
{
    // if connected, terminate connection (needed for headless mode)
    if ( IsRunning() )
    {
        Stop();
    }

    if ( JamController.GetRecordingEnabled() )
    {
        emit Stopped();
    }


    // this should trigger OnAboutToQuit
    QCoreApplication::instance()->exit();
}

void CClient::OnHandledSignal ( int sigNum )
{
#ifdef _WIN32
    // Windows does not actually get OnHandledSignal triggered
    QCoreApplication::instance()->exit();
    Q_UNUSED ( sigNum )
#else

    switch ( sigNum )
    {
    case SIGINT:
    case SIGTERM:
        // if connected, terminate connection (needed for headless mode)
        if ( IsRunning() )
        {
            Stop();
        }

        if ( JamController.GetRecordingEnabled() )
        {
            emit Stopped();
        }


        // this should trigger OnAboutToQuit
        QCoreApplication::instance()->exit();
        break;

    default:
        break;
    }
#endif
}

void CClient::OnControllerInFaderLevel ( int iChannelIdx,
                                         int iValue )
{
    // in case of a headless client the faders cannot be moved so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // only apply new fader level if channel index is valid
    if ( ( iChannelIdx >= 0 ) && ( iChannelIdx < MAX_NUM_CHANNELS ) )
    {
        SetRemoteChanGain ( iChannelIdx, MathUtils::CalcFaderGain ( iValue ), false, true, true );
    }
#endif

    emit ControllerInFaderLevel ( iChannelIdx, iValue );
}

void CClient::OnControllerInPanValue ( int iChannelIdx,
                                       int iValue )
{
    // in case of a headless client the panners cannot be moved so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // channel index is valid
    SetRemoteChanPan ( iChannelIdx, static_cast<float>( iValue ) / AUD_MIX_PAN_MAX);
#endif

    emit ControllerInPanValue ( iChannelIdx, iValue );
}

void CClient::OnControllerInFaderIsSolo ( int iChannelIdx,
                                          bool bIsSolo )
{
    // in case of a headless client the buttons are not displayed so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // FIXME: no idea what to do here.
#endif

    emit ControllerInFaderIsSolo ( iChannelIdx, bIsSolo );
}

void CClient::OnControllerInFaderIsMute ( int iChannelIdx,
                                          bool bIsMute )
{
    // in case of a headless client the buttons are not displayed so we need
    // to send the controller information directly to the server
#ifdef HEADLESS
    // FIXME: no idea what to do here.
#endif

    emit ControllerInFaderIsMute ( iChannelIdx, bIsMute );
}

void CClient::OnClientIDReceived ( int iChanID )
{
    // for headless mode we support to mute our own signal in the personal mix
    // (note that the check for headless is done in the main.cpp and must not
    // be checked here)
    if ( bNuteMeInPersonalMix )
    {
        SetRemoteChanGain ( iChanID, 0, false, true, false );
    }

    emit ClientIDReceived ( iChanID );
}

void CClient::OnCLServerIpReceived ( CHostAddress,
                                     CVector<CServerInfo> vecServerInfo )
{
    int vecSize = vecServerInfo.Size();
    if ( waitingForIp && (vecSize > 1) )
    {
        //get Ip from server list
        waitingForIp = false;
        for ( int i = 0; i<vecSize ; i++){
            // qInfo() << "DEBUG Hostaddr: " << i << " " << vecServerInfo[i].HostAddr.toString();
            // qInfo() << "DEBUG name: " << i << " " << vecServerInfo[i].strName;
            if ( vecServerInfo[i].HostAddr.toString() != "0.0.0.0:0" )
            {
                // qInfo() << "DEBUG connection! ";
                emit ServerConnection( vecServerInfo[i].HostAddr.toString(), vecServerInfo[i].strName );
                break;
            }
        }
        //SetServerAddr ( vecServerInfo[1].HostAddr.toString() );
        //Start(); -> option without GUI
    }
}

void CClient::Start()
{
    if ( serverNameChanged )
    {
        emit SessionNameChanged ( strStartupAddress );
    }

    // init object
    Init();

    // enable channel
    Channel.SetEnable ( true );

    // start audio interface
    Sound.Start();
}

void CClient::Stop()
{
    // stop audio interface
    Sound.Stop();

    // disable channel
    Channel.SetEnable ( false );

    // wait for approx. 100 ms to make sure no audio packet is still in the
    // network queue causing the channel to be reconnected right after having
    // received the disconnect message (seems not to gain much, disconnect is
    // still not working reliably)
    QTime DieTime = QTime::currentTime().addMSecs ( 100 );
    while ( QTime::currentTime() < DieTime )
    {
        // exclude user input events because if we use AllEvents, it happens
        // that if the user initiates a connection and disconnection quickly
        // (e.g. quickly pressing enter five times), the software can get into
        // an unknown state
        QCoreApplication::processEvents (
            QEventLoop::ExcludeUserInputEvents, 100 );
    }

    // Send disconnect message to server (Since we disable our protocol
    // receive mechanism with the next command, we do not evaluate any
    // respond from the server, therefore we just hope that the message
    // gets its way to the server, if not, the old behaviour time-out
    // disconnects the connection anyway).
    ConnLessProtocol.CreateCLDisconnection ( Channel.GetAddress() );

    // reset current signal level and LEDs
    bJitterBufferOK = true;
    SignalLevelMeter.Reset();
}

void CClient::Init()
{
    // check if possible frame size factors are supported
    const int iFraSizePreffered = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED;
    const int iFraSizeDefault   = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT;
    const int iFraSizeSafe      = SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE;

    bFraSiFactPrefSupported = ( Sound.Init ( iFraSizePreffered ) == iFraSizePreffered );
    bFraSiFactDefSupported  = ( Sound.Init ( iFraSizeDefault )   == iFraSizeDefault );
    bFraSiFactSafeSupported = ( Sound.Init ( iFraSizeSafe )      == iFraSizeSafe );

    // translate block size index in actual block size
    const int iPrefMonoFrameSize = iSndCrdPrefFrameSizeFactor * SYSTEM_FRAME_SIZE_SAMPLES;

    // get actual sound card buffer size using preferred size
    iMonoBlockSizeSam = Sound.Init ( iPrefMonoFrameSize );

    // Calculate the current sound card frame size factor. In case
    // the current mono block size is not a multiple of the system
    // frame size, we have to use a sound card conversion buffer.
    if ( ( ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_PREFERRED ) ) && bEnableOPUS64 ) ||
         ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_DEFAULT ) ) ||
         ( iMonoBlockSizeSam == ( SYSTEM_FRAME_SIZE_SAMPLES * FRAME_SIZE_FACTOR_SAFE ) ) )
    {
        // regular case: one of our predefined buffer sizes is available
        iSndCrdFrameSizeFactor = iMonoBlockSizeSam / SYSTEM_FRAME_SIZE_SAMPLES;

        // no sound card conversion buffer required
        bSndCrdConversionBufferRequired = false;
    }
    else
    {
        // An unsupported sound card buffer size is currently used -> we have
        // to use a conversion buffer. Per definition we use the smallest buffer
        // size as the current frame size.

        // store actual sound card buffer size (stereo)
        bSndCrdConversionBufferRequired  = true;
        iSndCardMonoBlockSizeSamConvBuff = iMonoBlockSizeSam;

        // overwrite block size factor by using one frame
        iSndCrdFrameSizeFactor = 1;
    }

    // select the OPUS frame size mode depending on current mono block size samples
    if ( bSndCrdConversionBufferRequired )
    {
        if ( ( iSndCardMonoBlockSizeSamConvBuff < DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES ) && bEnableOPUS64 )
        {
            iMonoBlockSizeSam     = SYSTEM_FRAME_SIZE_SAMPLES;
            eAudioCompressionType = CT_OPUS64;
        }
        else
        {
            iMonoBlockSizeSam     = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;
            eAudioCompressionType = CT_OPUS;
        }
    }
    else
    {
        if ( iMonoBlockSizeSam < DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  )
        {
            eAudioCompressionType = CT_OPUS64;
        }
        else
        {
            // since we use double size frame size for OPUS, we have to adjust the frame size factor
            iSndCrdFrameSizeFactor /= 2;
            eAudioCompressionType   = CT_OPUS;

        }
    }

    // inits for audio coding
    if ( eAudioCompressionType == CT_OPUS )
    {
        iOPUSFrameSizeSamples = DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES;

        if ( eAudioChannelConf == CC_MONO )
        {
            CurOpusEncoder    = OpusEncoderMono;
            CurOpusDecoder    = OpusDecoderMono;
            iNumAudioChannels = 1;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_LOW_QUALITY_DBLE_FRAMESIZE;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_NORMAL_QUALITY_DBLE_FRAMESIZE; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_HIGH_QUALITY_DBLE_FRAMESIZE;   break;
            }
        }
        else
        {
            CurOpusEncoder    = OpusEncoderStereo;
            CurOpusDecoder    = OpusDecoderStereo;
            iNumAudioChannels = 2;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_LOW_QUALITY_DBLE_FRAMESIZE;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY_DBLE_FRAMESIZE; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_HIGH_QUALITY_DBLE_FRAMESIZE;   break;
            }
        }
    }
    else /* CT_OPUS64 */
    {
        iOPUSFrameSizeSamples = SYSTEM_FRAME_SIZE_SAMPLES;

        if ( eAudioChannelConf == CC_MONO )
        {
            CurOpusEncoder    = Opus64EncoderMono;
            CurOpusDecoder    = Opus64DecoderMono;
            iNumAudioChannels = 1;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_LOW_QUALITY;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_NORMAL_QUALITY; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_MONO_HIGH_QUALITY;   break;
            }
        }
        else
        {
            CurOpusEncoder    = Opus64EncoderStereo;
            CurOpusDecoder    = Opus64DecoderStereo;
            iNumAudioChannels = 2;

            switch ( eAudioQuality )
            {
            case AQ_LOW:    iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_LOW_QUALITY;    break;
            case AQ_NORMAL: iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_NORMAL_QUALITY; break;
            case AQ_HIGH:   iCeltNumCodedBytes = OPUS_NUM_BYTES_STEREO_HIGH_QUALITY;   break;
            }
        }
    }

    // calculate stereo (two channels) buffer size
    iStereoBlockSizeSam = 2 * iMonoBlockSizeSam;

    vecCeltData.Init ( iCeltNumCodedBytes );
    vecZeros.Init ( iStereoBlockSizeSam, 0 );
    vecsStereoSndCrdMuteStream.Init ( iStereoBlockSizeSam );

    fMuteOutStreamGain = 1.0f;

    opus_custom_encoder_ctl ( CurOpusEncoder,
                              OPUS_SET_BITRATE (
                                  CalcBitRateBitsPerSecFromCodedBytes (
                                      iCeltNumCodedBytes, iOPUSFrameSizeSamples ) ) );

    // inits for network and channel
    vecbyNetwData.Init ( iCeltNumCodedBytes );

    // set the channel network properties
    Channel.SetAudioStreamProperties ( eAudioCompressionType,
                                       iCeltNumCodedBytes,
                                       iSndCrdFrameSizeFactor,
                                       iNumAudioChannels );

    // set channel network properties for p2p channel
    // set the channel network properties
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        p2pChannels[i].SetAudioStreamProperties ( eAudioCompressionType,
                                                  iCeltNumCodedBytes,
                                                  iSndCrdFrameSizeFactor,
                                                  iNumAudioChannels );
    }

    // init reverberation
    AudioReverb.Init ( eAudioChannelConf,
                       iStereoBlockSizeSam,
                       SYSTEM_SAMPLE_RATE_HZ );

    // init the sound card conversion buffers
    if ( bSndCrdConversionBufferRequired )
    {
        // inits for conversion buffer (the size of the conversion buffer must
        // be the sum of input/output sizes which is the worst case fill level)
        const int iSndCardStereoBlockSizeSamConvBuff = 2 * iSndCardMonoBlockSizeSamConvBuff;
        const int iConBufSize                        = iStereoBlockSizeSam + iSndCardStereoBlockSizeSamConvBuff;

        SndCrdConversionBufferIn.Init  ( iConBufSize );
        SndCrdConversionBufferOut.Init ( iConBufSize );
        vecDataConvBuf.Init            ( iStereoBlockSizeSam );

        // the output conversion buffer must be filled with the inner
        // block size for initialization (this is the latency which is
        // introduced by the conversion buffer) to avoid buffer underruns
        SndCrdConversionBufferOut.Put ( vecZeros, iStereoBlockSizeSam );
    }

    // reset initialization phase flag and mute flag
    bIsInitializationPhase = true;

    //p2p initialisation
    vecChanIDsCurConChan.Init          ( iMaxNumChannels );
    vecAudioComprType.Init             ( iMaxNumChannels );
    vecNumAudioChannels. Init          ( iMaxNumChannels );
    vecUseDoubleSysFraSizeConvBuf.Init ( iMaxNumChannels );
    vecNumFrameSizeConvBlocks.Init     ( iMaxNumChannels );
    vecvecbyCodedData.Init             ( iMaxNumChannels );
    p2pvecvecsData.Init                ( iMaxNumChannels );
    p2pvecGains.Init                   ( iMaxNumChannels );
    vecLoopAudio.Init                  ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES );
}

void CClient::AudioCallback ( CVector<int16_t>& psData, void* arg )
{
    // get the pointer to the object
    CClient* pMyClientObj = static_cast<CClient*> ( arg );

    // process audio data
    pMyClientObj->ProcessSndCrdAudioData ( psData );

/*
// TEST do a soundcard jitter measurement
static CTimingMeas JitterMeas ( 1000, "test2.dat" );
JitterMeas.Measure();
*/
}

void CClient::ProcessSndCrdAudioData ( CVector<int16_t>& vecsStereoSndCrd )
{
    // check if a conversion buffer is required or not
    if ( bSndCrdConversionBufferRequired )
    {
        // add new sound card block in conversion buffer
        SndCrdConversionBufferIn.Put ( vecsStereoSndCrd, vecsStereoSndCrd.Size() );

        // process all available blocks of data
        while ( SndCrdConversionBufferIn.GetAvailData() >= iStereoBlockSizeSam )
        {
            // get one block of data for processing
            SndCrdConversionBufferIn.Get ( vecDataConvBuf, iStereoBlockSizeSam );

            // process audio data
            ProcessAudioDataIntern ( vecDataConvBuf );

            SndCrdConversionBufferOut.Put ( vecDataConvBuf, iStereoBlockSizeSam );
        }

        // get processed sound card block out of the conversion buffer
        SndCrdConversionBufferOut.Get ( vecsStereoSndCrd, vecsStereoSndCrd.Size() );
    }
    else
    {
        // regular case: no conversion buffer required
        // process audio data
        ProcessAudioDataIntern ( vecsStereoSndCrd );
    }
}

void CClient::ProcessAudioDataIntern ( CVector<int16_t>& vecsStereoSndCrd )
{
    int            i, j, iUnused;
    unsigned char* pCurCodedData;


    // Transmit signal ---------------------------------------------------------
    // update stereo signal level meter (not needed in headless mode)
#ifndef HEADLESS
    SignalLevelMeter.Update ( vecsStereoSndCrd,
                              iMonoBlockSizeSam,
                              true );
#endif

    // add reverberation effect if activated
    if ( iReverbLevel != 0 )
    {
        AudioReverb.Process ( vecsStereoSndCrd,
                              bReverbOnLeftChan,
                              static_cast<float> ( iReverbLevel ) / AUD_REVERB_MAX / 4 );
    }

    // apply pan (audio fader) and mix mono signals
    if ( !( ( iAudioInFader == AUD_FADER_IN_MIDDLE ) && ( eAudioChannelConf == CC_STEREO ) ) )
    {
        // calculate pan gain in the range 0 to 1, where 0.5 is the middle position
        const float fPan = static_cast<float> ( iAudioInFader ) / AUD_FADER_IN_MAX;

        if ( eAudioChannelConf == CC_STEREO )
        {
            // for stereo only apply pan attenuation on one channel (same as pan in the server)
            const float fGainL = MathUtils::GetLeftPan  ( fPan, false );
            const float fGainR = MathUtils::GetRightPan ( fPan, false );

            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // note that the gain is always <= 1, therefore a simple cast is
                // ok since we never can get an overload
                vecsStereoSndCrd[j + 1] = static_cast<int16_t> ( fGainR * vecsStereoSndCrd[j + 1] );
                vecsStereoSndCrd[j]     = static_cast<int16_t> ( fGainL * vecsStereoSndCrd[j] );
            }
        }
        else
        {
            // for mono implement a cross-fade between channels and mix them, for
            // mono-in/stereo-out use no attenuation in pan center
            const float fGainL = MathUtils::GetLeftPan  ( fPan, eAudioChannelConf != CC_MONO_IN_STEREO_OUT );
            const float fGainR = MathUtils::GetRightPan ( fPan, eAudioChannelConf != CC_MONO_IN_STEREO_OUT );

            for ( i = 0, j = 0; i < iMonoBlockSizeSam; i++, j += 2 )
            {
                // note that we need the Float2Short for stereo pan mode
                vecsStereoSndCrd[i] = Float2Short (
                    fGainL * vecsStereoSndCrd[j] + fGainR * vecsStereoSndCrd[j + 1] );
            }
        }
    }

    // Support for mono-in/stereo-out mode: Per definition this mode works in
    // full stereo mode at the transmission level. The only thing which is done
    // is to mix both sound card inputs together and then put this signal on
    // both stereo channels to be transmitted to the server.
    if ( eAudioChannelConf == CC_MONO_IN_STEREO_OUT )
    {
        // copy mono data in stereo sound card buffer (note that since the input
        // and output is the same buffer, we have to start from the end not to
        // overwrite input values)
        for ( i = iMonoBlockSizeSam - 1, j = iStereoBlockSizeSam - 2; i >= 0; i--, j -= 2 )
        {
            vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] = vecsStereoSndCrd[i];
        }
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // OPUS encoding
        if ( CurOpusEncoder != nullptr )
        {
            if ( bMuteOutStream )
            {
                iUnused = opus_custom_encode ( CurOpusEncoder,
                                               &vecZeros[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                               iOPUSFrameSizeSamples,
                                               &vecCeltData[0],
                                               iCeltNumCodedBytes );
            }
            else
            {
                iUnused = opus_custom_encode ( CurOpusEncoder,
                                                &vecsStereoSndCrd[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                                iOPUSFrameSizeSamples,
                                                &vecCeltData[0],
                                                   iCeltNumCodedBytes );
            }
        }

        if ( p2pEnabled )
        {
            // send coded audio to all other clients
            for ( int i = 0; i<p2pNumClientIps; i++ )
            {
                if ( p2pChannels[i].IsEnabled() )
                {
                    p2pChannels[i].PrepAndSendPacket ( &Socket,
                                                   vecCeltData,
                                                   iCeltNumCodedBytes );
                }
            }
        }

        // send coded audio through the network to server
        Channel.PrepAndSendPacket ( &Socket,
                                    vecCeltData,
                                    iCeltNumCodedBytes );
    }


    // Receive signal from SERVER ----------------------------------------------------------
    // in case of mute stream, store local data
    if ( bMuteOutStream || p2pEnabled )
    {
        vecsStereoSndCrdMuteStream = vecsStereoSndCrd;
    }

    for ( i = 0; i < iSndCrdFrameSizeFactor; i++ )
    {
        // receive a new block
        const bool bReceiveDataOk =
            ( Channel.GetData ( vecbyNetwData, iCeltNumCodedBytes ) == GS_BUFFER_OK );

        // get pointer to coded data and manage the flags
        if ( bReceiveDataOk )
        {
            pCurCodedData = &vecbyNetwData[0];

            // on any valid received packet, we clear the initialization phase flag
            bIsInitializationPhase = false;
        }
        else
        {
            // for lost packets use null pointer as coded input data
            pCurCodedData = nullptr;

            // invalidate the buffer OK status flag
            bJitterBufferOK = false;
        }

        // OPUS decoding
        if ( CurOpusDecoder != nullptr )
        {
            iUnused = opus_custom_decode ( CurOpusDecoder,
                                           pCurCodedData,
                                           iCeltNumCodedBytes,
                                           &vecsStereoSndCrd[i * iNumAudioChannels * iOPUSFrameSizeSamples],
                                           iOPUSFrameSizeSamples );
        }
    }

    // Receive signal from CLIENTS (p2p) ---------------------------------------------------------- ----------------------------------------------------------
    int  iNumClients               = 0; // init connected client counter

    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( p2pChannels[i].IsConnected() )
        {
            // add ID and increment counter (note that the vector length is
            // according to the worst case scenario, if the number of
            // connected clients is less, only a subset of elements of this
            // vector are actually used and the others are dummy elements)
            vecChanIDsCurConChan[iNumClients] = i;
            iNumClients++;
        }
    }
    // process connected channels
    for ( int i = 0; i < iNumClients; i++ )
    {
        OpusCustomDecoder* p2pCurOpusDecoder;//CurOpusDecoder;   -> already defined
        //unsigned char*     pCurCodedData;     -> already defined
        bool                bUseDoubleSystemFrameSize = true;

        // get actual ID of current channel
        const int iCurChanID = vecChanIDsCurConChan[i];

        // allocate worst case memory for the coded data
        vecvecbyCodedData[i].Init ( MAX_SIZE_BYTES_NETW_BUF );

         // we always use stereo audio buffers (which is the worst case)
        p2pvecvecsData[i].Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

        // get and store number of audio channels and compression type
        vecNumAudioChannels[i] = p2pChannels[iCurChanID].GetNumAudioChannels();             // from NetTranspPropsReceived
        vecAudioComprType[i]   = p2pChannels[iCurChanID].GetAudioCompressionType();         // from NetTranspPropsReceived
        p2pvecGains[i] = p2pChannels[iCurChanID].GetP2pGain();                             // get Gain

        // get info about required frame size conversion properties -> is always false because bUseDoubleSystemFrameSize is TRUE
        vecUseDoubleSysFraSizeConvBuf[i] = ( !bUseDoubleSystemFrameSize && ( vecAudioComprType[i] == CT_OPUS ) ); // bUseDoubleSystemFrameSize -> set in CServer -> usually true

        if ( vecAudioComprType[i] == CT_OPUS64 )
        {
            vecNumFrameSizeConvBlocks[i] = 2;
        }
        else
        {
            vecNumFrameSizeConvBlocks[i] = 1;
        }

        // update conversion buffer size (nothing will happen if the size stays the same)
        // if ( vecUseDoubleSysFraSizeConvBuf[i] )
        // {
        //     DoubleFrameSizeConvBufIn[iCurChanID].SetBufferSize  ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  * vecNumAudioChannels[i] );
        //     DoubleFrameSizeConvBufOut[iCurChanID].SetBufferSize ( DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES  * vecNumAudioChannels[i] );
        // }

        // select the opus decoder and raw audio frame length
        if ( vecAudioComprType[i] == CT_OPUS )
        {
            if ( vecNumAudioChannels[i] == 1 )
            {
                p2pCurOpusDecoder = p2pOpusDecoderMono[iCurChanID];
            }
            else
            {
                p2pCurOpusDecoder = p2pOpusDecoderStereo[iCurChanID];
            }
        }
        else if ( vecAudioComprType[i] == CT_OPUS64 )
        {
            if ( vecNumAudioChannels[i] == 1 )
            {
                p2pCurOpusDecoder = p2pOpus64DecoderMono[iCurChanID];
            }
            else
            {
                p2pCurOpusDecoder = p2pOpus64DecoderStereo[iCurChanID];
            }
        }
        else
        {
            p2pCurOpusDecoder = nullptr;
        }

        // If the server frame size is smaller than the received OPUS frame size, we need a conversion
        // buffer which stores the large buffer.
        // Note that we have a shortcut here. If the conversion buffer is not needed, the boolean flag
        // is false and the Get() function is not called at all. Therefore if the buffer is not needed
        // we do not spend any time in the function but go directly inside the if condition.
        if ( ( vecUseDoubleSysFraSizeConvBuf[i] == 0 ) ||
                !DoubleFrameSizeConvBufIn[iCurChanID].Get ( p2pvecvecsData[i], SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] ) )
        {
            // get current number of OPUS coded bytes
            const int iCeltNumCodedBytes = p2pChannels[iCurChanID].GetCeltNumCodedBytes();

            for ( int iB = 0; iB < vecNumFrameSizeConvBlocks[i]; iB++ )
            {
                // get data
                const EGetDataStat eGetStat = p2pChannels[iCurChanID].GetData ( vecvecbyCodedData[i], iCeltNumCodedBytes );

                // if channel was just disconnected, set flag that connected
                // client list is sent to all other clients
                // and emit the client disconnected signal
                if ( eGetStat == GS_CHAN_NOW_DISCONNECTED )
                {
                    // if ( JamController.GetRecordingEnabled() )
                    // {
                    //     emit ClientDisconnected ( iCurChanID ); // TODO do this outside the mutex lock?
                    // }
                    qDebug() << "Timeout on p2pChannels[iCurChanID].GetChannelID()" << iCurChanID << p2pChannels[iCurChanID].GetChannelID();
                    emit P2PChStateChange( p2pChannels[iCurChanID].GetChannelID(), false );

                    //bChannelIsNowDisconnected = true; --> NOT DEFINED YET
                }

                // get pointer to coded data
                if ( eGetStat == GS_BUFFER_OK )
                {
                    pCurCodedData = &vecvecbyCodedData[i][0];
                }
                else
                {
                    // for lost packets use null pointer as coded input data
                    pCurCodedData = nullptr;
                }

                // OPUS decode received data stream
                if ( p2pCurOpusDecoder != nullptr )
                {
                    iUnused = opus_custom_decode ( p2pCurOpusDecoder,
                                                    pCurCodedData,
                                                    iCeltNumCodedBytes,
                                                    &p2pvecvecsData[i][iB * SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i]],
                                                    iOPUSFrameSizeSamples );
                }
            }

            //this is NOT executed with my standard settings
            // a new large frame is ready, if the conversion buffer is required, put it in the buffer
            // and read out the small frame size immediately for further processing
            if ( vecUseDoubleSysFraSizeConvBuf[i] != 0 )
            {
                DoubleFrameSizeConvBufIn[i].Init  ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ ); //-> new
                DoubleFrameSizeConvBufIn[iCurChanID].PutAll ( p2pvecvecsData[i] );
                DoubleFrameSizeConvBufIn[iCurChanID].Get ( p2pvecvecsData[i], SYSTEM_FRAME_SIZE_SAMPLES * vecNumAudioChannels[i] );
            }
        }
    }
    //---------------------------------------------------------- (p2p) END

    //----------------------------------------------------------
    // for muted stream or p2penabled we have to add our local data here
    if ( bMuteOutStream || p2pEnabled  )
    {
        for ( i = 0; i < iStereoBlockSizeSam; i++ )
        {
            vecsStereoSndCrd[i] = Float2Short (
                vecsStereoSndCrd[i] + vecsStereoSndCrdMuteStream[i] * fMuteOutStreamGain );
        }
    }

     //----------------------------------------------- (p2p)
    // mix audio from server & p2p Clients
    CVector<double>  vecdIntermProcBuf; // use reference for faster access
    vecdIntermProcBuf.Init    (  2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */ );

    CVector<int16_t> vecsSendData; // use reference for faster access
    vecsSendData.Init ( 2 /* stereo */ * DOUBLE_SYSTEM_FRAME_SIZE_SAMPLES /* worst case buffer size */  );

    int k;

    // init intermediate processing vector with zeros since we mix all channels on that vector
    vecdIntermProcBuf.Reset ( 0 );


    if ( eAudioChannelConf == CC_MONO )
    {
        // Mono target channel -------------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // this client runs mono
            const CVector<int16_t>& vecsData = p2pvecvecsData[j];
            const double            dGain    = p2pvecGains[j];

            if ( dGain == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < iOPUSFrameSizeSamples; i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i];
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iOPUSFrameSizeSamples; i++, k += 2 )
                    {
                        vecdIntermProcBuf[i] +=
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2;
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono
                    for ( i = 0; i < iOPUSFrameSizeSamples; i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i] * dGain;
                    }
                }
                else
                {
                    // stereo: apply stereo-to-mono attenuation
                    for ( i = 0, k = 0; i < iOPUSFrameSizeSamples; i++, k += 2 )
                    {
                        vecdIntermProcBuf[i] +=  dGain *
                            ( static_cast<double> ( vecsData[k] ) + vecsData[k + 1] ) / 2;
                    }
                }
            }
        }
        // convert from double to short with clipping
        for ( i = 0; i < iOPUSFrameSizeSamples; i++ )
        {
            vecsSendData[i] = Float2Short ( vecdIntermProcBuf[i] );
        }
    }
    else
    {
        // Stereo target channel -----------------------------------------------
        for ( j = 0; j < iNumClients; j++ )
        {
            // get a reference to the audio data and gain/pan of the current client
            const CVector<int16_t>& vecsData = p2pvecvecsData[j];
            const double            dGain    = p2pvecGains[j];

            if ( dGain == static_cast<double> ( 1.0 ) )
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < iOPUSFrameSizeSamples; i++, k += 2 )
                    {
                        // left/right channel
                        vecdIntermProcBuf[k]     += vecsData[i];
                        vecdIntermProcBuf[k + 1] += vecsData[i];
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iOPUSFrameSizeSamples ); i++ )
                    {
                        vecdIntermProcBuf[i] += vecsData[i];
                    }
                }
            }
            else
            {
                if ( vecNumAudioChannels[j] == 1 )
                {
                    // mono: copy same mono data in both out stereo audio channels
                    for ( i = 0, k = 0; i < iOPUSFrameSizeSamples; i++, k += 2 )
                    {
                        // left/right channel
                        vecdIntermProcBuf[k]     += vecsData[i] * dGain;
                        vecdIntermProcBuf[k + 1] += vecsData[i] * dGain;
                    }
                }
                else
                {
                    // stereo
                    for ( i = 0; i < ( 2 * iOPUSFrameSizeSamples ); i++ )
                    {
                        // left/right channel
                        vecdIntermProcBuf[i]     += vecsData[i] *     dGain;
                        vecdIntermProcBuf[i + 1] += vecsData[i + 1] * dGain;
                    }
                }
            }

        }
        // convert from double to short with clipping
        for ( i = 0; i < ( 2 * iOPUSFrameSizeSamples ); i++ )
        {
            vecsSendData[i] = Float2Short ( vecdIntermProcBuf[i] );
        }
    }

    // add p2p sound (vecsSendData) to server audio (vecsSendData)
    for ( i = 0; i < iOPUSFrameSizeSamples; i++ )
    {
        vecsStereoSndCrd[i] += vecsSendData[i];
    }

    for ( i = 0; i < iOPUSFrameSizeSamples; i++ )
    {
        vecLoopAudio[i] = vecsSendData[i];
    }
    //----------------------------------------------- (p2p) END

    // check if channel is connected and if we do not have the initialization phase
    if ( Channel.IsConnected() && ( !bIsInitializationPhase ) )
    {
        if ( eAudioChannelConf == CC_MONO )
        {
            // copy mono data in stereo sound card buffer (note that since the input
            // and output is the same buffer, we have to start from the end not to
            // overwrite input values)
            for ( i = iMonoBlockSizeSam - 1, j = iStereoBlockSizeSam - 2; i >= 0; i--, j -= 2 )
            {
                vecsStereoSndCrd[j] = vecsStereoSndCrd[j + 1] = vecsStereoSndCrd[i];
            }
        }
    }
    else
    {
        // if not connected, clear data
        vecsStereoSndCrd.Reset ( 0 );
    }

    // update socket buffer size
    Channel.UpdateSocketBufferSize();

    for ( int i = 0; i < p2pNumClientIps; i++ )
    {
        // update socket buffer size
        p2pChannels[i].UpdateSocketBufferSize();
    }

    // export the audio data for recording purpose
    if ( bRecorderEnabled && !bStopRecorder)
    {
        emit AudioFrame ( 0,
                          "recording",
                          CHostAddress(),
                          2,
                          vecsStereoSndCrd );
    }
    else if ( bStopRecorder )
    {
        JamController.SetEnableRecording ( false, true );
        bStopRecorder = false;
    }

    Q_UNUSED ( iUnused )
}

int CClient::EstimatedOverallDelay ( const int iPingTimeMs )
{
    const float fSystemBlockDurationMs = static_cast<float> ( iOPUSFrameSizeSamples ) /
        SYSTEM_SAMPLE_RATE_HZ * 1000;

    // If the jitter buffers are set effectively, i.e. they are exactly the
    // size of the network jitter, then the delay of the buffer is the buffer
    // length. Since that is usually not the case but the buffers are usually
    // a bit larger than necessary, we introduce some factor for compensation.
    // Consider the jitter buffer on the client and on the server side, too.
    const float fTotalJitterBufferDelayMs = fSystemBlockDurationMs *
        ( GetSockBufNumFrames() + GetServerSockBufNumFrames() ) * 0.7f;

    // consider delay introduced by the sound card conversion buffer by using
    // "GetSndCrdConvBufAdditionalDelayMonoBlSize()"
    float fTotalSoundCardDelayMs = GetSndCrdConvBufAdditionalDelayMonoBlSize() *
        1000.0f / SYSTEM_SAMPLE_RATE_HZ;

    // try to get the actual input/output sound card delay from the audio
    // interface, per definition it is not available if a 0 is returned
    const float fSoundCardInputOutputLatencyMs = Sound.GetInOutLatencyMs();

    if ( fSoundCardInputOutputLatencyMs == 0.0f )
    {
        // use an alternative approach for estimating the sound card delay:
        //
        // we assume that we have two period sizes for the input and one for the
        // output, therefore we have "3 *" instead of "2 *" (for input and output)
        // the actual sound card buffer size
        // "GetSndCrdConvBufAdditionalDelayMonoBlSize"
        fTotalSoundCardDelayMs +=
            ( 3 * GetSndCrdActualMonoBlSize() ) *
            1000.0f / SYSTEM_SAMPLE_RATE_HZ;
    }
    else
    {
        // add the actual sound card latency in ms
        fTotalSoundCardDelayMs += fSoundCardInputOutputLatencyMs;
    }

    // network packets are of the same size as the audio packets per definition
    // if no sound card conversion buffer is used
    const float fDelayToFillNetworkPacketsMs =
        GetSystemMonoBlSize() * 1000.0f / SYSTEM_SAMPLE_RATE_HZ;

    // OPUS additional delay at small frame sizes is half a frame size
    const float fAdditionalAudioCodecDelayMs = fSystemBlockDurationMs / 2;

    const float fTotalBufferDelayMs =
        fDelayToFillNetworkPacketsMs +
        fTotalJitterBufferDelayMs +
        fTotalSoundCardDelayMs +
        fAdditionalAudioCodecDelayMs;

    return MathUtils::round ( fTotalBufferDelayMs + iPingTimeMs );
}

void CClient::OnTimerClientReReqServList()
{
    // if the server list is not yet received, retransmit the request
    if ( waitingForIp )
    {
        // convert central server address string to chostaddress
        CHostAddress HostAddressCent;
        if( NetworkUtil().ParseNetworkAddress ( strCentralServerAddressClient, HostAddressCent ))
        {
            // request list from server
            CreateCLReqServerListMes ( HostAddressCent, strStartupAddress );
            return;
        }
        qInfo() << "central server address not possible to parse";
        return;
    }
    else
    {
        TimerClientReReqServList.stop();
    }
}

void CClient::P2pStateChanged( const bool enable )
{
    qInfo() << "p2p state changed";

    // set p2pEnabled for channels
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        p2pChannels[i].SetP2pEnabled( enable );
    }
}

void CClient::OnConClientListMesReceived ( CVector<CChannelInfo> vecChanInfo )
{
    CHostAddress HostAddr_temp;

    p2pNumClientIps = vecChanInfo.Size();

    int p2pChannelIndex = 0;

    for ( int i = 0; ( i < MAX_NUM_CHANNELS ) && ( i < vecChanInfo.Size() )  ; i++ )
    {
        // find out if this entry shows own client info

        // skip if channel id is own id
        if( vecChanInfo[i].iChanID == Channel.GetChannelID() )
        {
            p2pNumClientIps--;
            continue;
        }

        if( vecChanInfo[i].PIpAddr == 0)
        {
            //p2pNumClientIps--;
            continue;
        }

        // Check if public ip is the same as own public ip
        if ( Channel.PInetAddr.InetAddr.toIPv4Address() == vecChanInfo[i].PIpAddr )
        {
            // both devices are in the same LAN -> set local address
            HostAddr_temp.InetAddr.setAddress( vecChanInfo[i].LIpAddr );

            // -> set local port
            HostAddr_temp.iPort = vecChanInfo[i].LiPort;

        }
        else
        {
            // both devices are in different LANs -> set public address
            HostAddr_temp.InetAddr.setAddress( vecChanInfo[i].PIpAddr );

            // -> set public port
            HostAddr_temp.iPort = vecChanInfo[i].PiPort;


        }

        // save ipv4 chostaddr to channel
        p2pChannels[p2pChannelIndex].SetAddress(HostAddr_temp);

        // save local and global address as key to lookup id
        CHostAddress LocalIpAddress = CHostAddress(vecChanInfo[i].LIpAddr, vecChanInfo[i].LiPort);
        CHostAddress PublicIpAddress = CHostAddress(vecChanInfo[i].PIpAddr, vecChanInfo[i].PiPort);
        p2pChannels[p2pChannelIndex].SetKey ( LocalIpAddress,  PublicIpAddress );

        // set channel id of channel
        p2pChannels[p2pChannelIndex].SetChannelID(vecChanInfo[i].iChanID);

        // enable channel (so channel could also receive)
        p2pChannels[p2pChannelIndex].SetEnable ( true );
        qInfo() << "DEBUG p2pChannels " << p2pChannelIndex << " " << p2pChannels[p2pChannelIndex].GetAddress().toString();
        p2pChannelIndex++;
    }

    for ( ; p2pChannelIndex < MAX_NUM_CHANNELS; p2pChannelIndex++ )
    {
        // disable all other channels
        p2pChannels[p2pChannelIndex].SetEnable ( false );
    }

    TimerPingP2pClients.start( 1000 );
}

bool CClient::PutAudioData ( const CVector<uint8_t>& vecbyRecBuf,
                             const int               iNumBytesRead,
                             const CHostAddress&     HostAdr,
                             int&                    iCurChanID )
{
    //QMutexLocker locker ( &Mutex );
    bool bNewConnection = false; // init return value
    bool bChanOK        = true;  // init with ok, might be overwritten

    // Get channel ID ------------------------------------------------------
    // check address
    iCurChanID = FindP2PChannel ( HostAdr ); // Address of P2P Client

    if ( iCurChanID == INVALID_CHANNEL_ID )
    {
        qInfo() << "DEBUG unknown host tried to establish p2p connection";
        return false;
    }

    // Put received audio data in jitter buffer ----------------------------
    if ( bChanOK )
    {
        // put packet in socket buffer
        if ( p2pChannels[iCurChanID].PutAudioData ( vecbyRecBuf,
                                                    iNumBytesRead,
                                                    HostAdr ) == PS_NEW_CONNECTION )
        {
            // in case we have a new connection return this information
            bNewConnection = true;
        }
    }

    // return the state if a new connection was happening
    return bNewConnection;
}

int CClient::GetFreeChan()
{
    // look for a free channel
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        if ( !p2pChannels[i].IsConnected() )
        {
            return i;
        }
    }

    // no free channel found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CClient::FindChannel ( const CHostAddress& CheckAddr )
{
    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        // the "GetAddress" gives a valid address and returns true if the
        // channel is connected
        if ( p2pChannels[i].GetAddress ( InetAddr ) )
        {
            // IP found, return channel number
            if ( InetAddr == CheckAddr )
            {
                return i;
            }
        }
    }

    // IP not found, return invalid ID
    return INVALID_CHANNEL_ID;
}

int CClient::FindP2PChannel ( const CHostAddress& CheckAddr )
{
    // find the list index of the channel based on the
    // local or global ip address

    CHostAddress InetAddr;

    // check for all possible channels if IP is already in use
    for ( int i = 0; i < iMaxNumChannels; i++ )
    {
        // the "GetAddress" gives a valid address and returns true if the
        // channel is connected

        if ( p2pChannels[i].MatchesAddresses ( CheckAddr ) ) {
            // IP found, return channel number
                return i;
        }
    }

    // IP not found, return invalid ID
    return INVALID_CHANNEL_ID;
}

// central server sent clients public ip
void CClient::OnCLPublicIpRec ( CHostAddress PInetAddr )
{
    // get local port
    Channel.LInetAddr = CHostAddress( NetworkUtil::GetLocalAddress().InetAddr, iPort );
    Channel.PInetAddr = PInetAddr;

    qInfo() << "My public IP: " << PInetAddr.toString();
    qInfo() << "My local IP: " << Channel.LInetAddr.toString();
}

void CClient::OnNewP2pConnection ( int iChID, CHostAddress )
{
    qInfo() << "DEBUG OnNewP2pConnection";

    // inform the client about its own ID at the server (note that this
    // must be the first message to be sent for a new connection)
    //p2pChannels[iChID].CreateClientIDMes ( iChID );

    // on a new connection we query the network transport properties for the
    // audio packets (to use the correct network block size and audio
    // compression properties, etc.)
    p2pChannels[iChID].CreateReqNetwTranspPropsMes();

    // this is a new connection, query the jitter buffer size we shall use
    // for this client (note that at the same time on a new connection the
    // client sends the jitter buffer size by default but maybe we have
    // reached a state where this did not happen because of network trouble,
    // client or server thinks that the connection was still active, etc.)
    p2pChannels[iChID].CreateReqJitBufMes();

    // A new client connected to the server, the channel list
    // at all clients have to be updated. This is done by sending
    // a channel name request to the client which causes a channel
    // name message to be transmitted to the server. If the server
    // receives this message, the channel list will be automatically
    // updated (implicitly).
    //
    // Usually it is not required to send the channel list to the
    // client currently connecting since it automatically requests
    // the channel list on a new connection (as a result, he will
    // usually get the list twice which has no impact on functionality
    // but will only increase the network load a tiny little bit). But
    // in case the client thinks he is still connected but the server
    // was restartet, it is important that we send the channel list
    // at this place.
    p2pChannels[iChID].CreateReqChanInfoMes();

    emit P2PChStateChange( p2pChannels[iChID].GetChannelID(), true ); // P2P on
}

int CClient::CreateLevelForThisChan( const int iChanID,
                                          const CVector<int16_t> vecvecsData,
                                          const int              numAudioChannels )
{
    // update and get signal level for meter in dB for each channel
    const double dCurSigLevelForMeterdB =
                p2pChannels[iChanID].UpdateAndGetLevelForMeterdB ( vecvecsData,
                                                             iOPUSFrameSizeSamples,
                                                             numAudioChannels > 1 );

    // map value to integer for transmission via the protocol (4 bit available)
    return static_cast<int> ( ceil ( dCurSigLevelForMeterdB ) );
}

int CClient::CreateLevelForOwnChan( const CVector<int16_t> vecvecsData,
                                          const int        numAudioChannels )
{
    // update and get signal level for meter in dB for each channel
    const double dCurSigLevelForMeterdB =
                Channel.UpdateAndGetLevelForMeterdB ( vecvecsData,
                                                      iOPUSFrameSizeSamples,
                                                      numAudioChannels > 1 );

    // map value to integer for transmission via the protocol (4 bit available)
    return static_cast<int> ( ceil ( dCurSigLevelForMeterdB ) );
}

void CClient::OnServerRegisteredSuccessfully( QString serverName )
{
    // Set new server name
    if ( strStartupAddress != serverName)
    {
        strStartupAddress = serverName;

        // show info about server name change in clientdlg -> emit is done a little later in CClient::Start()
        serverNameChanged = true;
    }

    // Start waiting for server list from central server
    waitingForIp = true;
    TimerClientReReqServList.start( 1000 ); //ms
}
