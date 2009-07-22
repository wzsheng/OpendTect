/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2008
-*/

static const char* rcsID = "$Id: od_process_prestack.cc,v 1.6 2009-07-22 16:01:34 cvsbert Exp $";

#include "batchprog.h"

#include "horsampling.h"
#include "envvars.h"
#include "prestackprocessor.h"
#include "prestackprocessortransl.h"
#include "hostdata.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mmsockcommunic.h"
#include "progressmeter.h"
#include "posinfo.h"
#include "flatposdata.h"
#include "prestackgather.h"
#include "seispsread.h"
#include "seispswrite.h"
#include "seispsioprov.h"
#include "seistrc.h"
#include "seistype.h"
#include "survinfo.h"
#include "timefun.h"


#include "initalgo.h"
#include "initgeneral.h"
#include "initgeometry.h"
#include "initearthmodel.h"
#include "initseis.h"
#include "initattributeengine.h"
#include "initprestackprocessing.h"

#include <iostream>


#define mDestroyWorkers \
	{ delete procman; procman = 0; writer = 0; }

//defineTranslatorGroup(AttribDescSet,"Attribute definitions");


#define mRetError(s) \
{ errorMsg(s); mDestroyWorkers; return false; }

#define mRetHostErr(s) \
	{  \
	    if ( comm ) comm->setState( MMSockCommunic::HostError ); \
	    mRetError(s) \
	}

#define mRetJobErr(s) \
	{  \
	    if ( comm ) comm->setState( MMSockCommunic::JobError ); \
	    mRetError(s) \
	}

#define mRetFileProb(fdesc,fnm,s) \
	{ \
	    BufferString msg(fdesc); \
	    msg += " ("; msg += fnm; msg += ") "; msg += s; \
	    mRetHostErr( msg ); \
	}

#define mStrmWithProcID(s) \
    strm << "\n[" << process_id << "]: " << s << "." << std::endl

#define mSetCommState(State) \
	if ( comm ) \
	{ \
	    comm->setState( MMSockCommunic::State ); \
	    if ( !comm->updateState() ) \
		mRetHostErr( comm->errMsg() ) \
	}


