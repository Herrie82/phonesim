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

#include "callmanager.h"
#include "qatutils.h"
#include "qsimcontrolevent.h"

CallManager::CallManager( QObject *parent )
    : QObject( parent )
{
    _holdWillFail = false;
    _activateWillFail = false;
    _joinWillFail = false;
    _deflectWillFail = false;
    _multipartyLimit = -1;
    numRings = 0;

    hangupTimer = new QTimer(this);
    hangupTimer->setSingleShot(true);
    connect( hangupTimer, &QTimer::timeout, this, &CallManager::hangupTimeout );

    ringTimer = new QTimer(this);
    ringTimer->setSingleShot(true);
    connect( ringTimer, &QTimer::timeout, this, &CallManager::sendNextRing );
}

CallManager::~CallManager()
{
}

bool CallManager::command( const QString& cmd )
{
    if ( cmd.startsWith( "ATD*" ) || cmd.startsWith( "ATD#" ) ) {

        // Supplementary service request - just say OK for now.
        emit send( "OK" );

    } else if ( cmd.startsWith( "ATD" ) && cmd.endsWith( ";" ) ) {

        // Voice call setup.
        QString number = cmd.mid(3, cmd.length() - 4);
        if ( number.endsWith( "g" ) || number.endsWith( "G" ) )
            number = number.left(number.length() - 1);  // Closed user group flag - skip.
        if ( number.endsWith( "i" ) || number.endsWith( "I" ) )
            number = number.left(number.length() - 1);  // Caller id suppress flag - skip.

        // Determine if the number is blocked by the fixed-dialing phone book.
        bool dialCheckOk = true;
        emit dialCheck( number, dialCheckOk );
        if ( !dialCheckOk ) {
            emit send( "ERROR" );
            return true;
        }

        // Stop if a dialing call is already in progress, or there are both
        // connected and held calls at the same time.
        if ( hasCall( CallState_Dialing ) || hasCall( CallState_Alerting ) ) {
            emit send( "ERROR" );
            return true;
        }
        if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
            emit send( "ERROR" );
            return true;
        }

        // For SAT test purpose, reject the dialing number 0123401234 if there
        // is a connected call.
        if ( hasCall( CallState_Active ) && number == "+0123401234" ) {
            send( "NO CARRIER" );
            return true;
         }

        // If there is a connected call, place it on hold.
        changeGroup( CallState_Active, CallState_Held );

        // Check for special dial-back numbers.
        if ( number == "199" ) {
            send( "NO CARRIER" );
            QTimer::singleShot( 5000, this, &CallManager::dialBack );
            return true;
        } else if ( number == "1993" ) {
            send( "NO CARRIER" );
            QTimer::singleShot( 30000, this, &CallManager::dialBack );
            return true;
        } else if ( number == "177" ) {
            send( "NO CARRIER" );
            QTimer::singleShot( 2000, this, &CallManager::dialBackWithHangup5 );
            return true;
        } else if ( number == "166" ) {
            send( "NO CARRIER" );
            QTimer::singleShot( 1000, this, &CallManager::dialBackWithHangup4 );
            return true;
        } else if ( number == "155" ) {
            send( "BUSY" );
            return true;
        }

        // Other special numbers
        if ( number == "144" )
            send( "+CCWV" );

        // Perform call control on certain numbers.
        if ( number == "12399" ) {
            QSimControlEvent event;
            event.setType( QSimControlEvent::Call );
            event.setResult( QSimControlEvent::Allowed );
            event.setText( "12399 allowed by call control" );
            emit controlEvent( event );
        } else if ( number == "12388" ) {
            QSimControlEvent event;
            event.setType( QSimControlEvent::Call );
            event.setResult( QSimControlEvent::AllowedWithModifications );
            event.setText( "12388 allowed, but modified to 12389" );
            number = "12389";
            emit controlEvent( event );
        } else if ( number == "12377" ) {
            QSimControlEvent event;
            event.setType( QSimControlEvent::Call );
            event.setResult( QSimControlEvent::NotAllowed );
            event.setText( "12377 disallowed by call control" );
            send( "NO CARRIER" );
            emit controlEvent( event );
            return true;
        }

        // Create a new call and add it to the list.
        CallInfo info;
        info.id = newId();
        info.state = CallState_Dialing;
        info.number = number;
        info.incoming = false;
        info.dialBack = false;
        callList += info;

        emit callStatesChanged( &callList );

        // Advertise the call state change and then return to command mode.
        sendState( info );
        send( "OK" );

        // Automatic accept of calls
        if ( number == "6789" ) {
            QTimer::singleShot( 1000, this, &CallManager::dialingToConnected );
        } else if ( number.startsWith( "05123" ) ) {
            QTimer::singleShot( 1000, this, &CallManager::dialingToConnected );
        } else if ( number.startsWith( "06123" ) ) {
            QTimer::singleShot( 1000, this, &CallManager::dialingToAlerting );
        }

    // Data call - phone number 696969
    } else if ( cmd.startsWith( "ATD" ) ) {
        // Data call setup.
        QString number = cmd.mid(3, cmd.length() - 4);
        if ( number.endsWith( "g" ) || number.endsWith( "G" ) )
            number = number.left(number.length() - 1);  // Closed user group flag - skip.
        if ( number.endsWith( "i" ) || number.endsWith( "I" ) )
            number = number.left(number.length() - 1);  // Caller id suppress flag - skip.

        if ( number == "696969" ) {
                // Create a new call and add it to the list.
                CallInfo info;
                info.id = newId();
                info.state = CallState_Dialing;
                info.number = number;
                info.incoming = false;
                info.dialBack = false;
                callList += info;

                emit callStatesChanged( &callList );

                // Advertise the call state change and then return to command mode.
                sendState( info );
                send( "CONNECT 19200" );
                } else {
                // If not a data line
                emit send( "NO CARRIER" );
                }

    } else if ( cmd == "AT+CLCC" ) {

        // List all calls that are presently active.
        for ( const CallInfo &info : qAsConst(callList) ) {
            int multiparty;
            if ( countForState(info.state) >= 2 )
                multiparty = 1;
            else
                multiparty = 0;
            QString line =
                "+CLCC: " + QString::number(info.id) + "," +
                QString::number((int)(info.incoming)) + "," +
                QString::number((int)(info.state)) + ",0," +
                QString::number(multiparty);
            if ( !info.number.isEmpty() ) {
                line += ",";
                line += QAtUtils::encodeNumber(info.number);
            }
            send( line );
        }
        send( "OK" );

    } else if ( cmd.startsWith( "ATH" ) || cmd == "AT+CHUP" ) {

        // Hang up all active and held calls in the system.
        hangupAll();
        send( "OK" );

    } else if ( cmd == "AT+CHLD=0" ) {

        // Reject incoming call, or release held calls.
        if ( chld0() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd == "AT+CHLD=1" ) {

        // Release all active calls and accept the held or waiting ones.
        if ( chld1() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd.startsWith( "AT+CHLD=1" ) ) {

        // Release a specific call.
        if ( chld1x( cmd.mid(9).toInt() ) )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd == "AT+CHLD=2" ) {

        // Place active calls on hold and accept the held or waiting call.
        if ( chld2() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd.startsWith( "AT+CHLD=2" ) ) {

        // Place all active calls on hold except for the specified call.
        if ( chld2x( cmd.mid(9).toInt() ) )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd == "AT+CHLD=3" ) {

        // Add a held call to the conversation.
        if ( chld3() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd == "AT+CHLD=4" ) {

        // Add a held call to the conversation, and then disconnect.
        if ( chld4() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd == "ATA" ) {

        // Accept the incoming call.
        if ( acceptCall() )
            send( "OK" );
        else
            send( "ERROR" );

    } else if ( cmd.startsWith( "AT+CTFR=" ) ) {

        // Deflect the incoming call to another number.
        int id = idForIncoming();
        if ( id >= 0 && !deflectWillFail() ) {
            hangupCall(id);
            hangupTimer->stop();
            ringTimer->stop();
            send( "OK" );
        } else {
            send( "ERROR" );
        }

    } else {
        // Command is not understood by the call manager.
        return false;
    }

    // If we get here, then the command was understood.
    return true;
}

void CallManager::emitRing(const CallInfo &info)
{
    QString str;

    // Annouce the incoming call using GSM 27.007 notifications.
    if ( info.state == CallState_Waiting ) {
        str = "+CCWA: " + QAtUtils::encodeNumber( info.number ) + ",1,";

        if (info.number.isNull())
            str += "2";
        else if (info.number.isEmpty())
            str += "1";
        else
            str += "0";
    } else {
        str = "+CRING: VOICE";

        if (info.number.isNull())
            ;
	else if (info.number.isEmpty())
            str += "\\n\\n+CLIP: " + QAtUtils::encodeNumber( info.number ) + ",,,,1";
        else
            str += "\\n\\n+CLIP: " + QAtUtils::encodeNumber( info.number ) + ",,,,0";

        if (info.calledNumber.isNull())
            ;
        else
            str += "\\n\\n+CDIP: " + QAtUtils::encodeNumber( info.calledNumber ) + ",,,";

        if (info.name.isNull())
            ;
	else if (info.name.isEmpty())
            str += "\\n\\n+CNAP: \"" + info.name + "\",1";
        else
            str += "\\n\\n+CNAP: \"" + info.name + "\",0";
    }

    emit unsolicited(str);
}

void CallManager::startIncomingCall( const QString& number,
                                     const QString& calledNumber,
                                     const QString& name, bool dialBack )
{
    // Bail out if there is already an incoming call.
    if ( idForIncoming() >= 0 ) {
        qWarning( "Incoming call already exists - connect create another" );
        return;
    }

    // Create a new call and add it to the list.
    CallInfo info;
    info.id = newId();
    if ( hasCall( CallState_Active ) || hasCall( CallState_Held ) ) {
        // If there are active calls, the call state is "waiting", not "incoming".
        info.state = CallState_Waiting;
    } else {
        info.state = CallState_Incoming;
    }
    info.number = number;
    info.calledNumber = calledNumber;
    info.incoming = true;
    info.dialBack = dialBack;
    info.name = name;
    callList += info;

    emitRing(info);
    emit callStatesChanged( &callList );

    // Announce the incoming call using Ericsson-style state notifications.
    sendState( info );

    // Start the ring timer to periodically send RING notifications until call is accepted.
    numRings = 0;
    ringTimer->start(2000);
}

void CallManager::startIncomingCall( const QString& number,
                                     const QString& calledNumber,
                                     const QString& name )
{
    startIncomingCall( number, calledNumber, name, false );
}

void CallManager::hangupAll()
{
    for ( int index = 0; index < callList.size(); ++index ) {
        callList[index].state = CallState_Hangup;
        sendState( callList[index] );
    }
    callList.clear();
    emit callStatesChanged( &callList );
}

void CallManager::hangupConnected()
{
    QList<CallInfo> newCallList;
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == CallState_Active ) {
            callList[index].state = CallState_Hangup;
            sendState( callList[index] );
        } else {
            newCallList += callList[index];
        }
    }
    callList = newCallList;

    if ( !hasCall( CallState_Held ) )
        waitingToIncoming();

    emit callStatesChanged( &callList );
}

void CallManager::hangupHeld()
{
    QList<CallInfo> newCallList;
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == CallState_Held ) {
            callList[index].state = CallState_Hangup;
            sendState( callList[index] );
        } else {
            newCallList += callList[index];
        }
    }
    callList = newCallList;

    if ( !hasCall( CallState_Active ) )
        waitingToIncoming();

    emit callStatesChanged( &callList );
}

