/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : 21-1-1998
 * FUNCTION : Seismic data storage
-*/

static const char* rcsID = "$Id: seisstor.cc,v 1.8 2003-10-15 15:15:54 bert Exp $";

#include "seisstor.h"
#include "seistrctr.h"
#include "seistrcsel.h"
#include "ioobj.h"
#include "iopar.h"
#include "ioman.h"

const char* SeisStorage::sNrTrcs = "Nr of traces";


SeisStorage::SeisStorage( const IOObj* ioob )
	: trl(0)
	, ioobj(0)
	, trcsel(0)
	, selcomp(-1)
{
    setIOObj( ioob );
}


SeisStorage::~SeisStorage()
{
    cleanUp( true );
}


void SeisStorage::setIOObj( const IOObj* ioob )
{
    close();
    if ( !ioob ) return;
    ioobj = ioob->clone();
    trl = (SeisTrcTranslator*)ioobj->getTranslator();
    if ( !trl )
	{ delete ioobj; ioobj = 0; }
    else
	trl->setTrcSel( trcsel );
}


const Conn* SeisStorage::curConn() const
{ return trl ? trl->curConn() : 0; }
Conn* SeisStorage::curConn()
{ return trl ? trl->curConn() : 0; }


void SeisStorage::setTrcSel( SeisTrcSel* tsel )
{
    delete trcsel; trcsel = tsel;
    if ( trl ) trl->setTrcSel( trcsel );
}


void SeisStorage::cleanUp( bool alsoioobj )
{
    delete trl; trl = 0;
    nrtrcs = 0;
    if ( alsoioobj )
    {
	delete ioobj; ioobj = 0;
	delete trcsel; trcsel = 0;
    }
    init();
}


void SeisStorage::close()
{
    cleanUp( false );
}


void SeisStorage::fillPar( IOPar& iopar ) const
{
    if ( ioobj ) iopar.set( "ID", ioobj->key() );
}


void SeisStorage::usePar( const IOPar& iopar )
{
    const char* res = iopar["ID"];
    if ( *res )
    {
	IOObj* ioob = IOM().get( res );
	if ( ioob && (!ioobj || ioobj->key() != ioob->key()) )
	    setIOObj( ioob );
	delete ioob;
    }

    if ( !trcsel ) trcsel = new SeisTrcSel;
    trcsel->usePar( iopar );
    if ( trcsel->isEmpty() ) { delete trcsel; trcsel = 0; }

    if ( trl )
    {
	trl->setTrcSel( trcsel );
	trl->usePar( iopar );
    }

    iopar.get( "Selected component", selcomp );
}