bool BatchProgram::go( std::ostream& strm )
{
    strm << "Processing on " << HostData::localHostName() << '.' << std::endl;

    if ( !parversion_.isEmpty() )
    {
	const float vsn = atof( parversion_.buf() );
	if ( vsn < 3.2 )
	    { errorMsg("\nCannot execute pre-3.2 par files"); return false; }
    }

    const int process_id = GetPID();

    Algo::initStdClasses();
    General::initStdClasses();
    Geometry::initStdClasses();
    EarthModel::initStdClasses();
    Seis::initStdClasses();
    AttributeEngine::initStdClasses();
    PreStackProcessing::initStdClasses();

    double startup_wait = 0;
    pars().get( "Startup delay time", startup_wait );
    Time_sleep( startup_wait );

    const double pause_sleep_time = GetEnvVarDVal( "OD_BATCH_SLEEP_TIME", 1 );
    TextStreamProgressMeter progressmeter(strm);

    HorSampling horsampling( true );
    const bool hashorsampling = horsampling.usePar( pars() );

    BufferString linekey;
    pars().get( PreStack::ProcessManager::sKeyLineKey(), linekey );

    Seis::GeomType geomtype;
    if ( !Seis::getFromPar( pars(), geomtype ) )
    {
	errorMsg("\nCannot read geometry type");
	return false;
    }

    if ( geomtype!=Seis::VolPS && geomtype!=Seis::LinePS )
    {
	errorMsg("\nGeometry is not prestack");
	return false;
    }

    MultiID setupmid;
    if ( !pars().get( PreStack::ProcessManager::sKeySetup(), setupmid ) )
    {
	errorMsg( "\nCannot read setup" );
	return false;
    }

    PtrMan<IOObj> setupioobj = IOM().get( setupmid ); 
    if ( !setupioobj )
    {
	errorMsg("\nCannot create setup object");
	return false;
    }

    mDeclareAndTryAlloc( PreStack::ProcessManager*, procman,
	    		 PreStack::ProcessManager );

    if ( !procman )
    {
	errorMsg( "Cannot create processor");
	return false;
    }

    BufferString errmsg;
    if ( !PreStackProcTranslator::retrieve( *procman, setupioobj, errmsg ) ) 
    { 
	errorMsg( errmsg.buf() );
	delete procman;
	return false;
    } 
    
    if ( geomtype==Seis::LinePS && linekey.isEmpty() )
    {
	errorMsg("\nNo linekey set" );
	delete procman;
	return false;
    }

    MultiID inputmid;
    if ( !pars().get(PreStack::ProcessManager::sKeyInputData(), inputmid ) )
    {
	errorMsg("\nCannot read input id");
	delete procman;
	return false;
    }

    PtrMan<IOObj> inputioobj = IOM().get( inputmid );
    if ( !inputioobj )
    {
	errorMsg("\nCannot create input object");
	delete procman;
	return false;
    }

    MultiID outputmid;
    if ( !pars().get( PreStack::ProcessManager::sKeyOutputData(), outputmid ) )
    {
	errorMsg("\nCannot read output id");
	delete procman;
	return false;
    }

    PtrMan<IOObj> outputioobj = IOM().get( outputmid );
    if ( !outputioobj )
    {
	errorMsg("\nCannot create output object");
	delete procman;
	return false;
    }

    PtrMan<SeisPS3DReader> reader3d = 0;
    PtrMan<SeisPS2DReader> reader2d = 0;

    StepInterval<int> cdprange(0,0,1);

    if ( geomtype==Seis::VolPS )
    {
	reader3d = SPSIOPF().get3DReader( *inputioobj );
	if ( reader3d && !hashorsampling )
	{
	    const PosInfo::CubeData& posdata = reader3d->posData();
	    if ( posdata.size() )
	    {
		StepInterval<int> inlrg, crlrg;
		posdata.getInlRange( inlrg );
		posdata.getCrlRange( crlrg );

		horsampling.init();
		horsampling.setInlRange( inlrg );
		horsampling.setCrlRange( crlrg );
	    }
        }
    }
    else
    {
	reader2d = SPSIOPF().get2DReader( *inputioobj, linekey.buf() );
	if ( reader2d &&
	    !pars().get( PreStack::ProcessManager::sKeyCDPRange(), cdprange ) )
	{
	    const PosInfo::Line2DData& posdata = reader2d->posData();
	    for ( int idx=0; idx<posdata.posns_.size(); idx++ )
	    {
		if ( !idx )
		    cdprange.start = cdprange.stop = posdata.posns_[idx].nr_;
		else
		    cdprange.include( posdata.posns_[idx].nr_ );
	    }
	}
    }

    if ( !reader3d && !reader2d )
    {
	errorMsg("\nCannot create input reader");
	delete procman;
	return false;
    }

    SeisPSReader* reader = reader3d
	? (SeisPSReader*) reader3d
	: (SeisPSReader*) reader2d;

    PtrMan<SeisPSWriter> writer = geomtype==Seis::VolPS
	? SPSIOPF().get3DWriter( *outputioobj )
	: SPSIOPF().get2DWriter( *outputioobj, linekey.buf() );

    if ( !writer )
    {
	errorMsg("\nCannot create output writer");
	delete procman;
	return false;
    }

    const BinID stepout = procman->getInputStepout();

    BinID curbid; //inl not used if 2D
    BinID step;   //inl not used if 2D
    HorSamplingIterator hiter( horsampling );

    if ( geomtype==Seis::LinePS )
    {
	curbid.crl = cdprange.start;
	step.crl = 1;
    }
    else
    {
	if ( !hiter.next(curbid) )
	{
	    errorMsg("\nNo CDP's to process");
	    delete procman;
	    return false;
	}

	step.inl = SI().inlRange(true).step;
	step.crl = SI().crlRange(true).step;
    }

    mSetCommState(Working);

    while ( true )
    {
	bool paused = false;

	if ( pauseRequested() )
	{ 
	    paused = true;
	    mSetCommState(Paused);
	    Time_sleep( pause_sleep_time );  
	    continue;
	}

	if ( paused )
	{
	    paused = false;
	    mSetCommState(Working);
	}

	procman->reset();
	BinID relbid;

	if ( !procman->prepareWork() )
	{
	    errorMsg("\nCannot prepare processing.");
	    delete procman;
	    return false;
	}

	int nrfound = 0;
	for ( relbid.inl=-stepout.inl; relbid.inl<=stepout.inl; relbid.inl++ )
	{
	    for ( relbid.crl=-stepout.crl; relbid.crl<=stepout.crl;relbid.crl++)
	    {
		if ( !procman->wantsInput( relbid ) )
		    continue;

		const BinID inputbid( curbid.inl+relbid.inl*step.inl,
				      curbid.crl+relbid.crl*step.crl );

		PreStack::Gather* gather = new PreStack::Gather;
		if ( !gather->readFrom(*inputioobj,*reader,inputbid,0) )
		{
		    delete gather;
		    continue;
		}

		nrfound++;

		DPM( DataPackMgr::FlatID() ).addAndObtain( gather );
		procman->setInput( relbid, gather->id() );
		DPM( DataPackMgr::FlatID() ).release( gather );
	    }
	}

	if ( nrfound )
	{
	    if ( !procman->process() )
	    {
		errorMsg("\nCannot process.");
		delete procman;
		return false;
	    }

	    const DataPack* dp =
		DPM(DataPackMgr::FlatID()).obtain(procman->getOutput());
	    mDynamicCastGet( const PreStack::Gather*, gather, dp );
	    if ( gather )
	    {
		const int nrtraces =
		    gather->size( !PreStack::Gather::offsetDim() );
		const int nrsamples =
		    gather->size( PreStack::Gather::offsetDim() );
		const StepInterval<double> zrg =
		    gather->posData().range( PreStack::Gather::offsetDim() );
		SeisTrc trc( nrsamples );
		trc.info().sampling.start = zrg.start;
		trc.info().sampling.step = zrg.step;

		if ( reader2d )
		{
		    trc.info().nr = curbid.crl;
		    PosInfo::Line2DPos linepos;
		    if ( reader2d->posData().getPos( curbid.crl, linepos ) )
			trc.info().coord = linepos.coord_;
		}
		else
		{
		    trc.info().binid = curbid;
		    trc.info().coord = SI().transform( curbid );
		}

		for ( int idx=0; idx<nrtraces; idx++ )
		{
		    trc.info().azimuth = gather->getAzimuth( idx );
		    trc.info().offset = gather->getOffset( idx );
		    for ( int idy=0; idy<nrsamples; idy++ )
			trc.set( idy, gather->data().get( idx, idy ), 0 );

		    if ( !writer->put( trc ) )
		    {
			errorMsg("\nCannot write output");
			delete procman;
			return false;
		    }
		}

		DPM(DataPackMgr::FlatID()).release( dp );
	    }

	    ++progressmeter;
	}


	if ( geomtype==Seis::VolPS )
	{
	    if ( !hiter.next(curbid) )
		break;
	}
	else
	{
	    curbid.crl += cdprange.step;
	    if ( !cdprange.includes( curbid.crl ) )
		break;
	}

    }

    // It is VERY important workers are destroyed BEFORE the last sendState!!!
    mDestroyWorkers
    progressmeter.setFinished();
    mStrmWithProcID( "Threads closed; Writing finish status" );

    if ( !comm )
    {
	delete procman;
	return true;
    }

    comm->setState( MMSockCommunic::Finished );
    bool ret = comm->sendState();

    if ( ret )
	mStrmWithProcID( "Successfully wrote finish status" );
    else
	mStrmWithProcID( "Could not write finish status" );

    delete procman;
    return ret;
}