void CallManager::hangupConnectedAndHeld()
{
    QList<CallInfo> newCallList;
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == CallState_Active ||
             callList[index].state == CallState_Held ) {
            callList[index].state = CallState_Hangup;
            sendState( callList[index] );
        } else {
            newCallList += callList[index];
        }
    }
    callList = newCallList;
    waitingToIncoming();
    emit callStatesChanged( &callList );
}

void CallManager::hangupCall( int id )
{
    chld1x( id );
}

void CallManager::hangupRemote( int id )
{
    int index = indexForId( id );
    if ( index >= 0 )
    {
        callList[index].state = CallState_Hangup;
        sendState( callList[index] );

        callList.removeAt( index );

        if ( !hasCall( CallState_Active ) && !hasCall( CallState_Held ) )
            waitingToIncoming();

        send ( "NO CARRIER" );
        emit callStatesChanged( &callList );
    }
}

bool CallManager::acceptCall()
{
    int id = idForIncoming();
    int index = indexForId(id);
    if ( id < 0 ) {
        return false;
    } else if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
        // No open slots to accept the call - fail it.  AT+CHLD=1 must be used instead.
        return false;
    } else if ( hasCall( CallState_Active ) ) {
        // Put the active calls on hold and accept the incoming call.
        changeGroup( CallState_Active, CallState_Held );
        callList[index].state = CallState_Active;
        sendState( callList[index] );
        emit callStatesChanged( &callList );
        return true;
    } else {
        // Only held calls, or no other calls, so just make the incoming call active.
        callList[index].state = CallState_Active;
        sendState( callList[index] );
        emit callStatesChanged( &callList );
        return true;
    }
}

