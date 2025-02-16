/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2015
 * FUNCTION :
-*/


#include "applicationdata.h"
#include "networkcommon.h"
#include "netsocket.h"
#include "systeminfo.h"
#include "timer.h"

#include "testprog.h"

#include <QHostAddress>

static const char* localhoststr = Network::Socket::sKeyLocalHost();


class TestClass : public CallBacker
{
public:
    TestClass()
        : timer_( "starter" )
    {
        mAttachCB( timer_.tick, TestClass::timerTick );
        timer_.start( 0, true );
    }

    ~TestClass()
    {
        detachAllNotifiers();
        CallBack::removeFromThreadCalls( this );
    }

private:

    void timerTick( CallBacker* )
    {
	retval_ = testNetAuthority() && testSystemInfo() ? 0 : 1;
	CallBack::addToMainThread( mCB(this,TestClass,closeTesterCB) );
    }

    void closeTesterCB( CallBacker* )
    {
	ApplicationData::exit( retval_ );
    }

    const char* getDomainName() const
    {
	static BufferString domainnm( System::localDomainName() );
	if ( domainnm.isEmpty() )
	    domainnm.set( "enschede.dgbes.com" );
	return domainnm.buf();
    }

    bool testSystemInfo()
    {
	//Dummy test in a sense, as we cannot check the result
	mRunStandardTest( System::macAddressHash(), "macAddressHash" );

	const BufferString localaddress = System::localAddress();
	mRunStandardTest( !localaddress.isEmpty(), "Local address" );

	BufferString hostaddress = System::hostAddress( localhoststr, true );
	mRunStandardTest( hostaddress=="127.0.0.1", "localhost address ipv4" );

	hostaddress = System::hostAddress( localhoststr, false );
	mRunStandardTest( hostaddress=="::1", "localhost address ipv6" );

	hostaddress = System::hostAddress( "dgb29", true );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29 ipv4" );

	hostaddress = System::hostAddress( "dgb29", false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29 ipv6" );

	const BufferString dgb29hostname( "dgb29.", getDomainName() );
	hostaddress = System::hostAddress( dgb29hostname );
	mRunStandardTest( hostaddress=="192.168.0.29", "dgb29.domain ipv4" );

	hostaddress = System::hostAddress( dgb29hostname, false );
	mRunStandardTest( !hostaddress.isEmpty(), "dgb29.domain ipv6" );

	return true;
    }

    bool testNetAuthority()
    {
	TypeSet<QHostAddress> qaddrs;
	qaddrs += QHostAddress( QHostAddress::Null );
	qaddrs += QHostAddress( QHostAddress::Broadcast );
	qaddrs += QHostAddress( QHostAddress::LocalHost );
	qaddrs += QHostAddress( QHostAddress::LocalHostIPv6 );
	qaddrs += QHostAddress( QHostAddress::AnyIPv6 );
	qaddrs += QHostAddress( QHostAddress::AnyIPv4 );
	qaddrs += QHostAddress( QHostAddress::Any );

	static PortNr_Type port = 12345;
	TypeSet<Network::Authority> auths;
	for ( const auto& qaddr : qaddrs )
	{
	    const BufferString addr( qaddr.toString() );
	    auths += Network::Authority( addr, port );
	}

	const BufferString localhostnm( System::localHostName() );
	const BufferString localfqhn( System::localFullHostName() );
	const BufferString localipaddr( System::localAddress() );
	BufferStringSet exphostaddrs;
	exphostaddrs
	    .add( "" ).add( "255.255.255.255" )
	    .add( localhoststr ).add( localhoststr )
	    .add( localfqhn ).add( localfqhn ).add( localfqhn );
	BoolTypeSet isoks( exphostaddrs.size(), true );
	BoolTypeSet isaddrs( exphostaddrs.size(), true );
	isoks[0] = false; isaddrs[0] = false;

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	auths.setEmpty(); //Strings equivalent of these special addresses
	auths += Network::Authority( "", port );
	auths += Network::Authority( "255.255.255.255", port );
	auths += Network::Authority( "127.0.0.1", port);
	auths += Network::Authority( "::1", port );
	auths += Network::Authority( "0.0.0.0", port );
	auths += Network::Authority( "::", port );
	// QHostAddress::Any does not have a string equivalent
	auths += Network::Authority( localipaddr, port );
	exphostaddrs.last()->set( localipaddr );

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	auths.setEmpty(); isoks.setEmpty(); isaddrs.setEmpty();
	exphostaddrs.setEmpty();
	exphostaddrs.add( localfqhn ).add( localhostnm ).add( localipaddr );
	TypeSet<Network::Authority::ConnType> conntypes;
	conntypes += Network::Authority::FQDN;
	conntypes += Network::Authority::HostName;
	conntypes += Network::Authority::IPv4;
	for ( int idx=0; idx<exphostaddrs.size(); idx++ )
	{
	    const QHostAddress qaddr = QHostAddress( QHostAddress::Any );
	    auths += Network::Authority( BufferString(qaddr.toString()), port );
	    isoks += true;
	    isaddrs += true;
	}

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getConnHost( conntypes[idx] );
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	// Now hostname based authorities

	const BufferString dgb29hostname( "dgb29.", getDomainName() );
	exphostaddrs.setEmpty();
	exphostaddrs
	    .add( localhoststr )
	    .add( localhostnm )
	    .add( "dgb29" )
	    .add( dgb29hostname );

	auths.setEmpty(); isoks.setEmpty(); isaddrs.setEmpty();
	for ( const auto* exphostaddr : exphostaddrs )
	{
	    auths += Network::Authority( exphostaddr->str(), port );
	    isoks += true;
	    isaddrs += false;
	}

	for ( int idx=0; idx<auths.size(); idx++ )
	{
	    const Network::Authority& auth = auths[idx];
	    const bool isok = auth.isOK();
	    const bool isaddr = auth.isAddressBased();
	    const BufferString& exphostnm = exphostaddrs.get( idx );
	    const BufferString hostnm = auth.getHost();
	    mRunStandardTest( hostnm == exphostnm && auth.getPort() == port &&
			      isok == isoks[idx] && isaddr == isaddrs[idx],
		BufferString( "Authority for host ", exphostnm ) );
	}

	return true;
    }

    Timer	timer_;
    int		retval_ = 0;

};




int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    ApplicationData app;
    TestClass tester;

    return app.exec();
}
