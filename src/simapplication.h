/****************************************************************************
**
** This file is part of the Qt Extended Opensource Package.
**
** Copyright (C) 2009 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** version 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#ifndef SIMAPPLICATION_H
#define SIMAPPLICATION_H

#include "phonesim.h"
#include "qsimcommand.h"
#include "qsimterminalresponse.h"
#include "qsimenvelope.h"
#include "qsimcontrolevent.h"

class SimApplicationPrivate;

class SimApplication : public QObject
{
    Q_OBJECT
public:
    SimApplication( SimRules *rules, QObject *parent = nullptr );
    ~SimApplication() override;

    virtual bool envelope( const QSimEnvelope& env );
    virtual bool response( const QSimTerminalResponse& resp );
    virtual QByteArray fetch( bool clear = false );

    virtual const QString getName() = 0;

public slots:
    virtual void start();
    virtual void abort();

protected slots:
    void command( const QSimCommand& cmd,
                  QObject *target, const char *slot,
                  QSimCommand::ToPduOptions options
                        = QSimCommand::NoPduOptions );
    void modemHandledCommand( const QSimCommand& cmd, int timeout );
    void controlEvent( const QSimControlEvent& event );

    virtual void mainMenu() = 0;
    virtual void mainMenuSelection( int id );
    virtual void mainMenuHelpRequest( int id );
    virtual void endSession();
    virtual void reinitSim();

private:
    SimApplicationPrivate *d;
};

class DemoSimApplication : public SimApplication
{
    Q_OBJECT
public:
    DemoSimApplication( SimRules *rules, QObject *parent = nullptr );
    ~DemoSimApplication() override;

    const QString getName() override;

    bool envelope( const QSimEnvelope& env ) override;

protected slots:
    void mainMenu() override;
    void mainMenuSelection( int id ) override;
    void sendSportsMenu();
    void sportsMenu( const QSimTerminalResponse& resp );
    void sendCallsMenu();
    void callsMenu( const QSimTerminalResponse& resp );
    void startSticksGame();
    void sticksGameShow();
    void sticksGameLoop( const QSimTerminalResponse& resp );
    void getInputLoop( const QSimTerminalResponse& resp );
    void sticksGamePlayAgain( const QSimTerminalResponse& resp );
    void sendToneMenu();
    void toneMenu( const QSimTerminalResponse& resp );
    void sendIconMenu();
    void iconMenu( const QSimTerminalResponse& resp );
    void sendIconSEMenu();
    void iconSEMenu( const QSimTerminalResponse& resp );
    void sendDisplayText();
    void displayTextResponse( const QSimTerminalResponse& resp );
    void sendBrowserMenu();
    void browserMenu( const QSimTerminalResponse& resp );
    void sendDTMF();
    void sendSendSSMenu();
    void sendSSMenu( const QSimTerminalResponse& resp );
    void sendCBMenu();
    void CBMenu( const QSimTerminalResponse& resp );
    void sendCFMenu();
    void CFMenu( const QSimTerminalResponse& resp );
    void sendCWMenu();
    void CWMenu( const QSimTerminalResponse& resp );
    void sendCLIPMenu();
    void CLIPMenu( const QSimTerminalResponse& resp );
    void sendCLIRMenu();
    void CLIRMenu( const QSimTerminalResponse& resp );
    void sendCoLPMenu();
    void CoLPMenu( const QSimTerminalResponse& resp );
    void sendCoLRMenu();
    void CoLRMenu( const QSimTerminalResponse& resp );
    void sendCNAPMenu();
    void CNAPMenu( const QSimTerminalResponse& resp );
    void sendLanguageMenu();
    void languageMenu( const QSimTerminalResponse& resp );
    void sendUSSDMenu();
    void USSDMenu( const QSimTerminalResponse& resp );
    void sendSMSMenu();
    void smsMenuResp( const QSimTerminalResponse& resp );
    void smsSetDestResp( const QSimTerminalResponse& resp );
    void smsSetTextResp( const QSimTerminalResponse& resp );
    void sendPollingMenu();
    void pollingMenuResp( const QSimTerminalResponse& resp );
    void sendTimersMenu();
    void timersMenuResp( const QSimTerminalResponse& resp );
    void timersCmdResp( const QSimTerminalResponse& resp );
    void sendRefreshMenu();
    void refreshMenuResp( const QSimTerminalResponse& resp );
    void sendLocalInfoMenu();
    void localInfoMenu( const QSimTerminalResponse& resp );
    void sendBIPMenu();
    void BIPMenu( const QSimTerminalResponse& resp );
    void sendHandledMenu();
    void handledMenuResp( const QSimTerminalResponse& resp );

private:
    int sticksLeft;
    bool immediateResponse;
    QString smsDestNumber, smsText;
    QString timerStatus;
};

class ConformanceSimApplication : public SimApplication
{
    Q_OBJECT
public:
    ConformanceSimApplication( SimRules *rules, QObject *parent = nullptr );
    ~ConformanceSimApplication() override;

    const QString getName() override;

protected slots:
    void mainMenu() override;
    void mainMenuSelection( int id ) override;
    void sendDisplayTextMenu();
    void DisplayTextMenu( const QSimTerminalResponse& resp );
    void DisplayTextNormalMenu( const QSimTerminalResponse& resp );
    void sendDisplayTextNormalMenu();
    void sendDisplayTextIconMenu();
    void DisplayTextIconMenu( const QSimTerminalResponse& resp );
    void sendGetInkeyMenu();
    void GetInkeyMenu( const QSimTerminalResponse& resp );
    void GetInkeyNormalMenu( const QSimTerminalResponse& resp );
    void sendGetInkeyNormalMenu();
    void sendGetInkeyIconMenu();
    void GetInkeyIconMenu( const QSimTerminalResponse& resp );
    void sendHelpInfo( const QSimTerminalResponse& resp );
    void sendGetInputMenu();
    void GetInputMenu( const QSimTerminalResponse& resp );
    void GetInputNormalMenu( const QSimTerminalResponse& resp );
    void sendGetInputNormalMenu();
    void sendGetInputIconMenu();
    void GetInputIconMenu( const QSimTerminalResponse& resp );
    void sendSetupCallMenu();
    void SetupCallMenu( const QSimTerminalResponse& resp );
};

#endif