bool CallManager::chld0()
{
    // If there is an incoming call, then that is the one to hang up.
    int id = idForIncoming();
    if ( id >= 0 )
        return chld1x( id );

    // Bail out if no held calls.
    if ( !hasCall( CallState_Held ) )
        return false;

    // Hang up the held calls.
    hangupHeld();
    return true;
}

bool CallManager::chld1()
{
    int id = idForIncoming();
    if ( id >= 0 ) {
        // Hangup the active calls and then make the incoming call active.
        hangupConnected();
        int index = indexForId(id);
        callList[index].state = CallState_Active;
        sendState( callList[index] );
        emit callStatesChanged( &callList );
        return true;
    } else if ( hasCall( CallState_Held ) ) {
        // Hangup the active calls and activate the held ones.
        hangupConnected();
        for ( int index = 0; index < callList.size(); ++index ) {
            if ( callList[index].state == CallState_Held ) {
                callList[index].state = CallState_Active;
                sendState( callList[index] );
                emit callStatesChanged( &callList );
            }
        }
        return true;
    } else if ( hasCall( CallState_Active ) ) {
        // We only have active calls - hang them up.
        hangupConnected();
        return true;
    } else if ( ( id = idForDialing() ) >= 0 ) {
        // We have a dialing call.
        hangupCall(id);
        return true;
    } else {
        return false;
    }
}

