/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/


#include "raytracerrunner.h"

#include "ailayer.h"
#include "iopar.h"

const char* RayTracerRunner::sKeyParallel()	{ return "parallel"; }


RayTracerRunner::RayTracerRunner( const char* rt1dfactkeywd )
    : ParallelTask("Raytracing")
    , raypar_(*new IOPar())
{
    const Factory<RayTracer1D>& rt1dfact = RayTracer1D::factory();
    if ( rt1dfact.isEmpty() )
	return;

    if ( rt1dfact.hasName(rt1dfactkeywd) )
    {
	raypar_.set( sKey::Type(), rt1dfactkeywd );
	return;
    }

    const BufferStringSet& factnms = rt1dfact.getNames();
    if ( !factnms.isEmpty() )
    {
	const FixedString defnm( rt1dfact.getDefaultName() );
	if ( !defnm.isEmpty() )
	    raypar_.set( sKey::Type(), defnm.str() );
    }
}


RayTracerRunner::RayTracerRunner( const IOPar& raypars )
    : ParallelTask("Raytracing")
    , raypar_(*new IOPar(raypars))
{
    msg_ = tr("Running raytracers");
}


RayTracerRunner::RayTracerRunner( const TypeSet<ElasticModel>& aims,
				  const IOPar& raypars )
    : RayTracerRunner(raypars)
{
    setModel( aims );
}


RayTracerRunner::~RayTracerRunner()
{
    deepErase( raytracers_ );
    delete &raypar_;
}


od_int64 RayTracerRunner::nrIterations() const
{
    return totalnr_;
}


uiString RayTracerRunner::uiNrDoneText() const
{
    return tr("Layers done");
}


od_int64 RayTracerRunner::nrDone() const
{
    od_int64 nrdone = 0;
    for ( const auto* rt1d : raytracers_ )
	nrdone += rt1d->nrDone();
    return nrdone;
}


void RayTracerRunner::setOffsets( const TypeSet<float>& offsets )
{
    raypar_.set( RayTracer1D::sKeyOffset(), offsets );
    for ( auto* rt : raytracers_ )
	rt->setOffsets( offsets );
}


#define mErrRet(msg) { msg_ = msg; return false; }

bool RayTracerRunner::setModel( const TypeSet<ElasticModel>& aimodels )
{
    deepErase( raytracers_ );

    if ( aimodels.isEmpty() )
	mErrRet( tr("No Elastic models set") );

    if ( RayTracer1D::factory().getNames().isEmpty() )
	mErrRet( tr("RayTracer factory is empty") )

    uiString errmsg;
    totalnr_ = 0;
    for ( const auto& aimodel : aimodels )
    {
	RayTracer1D* rt1d = RayTracer1D::createInstance( raypar_, &aimodel,
							 errmsg );
	if ( !rt1d )
	{
	    uiString msg = tr( "Wrong input for raytracing on model: %1" )
				.arg(aimodels.indexOf(aimodel)+1);
	    msg.append( errmsg, true );
	    mErrRet( msg );
	}

	const int totnr = rt1d->totalNr();
	if ( totnr > 0 )
	    totalnr_ += totnr;

	raytracers_ += rt1d;
    }

    return true;
}


bool RayTracerRunner::doPrepare( int /* nrthreads */ )
{
    msg_ = tr("Running raytracers");

    return !raytracers_.isEmpty();
}


bool RayTracerRunner::doWork( od_int64 start, od_int64 stop, int threadidx )
{
    const bool parallel = start == 0 && threadidx == 0 &&
		(stop == nrIterations()-1);

    bool startlayer = false;
    int startmdlidx = modelIdx( start, startlayer );
    if ( !startlayer ) startmdlidx++;
    const int stopmdlidx = modelIdx( stop, startlayer );
    for ( int idx=startmdlidx; idx<=stopmdlidx; idx++ )
    {
	ParallelTask* rt1d = raytracers_[idx];
	if ( !rt1d->executeParallel(!parallel) )
	    mErrRet( rt1d->uiMessage() );
    }

    return true;
}


bool RayTracerRunner::doFinish( bool success )
{
    if ( !success )
	deepErase( raytracers_ );

    return success;
}


int RayTracerRunner::modelIdx( od_int64 idx, bool& startlayer ) const
{
    od_int64 stopidx = -1;
    startlayer = false;
    int modelidx = 0;
    for ( const auto* rt : raytracers_ )
    {
	if ( idx == (stopidx + 1) )
	    startlayer = true;

	const int totnr = rt->totalNr();
	stopidx += totnr > 0 ? totnr : 0;
	if ( stopidx>=idx )
	    return modelidx;

	modelidx++;
    }

    return -1;
}


ConstRefMan<ReflectivityModelSet> RayTracerRunner::getRefModels() const
{
    RefMan<ReflectivityModelSet> ret = new ReflectivityModelSet( raypar_ );
    getResults( *ret.ptr() );
    return ConstRefMan<ReflectivityModelSet>( ret.ptr() );
}


bool RayTracerRunner::getResults( ReflectivityModelSet& ret ) const
{
    const ObjectSet<RayTracer1D>& raytracers = raytracers_;
    if ( raytracers.isEmpty() )
	return true;

    if ( !raytracers.first()->setup().doreflectivity_ )
	return false;

    for ( const auto* rt : raytracers )
    {
	ConstRefMan<OffsetReflectivityModel> refmodel = rt->getRefModel();
	if ( !refmodel || !refmodel->isOK() )
	    return false;

	ret.add( *refmodel );
    }

    return true;
}


ConstRefMan<ReflectivityModelSet> RayTracerRunner::getRefModels(
				    const TypeSet<ElasticModel>& emodels,
				    const IOPar& raypar, uiString& msg,
				    TaskRunner* taskrun,
			    const ObjectSet<const TimeDepthModel>* tdmodels )
{
    IOPar raypars( raypar );
    if ( !raypar.isPresent(sKey::Type()) )
    {
	const BufferStringSet& facnms = RayTracer1D::factory().getNames();
	if ( !facnms.isEmpty() )
	{
	    const BufferString defnm( RayTracer1D::factory().getDefaultName() );
	    raypars.set( sKey::Type(), defnm.isEmpty()
		    ? VrmsRayTracer1D::sFactoryKeyword() : defnm.str() );
	}
    }

    if ( !raypar.isPresent(RayTracer1D::sKeyOffset()) )
	RayTracer1D::setIOParsToZeroOffset( raypars );

    RayTracerRunner rtrunner( emodels, raypars );
    bool parallel = true;
    if ( raypars.getYN(sKeyParallel(),parallel) )
	rtrunner.doParallel( parallel );

    if ( !TaskRunner::execute(taskrun,rtrunner) )
    {
	msg = rtrunner.uiMessage();
	return nullptr;
    }

    RefMan<ReflectivityModelSet> refmodels = new ReflectivityModelSet( raypars);
    if ( !rtrunner.getResults(*refmodels.ptr()) )
	return nullptr;

    if ( tdmodels )
	refmodels->use( *tdmodels );

    return refmodels.ptr();
}
