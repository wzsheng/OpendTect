/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Y. Liu
 * DATE     : January 2010
-*/


#include "prestackanglemute.h"

#include "arrayndslice.h"
#include "ioman.h"
#include "muter.h"
#include "prestackanglecomputer.h"
#include "prestackgather.h"
#include "prestackmute.h"
#include "raytracerrunner.h"
#include "unitofmeasure.h"


namespace PreStack
{

AngleCompParams::AngleCompParams()
    : anglerange_(0,30)
    , velvolmid_(MultiID::udf())
{
    smoothingpar_.set( AngleComputer::sKeySmoothType(),
		       AngleComputer::FFTFilter );
    smoothingpar_.set( AngleComputer::sKeyFreqF3(), 10.0f );
    smoothingpar_.set( AngleComputer::sKeyFreqF4(), 15.0f );
    // defaults for other types:
    smoothingpar_.set( AngleComputer::sKeyWinFunc(), HanningWindow::sName() );
    smoothingpar_.set( AngleComputer::sKeyWinLen(),
		       100.0f/SI().zDomain().userFactor() );
    smoothingpar_.set( AngleComputer::sKeyWinParam(), 0.95f );

    raypar_.set( sKey::Type(), RayTracer1D::factory().getDefaultName() );
    const StepInterval<float>& offsrange = RayTracer1D::sDefOffsetRange();
    TypeSet<float> offsetvals;
    for ( int idx=0; idx<=offsrange.nrSteps(); idx++ )
	offsetvals += offsrange.atIndex( idx );
    raypar_.set( RayTracer1D::sKeyOffset(), offsetvals );
    raypar_.setYN( RayTracer1D::sKeyOffsetInFeet(), SI().xyInFeet() );
    raypar_.setYN( RayTracer1D::sKeyReflectivity(), false );

    uiString msg;
    PtrMan<RayTracer1D> rt1d = RayTracer1D::createInstance( raypar_, msg );
    if ( !rt1d )
	return;

    rt1d->fillPar( raypar_ );
}


AngleMuteBase::AngleMuteBase()
{
    velsource_ = new Vel::VolumeFunctionSource();
}


AngleMuteBase::~AngleMuteBase()
{
    delete params_;
    deepErase( rtrunners_ );
}


void AngleMuteBase::fillPar( IOPar& par ) const
{
    PtrMan<IOObj> ioobj = IOM().get( params_->velvolmid_ );
    if ( ioobj ) par.set( sKeyVelVolumeID(), params_->velvolmid_ );

    par.merge( params_->raypar_ );
    par.set( sKeyMuteCutoff(), params_->mutecutoff_ );
}


bool AngleMuteBase::usePar( const IOPar& par  )
{
    params_->raypar_.merge( par );
    par.get( sKeyVelVolumeID(), params_->velvolmid_ );
    par.get( sKeyMuteCutoff(), params_->mutecutoff_ );

    return true;
}


bool AngleMuteBase::setVelocityFunction()
{
    const MultiID& mid = params_->velvolmid_;
    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( !ioobj ) return false;

    velsource_->setFrom( mid );
    return true;
}



bool AngleMuteBase::getLayers( const BinID& bid, ElasticModel& model,
			       SamplingData<float>& sd, int resamplesz )
{
    TypeSet<float> vels;
    RefMan<Vel::VolumeFunction> velfun = velsource_->createFunction( bid );
    if ( !velfun || !velsource_->getVel(bid,sd,vels) )
	return false;

    const int velsz = vels.size();
    if ( resamplesz > velsz )
    {
	for ( int idy=velsz; idy<resamplesz; idy++ )
	{
	    const float vel = velfun->getVelocity( sd.atIndex(idy) );
	    if ( mIsUdf(vel) )
		continue;

	    vels += vel;
	}
    }
    int nrlayers = vels.size();

    const StepInterval<float> zrg = velfun->getAvailableZ();
    TypeSet<float> depths;
    depths.setSize( nrlayers, 0 );

    if ( velsource_->zIsTime() )
    {
	 ArrayValueSeries<float,float> velvals( vels.arr(), false, nrlayers );
	 ArrayValueSeries<float,float> depthvals(depths.arr(), false, nrlayers);
	 const VelocityDesc& veldesc = velfun->getDesc();
	 TimeDepthConverter tdc;
	 tdc.setVelocityModel( velvals, nrlayers, sd, veldesc, true,
				&UnitOfMeasure::surveyDefVelUnit()->scaler() );
	 if ( !tdc.calcDepths(depthvals,nrlayers,sd) )
	     return false;

    }
    else
    {
	for ( int il=0; il<nrlayers; il++ )
	    depths[il] = zrg.atIndex( il );
    }

    int il = 1;
    for ( il=1; il<nrlayers; il++ )
	model += ElasticLayer(depths[il]-depths[il-1],
			vels[il], mUdf(float), mUdf(float) );
    model += ElasticLayer(depths[il-1]-depths[il-2],
			vels[il-1], mUdf(float), mUdf(float) );
    block( model );

    sd = zrg;
    return !model.isEmpty();
}


float AngleMuteBase::getOffsetMuteLayer( const ReflectivityModelBase& refmodel,
					 int nrlayers, int ioff, bool tail,
					 int startlayer,bool belowcutoff ) const
{
    float mutelayer = mUdf(float);
    const float cutoffsin = (float) sin( params_->mutecutoff_ * M_PI / 180 );
    if ( tail )
    {
	float prevsin = mUdf(float);
	int previdx = -1;
	for ( int il=startlayer; il<nrlayers; il++ )
	{
	    const float sini = refmodel.getSinAngle( ioff, il );
	    if ( mIsUdf(sini) || (mIsZero(sini,1e-8) && il<nrlayers/2) )
		continue; //Ordered down, get rid of edge 0.

	    bool ismuted = ( sini < cutoffsin && belowcutoff ) ||
				( sini > cutoffsin && !belowcutoff );
	    if ( ismuted )
	    {
		if ( previdx != -1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx)*
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = (float)il;
		break;
	    }

	    prevsin = sini;
	    previdx = il;
	}
    }
    else
    {
	float prevsin = mUdf(float);
	int previdx = -1;
	for ( int il=nrlayers-1; il>=startlayer; il-- )
	{
	    const float sini = refmodel.getSinAngle( ioff, il );
	    if ( mIsUdf(sini) )
		continue;

	    bool ismuted = ( sini > cutoffsin && belowcutoff ) ||
				( sini < cutoffsin && !belowcutoff );
	    if ( ismuted )
	    {
		if ( previdx!=-1 && !mIsZero(sini-prevsin,1e-5) )
		{
		    mutelayer = previdx + (il-previdx) *
			(cutoffsin-prevsin)/(sini-prevsin);
		}
		else
		    mutelayer = (float)il;
		break;
	    }

	    prevsin = sini;
	    previdx = il;
	}
    }
    return mutelayer;
}


AngleMute::AngleMute()
    : Processor(sFactoryKeyword())
{
    params_ = new AngleMutePars();
}


AngleMute::~AngleMute()
{
    deepErase( muters_ );
}


Muter* AngleMute::getMuter( int idx )
{
    return muters_.validIdx( idx ) ? muters_.get(idx) : nullptr;
}


bool AngleMute::doPrepare( int nrthreads )
{
    deepErase( rtrunners_ );
    deepErase( muters_ );

    if ( !setVelocityFunction() )
	return false;

    raytraceparallel_ = nrthreads < Threads::getNrProcessors();

    for ( int idx=0; idx<nrthreads; idx++ )
    {
	muters_ += new Muter( params().taperlen_, params().tail_ );
	rtrunners_ += new RayTracerRunner( params().raypar_ );
    }

    return true;
}


bool AngleMute::usePar( const IOPar& par )
{
    if ( !AngleMuteBase::usePar(par) )
	return false;

    par.get( Mute::sTaperLength(), params().taperlen_ );
    par.getYN( Mute::sTailMute(), params().tail_ );

    return true;
}


void AngleMute::fillPar( IOPar& par ) const
{
    AngleMuteBase::fillPar( par );
    par.set( Mute::sTaperLength(), params().taperlen_ );
    par.setYN( Mute::sTailMute(), params().tail_ );
}


bool AngleMute::doWork( od_int64 start, od_int64 stop, int thread )
{
    if ( !rtrunners_.validIdx(thread) )
	return false;

    RayTracerRunner* rtrunner = rtrunners_[thread];

    TypeSet<ElasticModel> emodels;
    ElasticModel layers;
    emodels += layers;
    rtrunner->setModel( emodels );
    for ( int idx=mCast(int,start); idx<=stop; idx++, addToNrDone(1) )
    {
	Gather* output = outputs_[idx];
	const Gather* input = inputs_[idx];
	if ( !output || !input )
	    continue;

	const BinID bid = input->getBinID();

	int nrlayers = input->data().info().getSize( Gather::zDim() );
	layers.setEmpty();
	SamplingData<float> sd;
	if ( !getLayers(bid,layers,sd,nrlayers) )
	    continue;

	const int nrblockedlayers = layers.size();
	TypeSet<float> offsets;
	const int nroffsets = input->size( input->offsetDim()==0 );
	for ( int ioffset=0; ioffset<nroffsets; ioffset++ )
	    offsets += input->getOffset( ioffset );

	rtrunner->setOffsets( offsets );
	if ( !rtrunner->executeParallel(raytraceparallel_) )
	    { errmsg_ = rtrunner->uiMessage(); continue; }

	ConstRefMan<ReflectivityModelSet> refmodels = rtrunner->getRefModels();
	const ReflectivityModelBase* refmodel = refmodels ? refmodels->get(0)
							  : nullptr;
	if ( !refmodel || !refmodel->hasAngles() )
	    return false;

	Array1DSlice<float> trace( output->data() );
	trace.setDimMap( 0, Gather::zDim() );

	for ( int ioffs=0; ioffs<nroffsets; ioffs++ )
	{
	    trace.setPos( Gather::offsetDim(), ioffs );
	    if ( !trace.init() )
		continue;

	    float mutelayer = getOffsetMuteLayer( *refmodel, nrblockedlayers,
						  ioffs, params().tail_ );
	    if ( mIsUdf( mutelayer ) )
		continue;

	    if ( nrlayers != nrblockedlayers )
	    {
		const int muteintlayer = (int)mutelayer;
		if ( input->zIsTime() )
		{
		    float mtime = 0;
		    for ( int il=0; il<=muteintlayer; il++ )
		    {
			mtime += layers[il].thickness_/layers[il].vel_;
			if ( il==muteintlayer )
			{
			    const float diff = mutelayer-muteintlayer;
			    if ( diff>0 )
				mtime += diff*
				    layers[il+1].thickness_/layers[il+1].vel_;
			}
		    }
		    mutelayer = sd.getfIndex( mtime );
		}
		else
		{
		    float depth = 0;
		    for ( int il=0; il<muteintlayer+2; il++ )
		    {
			if ( il >= nrblockedlayers ) break;
			float thk = layers[il].thickness_;
			if ( il == muteintlayer+1 )
			    thk *= ( mutelayer - muteintlayer);

			depth += thk;
		    }
		    mutelayer = sd.getfIndex( depth );
		}
	    }
	    muters_[thread]->mute( trace, nrlayers, mutelayer );
	}
    }

    return true;
}


AngleMute::AngleMutePars& AngleMute::params()
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }


const AngleMute::AngleMutePars& AngleMute::params() const
{ return static_cast<AngleMute::AngleMutePars&>(*params_); }

} // namespace PreStack
