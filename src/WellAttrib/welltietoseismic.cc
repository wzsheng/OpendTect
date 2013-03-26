/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Jan 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "welltietoseismic.h"

#include "ioman.h"
#include "arrayndimpl.h"
#include "arrayndutils.h"
#include "raytrace1d.h"
#include "synthseis.h"
#include "seistrc.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllog.h"
#include "welllogset.h"
#include "welld2tmodel.h"
#include "welltiedata.h"
#include "welltieunitfactors.h"
#include "welltieextractdata.h"
#include "welltrack.h"


namespace WellTie
{
static const int cDefTimeResampFac = 20;

#define mErrRet(msg) { errmsg_ = msg; return false; }
#define mGetWD() { wd_ = data_.wd_; if ( !wd_ ) return false; }
DataPlayer::DataPlayer( Data& data, const MultiID& seisid, const LineKey* lk ) 
    : data_(data)		    
    , seisid_(seisid)
    , linekey_(lk)
{
    disprg_ = data_.timeintv_;
    dispsz_ = disprg_.nrSteps();

    workrg_ = disprg_; workrg_.step /= cDefTimeResampFac;
    worksz_ = workrg_.nrSteps()+1;
}


bool DataPlayer::computeAll()
{
    mGetWD();

    d2t_ = wd_->d2TModel(); 
    if ( !d2t_ )
	mErrRet( "No depth/time model computed" );


    bool success = setAIModel() && doFullSynthetics() && extractSeismics(); 
    copyDataToLogSet();

    computeAdditionalInfo( disprg_ );

    return success;
}


bool DataPlayer::processLog( const Well::Log* log, 
			     Well::Log& outplog, const char* nm ) 
{
    BufferString msg;
    if ( !log ) 
	{ msg += "Can not find "; msg += nm; mErrRet( msg ); }

    outplog.setUnitMeasLabel( log->unitMeasLabel() );

    int sz = log->size();
    for ( int idx=1; idx<sz-1; idx++ )
    {
	const float prvval = log->value( idx-1 );
	const float curval = log->value( idx );
	const float nxtval = log->value( idx+1 );
	if ( mIsUdf(prvval) || mIsUdf(curval) || mIsUdf(nxtval) )
	    continue;

	outplog.addValue( log->dah(idx), (prvval + curval + nxtval )/3 );
    }
    sz = outplog.size();
    if ( sz <= 2 )
    {
	msg += nm;
	msg +="log size too small, please check your input log";
	mErrRet(msg)
    }

    GeoCalculator gc; 
    gc.removeSpikes( outplog.valArr(), sz, 10, 3 );

    return true;
}


bool DataPlayer::setAIModel()
{
    aimodel_.erase();

    const Well::Log* sonlog = wd_->logs().getLog( data_.sonic() );
    const Well::Log* denlog = wd_->logs().getLog( data_.density() );

    Well::Log pslog, pdlog;
    if ( !processLog( sonlog, pslog, data_.sonic() ) 
	    || !processLog( denlog, pdlog, data_.density() ) )
	return false;

    if ( data_.isSonic() )
	{ GeoCalculator gc; gc.son2Vel( pslog, true ); }

    const float srddepth = -1.f * data_.wd_->info().srdelev;

    float prev_depth = srddepth;
    // Now we take srd from well; we should get it from SI() if it is set there
    float thickness = 0;
    for ( int idx=0; idx<worksz_; idx++ )
    {
	const float twt = workrg_.atIndex( idx );
	const float dah = d2t_->getDah( twt );
	const float depth = (float) wd_->track().getPos(dah).z;
	if ( depth <= srddepth || mIsUdf(dah) || mIsUdf(depth) )
	    continue;
	if ( !data_.dahrg_.includes(dah,true) )
	    continue;
	const float vel = pslog.getValue( dah, true );
	const float den = pdlog.getValue( dah, true );
	if ( mIsUdf(vel) || mIsUdf(den) || mIsEqual(prev_depth,depth,1e-3) )
	    continue;
	thickness = depth - prev_depth;
	aimodel_ += AILayer( thickness, vel, den );
	prev_depth = depth;
    }

    return true;
}


bool DataPlayer::doFullSynthetics()
{
    refmodel_.erase();
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;
    StepInterval<float> reflrg = workrg_;
    reflrg.start = d2t_->getTime( data_.dahrg_.start, wd_->track() );
    reflrg.stop  = d2t_->getTime( data_.dahrg_.stop, wd_->track() );
    if ( disprg_.start > reflrg.start )
	reflrg.start = workrg_.start; 
    if ( disprg_.stop < reflrg.stop )
	reflrg.stop = workrg_.stop; 

    Seis::RaySynthGenerator gen;
    gen.addModel( aimodel_ );
    gen.forceReflTimes( reflrg );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( disprg_ );
    IOPar par;
    gen.usePar( par ); 
    TaskRunner* tr = data_.trunner_;
    if ( !TaskRunner::execute( tr, gen ) )
	mErrRet( gen.errMsg() )

    Seis::RaySynthGenerator::RayModel& rm = gen.result( 0 );
    data_.synthtrc_ = *rm.stackedTrc();
    rm.getSampledRefs( reflvals_ );
    for ( int idx=0; idx<reflvals_.size(); idx++ )
    {
	refmodel_ += ReflectivitySpike(); 
	refmodel_[idx].reflectivity_ = reflvals_[idx];
    }

    return true;
}


bool DataPlayer::doFastSynthetics()
{
    const Wavelet& wvlt = data_.isinitwvltactive_ ? data_.initwvlt_ 
						  : data_.estimatedwvlt_;
    Seis::SynthGenerator gen;
    gen.enableFourierDomain( false );
    gen.setModel( refmodel_ );
    gen.setWavelet( &wvlt, OD::UsePtr );
    gen.setOutSampling( disprg_ );

    if ( !gen.doWork() )
	mErrRet( gen.errMsg() )

    data_.synthtrc_ = *new SeisTrc( gen.result() );

    return true;
}


bool DataPlayer::extractSeismics()
{
    Well::SimpleTrackSampler wtextr( wd_->track(), d2t_, true, false);
    wtextr.setSampling( disprg_ );
    TaskRunner::execute( data_.trunner_, wtextr );

    const IOObj& ioobj = *IOM().get( seisid_ );
    IOObj* seisobj = ioobj.clone();

    SeismicExtractor seisextr( *seisobj );
    if ( linekey_ )
	seisextr.setLineKey( linekey_ );
    TypeSet<BinID> bids;  wtextr.getBIDs( bids );
    seisextr.setBIDValues( bids );
    seisextr.setInterval( disprg_ );

    const bool success = TaskRunner::execute( data_.trunner_, seisextr );
    data_.seistrc_ = SeisTrc( seisextr.result() );
    BufferString msg;
    if ( !success )
    { 
	msg += "Can not extract seismic: "; 
	msg += seisextr.errMsg(); 
	mErrRet( msg ); 
    }
    return success;
}


bool DataPlayer::copyDataToLogSet()
{
    if ( aimodel_.isEmpty() ) 
	mErrRet( "No data found" )

    TypeSet<float> dahlog, son, den, ai, synth, refs;
    for ( int idx=0; idx<dispsz_; idx++ )
    {
	const int workidx = idx*cDefTimeResampFac;
	const float dah = d2t_->getDah( workrg_.atIndex(workidx) );

	if ( !data_.dahrg_.includes( dah, true ) )
	    continue;

	dahlog += dah;
	refs += reflvals_.validIdx(idx) ? reflvals_[idx] : mUdf(float);
	synth += data_.synthtrc_.size() > idx ? data_.synthtrc_.get(idx,0) 
					      : mUdf(float);

	const AILayer& layer = aimodel_[workidx];
	son += layer.vel_;
	den += layer.den_;
	ai += layer.getAI();
    }
    createLog( data_.sonic(), dahlog.arr(), son.arr(), son.size() ); 
    createLog( data_.density(), dahlog.arr(), den.arr(), den.size() ); 
    createLog( data_.ai(), dahlog.arr(), ai.arr(), ai.size() );
    createLog( data_.reflectivity(), dahlog.arr(), refs.arr(), refs.size()  );
    createLog( data_.synthetic(), dahlog.arr(), synth.arr(), synth.size()  );

    if ( data_.isSonic() )
    {
	Well::Log* vellog = data_.logset_.getLog( data_.sonic() );
	if ( vellog )
	{ 
	    vellog->setUnitMeasLabel( UnitFactors::getStdVelLabel() );
	    GeoCalculator gc; 
	    gc.son2Vel( *vellog, false ); 
	}
    }
    return true;
}


void DataPlayer::createLog( const char* nm, float* dah, float* vals, int sz )
{
    Well::Log* log = 0;
    if ( data_.logset_.indexOf( nm ) < 0 ) 
    {
	log = new Well::Log( nm );
	data_.logset_.add( log );
    }
    else
	log = data_.logset_.getLog( nm );

    log->setEmpty();

    for( int idx=0; idx<sz; idx ++)
	log->addValue( dah[idx], vals[idx] );
}


#define mDelAndReturn(yn) { delete [] seisarr;  delete [] syntharr; return yn;}
bool DataPlayer::computeAdditionalInfo( const Interval<float>& zrg )  
{
    const float step = disprg_.step;
    const int nrsamps = mNINT32( zrg.width()/step )+1;
    const int istart = mNINT32( zrg.start/step );

    if ( data_.seistrc_.size() < nrsamps || data_.synthtrc_.size() < nrsamps )
	{ errmsg_ = "No valid synthetic or seismic trace"; return false; }

    if ( nrsamps <= 1 )
	{ errmsg_ = "Invalid time or depth range specified"; return false; }

    Data::CorrelData& cd = data_.correl_;
    cd.vals_.erase();
    cd.vals_.setSize( nrsamps, 0 ); 

    mDeclareAndTryAlloc( float*, seisarr, float[nrsamps] );
    mDeclareAndTryAlloc( float*, syntharr, float[nrsamps] );

    for ( int idx=0; idx<nrsamps; idx++ )
    {
	if ( idx+istart >= data_.synthtrc_.size() ) break;
	syntharr[idx] = data_.synthtrc_.get( idx + istart, 0 );
	seisarr[idx] = data_.seistrc_.get( idx + istart, 0 );
    }
    GeoCalculator gc;
    cd.coeff_ = gc.crossCorr( seisarr, syntharr, cd.vals_.arr(), nrsamps );
    if ( data_.isinitwvltactive_ )
    {
	int wvltsz = data_.estimatedwvlt_.size();
	wvltsz += wvltsz%2 ? 0 : 1;
	data_.estimatedwvlt_.reSize( wvltsz );
	if ( data_.timeintv_.nrSteps() < wvltsz )
	{
	    errmsg_ = "Seismic trace shorter than wavelet";
	    mDelAndReturn(false)
	}

	const Well::Log* log = data_.logset_.getLog( data_.reflectivity() );
	if ( !log )
	{
	    errmsg_ = "No reflectivity to estimate wavelet";
	    mDelAndReturn(false);
	}

	mDeclareAndTryAlloc( float*, refarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltarr, float[nrsamps] );
	mDeclareAndTryAlloc( float*, wvltshiftedarr, float[wvltsz] );

	mGetWD()

	for ( int idx=0; idx<nrsamps; idx++ )
	    refarr[idx]= log->getValue(d2t_->getDah(zrg.atIndex(idx,step)),true);

	gc.deconvolve( seisarr, refarr, wvltarr, nrsamps );
	for ( int idx=0; idx<wvltsz; idx++ )
	    wvltshiftedarr[idx] = wvltarr[(nrsamps-wvltsz)/2 + idx];

	Array1DImpl<float> wvltvals( wvltsz );
	memcpy( wvltvals.getData(), wvltshiftedarr, wvltsz*sizeof(float) );
	ArrayNDWindow window( Array1DInfoImpl(wvltsz), false, "CosTaper", .05 );
	window.apply( &wvltvals );
	memcpy( data_.estimatedwvlt_.samples(),
	wvltvals.getData(), wvltsz*sizeof(float) );
	delete [] wvltarr;      delete [] wvltshiftedarr;  delete [] refarr;
    }
    mDelAndReturn(true)
}

}; //namespace WellTie