bool CallManager::chld1x( int x )
{
    QList<CallInfo> newCallList;
    bool found = false;
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].id == x ) {
            callList[index].state = CallState_Hangup;
            sendState( callList[index] );
            found = true;
        } else {
            newCallList += callList[index];
        }
    }
    callList = newCallList;

    if ( !hasCall( CallState_Active ) && !hasCall( CallState_Held ) )
        waitingToIncoming();

    emit callStatesChanged( &callList );
    return found;
}

bool CallManager::chld2()
{
    int id = idForIncoming();
    if ( id >= 0 ) {
        if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
            // Three-way calling situation: cannot do anything.
            return false;
        }
        if ( holdWillFail() && hasCall( CallState_Active ) ) {
            // Cannot put calls on hold at this time.
            return false;
        }
        changeGroup( CallState_Active, CallState_Held );
        int index = indexForId( id );
        callList[index].state = CallState_Active;
        sendState( callList[index] );
        emit callStatesChanged( &callList );
        return true;
    } else if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
        // Swap the active and held calls.
        if ( activateWillFail() || holdWillFail() )
            return false;
        changeGroup( CallState_Active, CallState_SwapDummy );
        changeGroup( CallState_Held, CallState_Active );
        changeGroup( CallState_SwapDummy, CallState_Held );
        return true;
    } else if ( hasCall( CallState_Active ) ) {
        // No held calls - put active calls on hold.
        if ( holdWillFail() )
            return false;
        changeGroup( CallState_Active, CallState_Held );
        return true;
    } else if ( hasCall( CallState_Held ) ) {
        // Re-activate the held calls.
        if ( activateWillFail() )
            return false;
        changeGroup( CallState_Held, CallState_Active );
        return true;
    } else {
        // No held or active calls - cannot do anything.
        return false;
    }
}

