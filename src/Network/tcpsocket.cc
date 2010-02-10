/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          March 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: tcpsocket.cc,v 1.5 2010-02-10 08:08:45 cvsranojay Exp $";

#include "tcpsocket.h"
#include "qtcpsocketcomm.h"

#define mInit \
    , comm_(new QTcpSocketComm(qtcpsocket_,this)) \
    , connected(this) \
    , disconnected(this) \
    , hostFound(this) \
    , readyRead(this) \
    , error(this) \
    , stateChanged(this)

TcpSocket::TcpSocket()
    : qtcpsocket_(new QTcpSocket)
    mInit
{}


TcpSocket::TcpSocket( QTcpSocket* qsocket )
    : qtcpsocket_(qsocket)
    mInit
{}


TcpSocket::~TcpSocket()
{
    delete qtcpsocket_;
    delete comm_;
}


const char* TcpSocket::errorMsg() const
{
    errmsg_ = qtcpsocket_->errorString().toAscii().constData();
    return errmsg_.buf();
}


void TcpSocket::connectToHost( const char* host, int port )
{ qtcpsocket_->connectToHost( QString(host), port ); }

void TcpSocket::disconnectFromHost()
{ qtcpsocket_->disconnectFromHost(); }

void TcpSocket::abort()
{ qtcpsocket_->abort(); }

bool TcpSocket::waitForConnected( int msec )
{ return qtcpsocket_->waitForConnected( msec ); }

bool TcpSocket::waitForReadyRead( int msec )
{ return qtcpsocket_->waitForReadyRead( msec ); }

int TcpSocket::write( const char* str )
{ return qtcpsocket_->write( str ); }

void TcpSocket::read( BufferString& str )
{
    QByteArray ba = qtcpsocket_->readAll();
    str = ba.data();
}