bool CallManager::chld2x( int x )
{
    int index = indexForId(x);
    if ( index < 0 )
        return false;
    if ( callList[index].state == CallState_Held ) {
        // Call is currently on hold - activate it.
        if ( activateWillFail() )
            return false;
        if ( hasCall( CallState_Active ) && countForState( CallState_Held ) > 1 ) {
            // Cannot swap because there are active calls, but multiple held calls.
            return false;
        } else if ( hasCall( CallState_Active ) ) {
            // Swap active and held calls.
            if ( holdWillFail() )
                return false;
            changeGroup( CallState_Active, CallState_SwapDummy );
            changeGroup( CallState_Held, CallState_Active );
            changeGroup( CallState_SwapDummy, CallState_Held );
        } else {
            // No active calls, so make just this call active.
            callList[index].state = CallState_Active;
            sendState( callList[index] );
            emit callStatesChanged( &callList );
        }
        return true;
    } else if ( callList[index].state == CallState_Active ) {
        // Call is currently active - put all others in the active group on hold.
        if ( activateWillFail() )
            return false;
        if ( hasCall( CallState_Held ) )
            return false;       // Can't do this if there is already a hold group.
        for ( int index2 = 0; index2 < callList.size(); ++index2 ) {
            if ( callList[index2].state == CallState_Active && index2 != index ) {
                if ( holdWillFail() )
                    return false;
                callList[index2].state = CallState_Held;
                sendState( callList[index2] );
                emit callStatesChanged( &callList );
            }
        }
        return true;
    } else {
        // Not held or active.
        return false;
    }
}

bool CallManager::chld3()
{
    if ( joinWillFail() ) {
        return false;
    } else if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
        if ( multipartyLimit() >= 0 &&
             ( countForState( CallState_Active ) +
               countForState( CallState_Held ) ) > multipartyLimit() ) {
            // We have exceeded the multiparty limit.
            return false;
        }
        changeGroup( CallState_Held, CallState_Active );
        return true;
    } else {
        return false;
    }
}

bool CallManager::chld4()
{
    if ( joinWillFail() ) {
        return false;
    } else if ( hasCall( CallState_Active ) && hasCall( CallState_Held ) ) {
        if ( multipartyLimit() >= 0 &&
             ( countForState( CallState_Active ) +
               countForState( CallState_Held ) ) > multipartyLimit() ) {
            // We have exceeded the multiparty limit.
            return false;
        }
        hangupConnectedAndHeld();
        return true;
    } else {
        return false;
    }
}

void CallManager::dialingToConnected()
{
    // Find the currently dialing or alerting call.
    int index = indexForId( idForState( CallState_Dialing ) );
    if ( index < 0 )
        index = indexForId( idForState( CallState_Alerting ) );
    if ( index < 0 )
        return;

    // Transition the call to its new state.
    callList[index].state = CallState_Active;
    sendState( callList[index] );
    emit callStatesChanged( &callList );
    // If the dialed number starts with 05123, disconnect the
    // call after xx seconds, where xx is part of the dial string
    // as 05123xx
    if( callList[index].number.startsWith( "05123" ) ) {
        bool ok;
        QString temp = callList[index].number;
        temp = temp.replace( "05123" , "" );
        int timeout = temp.toInt( &ok, 10 );
        timeout = ok ? timeout * 1000 : 10000;
        QTimer::singleShot( timeout, this, &CallManager::hangup );
    }
}

void CallManager::dialingToAlerting()
{
    // Find the currently dialing or alerting call.
    int index = indexForId( idForState( CallState_Dialing ) );
    if ( index < 0 )
        return;

    // Transition the call to its new state.
    callList[index].state = CallState_Alerting;
    sendState( callList[index] );
    emit callStatesChanged( &callList );
    // If the dialed number starts with 06123, accept that
    // call after xx seconds, where xx is part of the dial string
    // as 06123xx
    if( callList[index].number.startsWith( "06123" ) ) {
        bool ok;
        QString temp = callList[index].number;
        temp = temp.replace( "06123" , "" );
        int timeout = temp.toInt( &ok, 10 );
        timeout = ok ? timeout * 1000 : 10000;
        QTimer::singleShot( timeout, this, &CallManager::dialingToConnected );
    }
}

void CallManager::waitingToIncoming()
{
    int index = indexForId( idForState( CallState_Waiting ) );

    if ( index < 0 )
        return;

    callList[index].state = CallState_Incoming;
    sendState( callList[index] );
}

void CallManager::dialBack()
{
    startIncomingCall( "1234567", "7654321", "Alice", true );
}

void CallManager::dialBackWithHangup5()
{
    startIncomingCall( "1234567", "7654321", "Bob", true );
    hangupTimer->start( 5000 );
}

void CallManager::dialBackWithHangup4()
{
    startIncomingCall( "1234567", "7654321", "Mallory", true );
    hangupTimer->start( 4000 );
}

void CallManager::hangupTimeout()
{
    // Find the call that started off as incoming, even if it isn't any longer.
    // Note: this won't work too well if there are multiple dial-back calls.
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].dialBack ) {
            hangupCall( callList[index].id );
            break;
        }
    }
}

void CallManager::hangup()
{
    send ( "NO CARRIER" );
    hangupConnected();
}

void CallManager::sendNextRing()
{
    if ( idForIncoming() >= 0 ) {
        if ( numRings++ >= 10 ) {
            // Ringing for too long, so hang up the call.
            hangupCall( idForIncoming() );
        } else {
            for ( int index = 0; index < callList.size(); ++index )
                if ( callList[index].id == idForIncoming() && callList[index].state == CallState_Incoming )
                    emitRing(callList[index]);
            ringTimer->start(2000);
        }
    }
}

int CallManager::newId()
{
    int id;
    bool seen;
    for ( id = 1; id <= 32; ++id ) {
        seen = false;
        for ( const CallInfo &info : qAsConst(callList) ) {
            if ( info.id == id ) {
                seen = true;
                break;
            }
        }
        if ( !seen )
            return id;
    }
    return -1;
}

int CallManager::idForDialing()
{
    int id = idForState( CallState_Dialing );
    if ( id < 0 )
        id = idForState( CallState_Alerting );
    return id;
}

int CallManager::idForIncoming()
{
    int id = idForState( CallState_Incoming );
    if ( id < 0 )
        id = idForState( CallState_Waiting );
    return id;
}

int CallManager::idForState( CallState state )
{
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == state )
            return callList[index].id;
    }
    return -1;
}

int CallManager::countForState( CallState state )
{
    int count = 0;
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == state )
            ++count;
    }
    return count;
}

int CallManager::indexForId( int id )
{
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].id == id )
            return index;
    }
    return -1;
}

bool CallManager::hasCall( CallState state )
{
    for ( const CallInfo &info : qAsConst(callList) ) {
        if ( info.state == state )
            return true;
    }
    return false;
}

void CallManager::changeGroup( CallState oldState, CallState newState )
{
    for ( int index = 0; index < callList.size(); ++index ) {
        if ( callList[index].state == oldState ) {
            callList[index].state = newState;
            sendState( callList[index] );
        }
    }
    emit callStatesChanged( &callList );
}

void CallManager::sendState( const CallInfo& info )
{
    static int const stateMap[] = {3, 4, 1, 6, 5, 5, 0, 0};
    if ( info.state == CallState_SwapDummy )
        return;     // In the middle of a state swap: don't send this.
    QString line =
        "*ECAV: " + QString::number(info.id) + "," +
        QString::number(stateMap[(int)(info.state)]) + ",0";
    if ( !info.number.isEmpty() ) {
        line += ",,," + QAtUtils::encodeNumber(info.number);
    }
    if ( info.state == CallState_Incoming || info.state == CallState_Waiting ) {
        // Stop sending RING notifications.
        ringTimer->stop();
    }
    if ( info.state == CallState_Hangup && info.dialBack ) {
        // Hanging up an incoming call that started as a dialback.  Stop hangup timer.
        hangupTimer->stop();
    }
    emit unsolicited( line );
}
