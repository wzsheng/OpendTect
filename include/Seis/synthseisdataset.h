#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno / Bert
 Date:		July 2013 / Sep 2018
________________________________________________________________________

-*/

#include "synthseisgenparams.h"
#include "datapack.h"
#include "integerid.h"
#include "reflectivitymodel.h"
#include "timedepthmodel.h"

class PropertyRef;
class RayTracer1D;
class SeisTrc;
class SeisTrcBufDataPack;
namespace ColTab { class MapperSetup; }


namespace SynthSeis
{


mExpClass(Seis) DispPars
{
public:

    typedef ColTab::MapperSetup	MapperSetup;

			DispPars();

    BufferString	colseqname_;
    RefMan<MapperSetup>	vdmapsetup_;
    RefMan<MapperSetup>	wvamapsetup_;
    float		overlap_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};



mExpClass(Seis) RayModel
{
public:

    typedef RefMan<ReflectivityModelSet>	RflMdlSetRef;
    typedef ConstRefMan<ReflectivityModelSet>	ConstRflMdlSetRef;

			RayModel(const RayTracer1D& rt1d,
				 int nroffsets);
			RayModel(const RayModel&);
			~RayModel();

    void		forceReflTimes(const StepInterval<float>&);
    void		getTraces(ObjectSet<SeisTrc>&,bool steal);
    void		getD2T(ObjectSet<TimeDepthModel>&,bool steal);
    const TimeDepthModel& zeroOffsetD2T() const;
    RflMdlSetRef	reflModels(bool sampled=false);
    ConstRflMdlSetRef	reflModels(bool sampled=false) const;
    SeisTrc*		stackedTrc() const;

    ObjectSet<SeisTrc>	outtrcs_;
    ObjectSet<TimeDepthModel> t2dmodels_;
    TimeDepthModel*	zerooffsett2dmodel_;
    RflMdlSetRef	reflmodels_;
    RflMdlSetRef	sampledreflmodels_;

};


/*! brief the basic synthetic dataset. contains the data cubes */

mExpClass(Seis) DataSet : public ::RefCount::Referenced
			, public ::NamedObject
{
public:

    typedef IntegerID<int>			MgrID;
    typedef SynthSeis::SyntheticType		SynthType;
    typedef RefMan<ReflectivityModelSet>	RflMdlSetRef;
    typedef ConstRefMan<ReflectivityModelSet>	ConstRflMdlSetRef;
    typedef TimeDepthModelSet			D2TModelSet;
    typedef ObjectSet<RayModel>			RayModelSet;
    typedef TimeDepthModelSet::size_type	size_type;
    typedef TimeDepthModelSet::idx_type		idx_type;

    MgrID		id() const		{ return id_; }
    virtual SynthType	synthType() const	{ return genpars_.type_; }
    virtual bool	isPS() const		{ return true; }
    virtual bool	hasOffset() const	{ return false; }

    const GenParams&	genParams() const	{ return genpars_; }
    DataPack&		dataPack()		{ return *datapack_; }
    const DataPack&	dataPack() const	{ return *datapack_; }
    DispPars&		dispPars()		{ return disppars_; }
    const DispPars&	dispPars() const	{ return disppars_; }

    bool		isEmpty() const		{ return size() < 1; }
    size_type		size() const;
    const SeisTrc*	getTrace(idx_type) const;
    ZSampling		zRange() const;

    float		getTime(float dpt,int seqnr) const;
    float		getDepth(float time,int seqnr) const;
    const D2TModelSet&	d2TModels() const	{ return finald2tmodels_; }
    ConstRflMdlSetRef	reflModels(int modelid,bool sampled) const;
    const IOPar&	rayPars() const		{ return raypars_; }
    const RayModelSet&	rayModels() const	{ return raymodels_; }
    DataPack::FullID	dataPackID() const;

    void		useDispPars(const IOPar&);
    void		fillDispPars(IOPar&) const;

    DBKey		waveletID() const	{ return genpars_.wvltid_; }
    BufferString	waveletName() const	{ return nameOf(waveletID()); }

protected:

			DataSet(const GenParams&,DataPack&);
			DataSet(const DataSet&);
			~DataSet();
    DataSet&		operator =(const DataSet&)	= delete;

    MgrID		id_;
    const GenParams	genpars_;
    IOPar		raypars_;
    DispPars		disppars_;

    D2TModelSet		finald2tmodels_;
    ObjectSet<RayModel>	raymodels_;
    RefMan<DataPack>	datapack_;

    bool		validIdx( idx_type idx ) const
			{ return finald2tmodels_.validIdx( idx ); }

    virtual const SeisTrc*	gtTrace(idx_type) const		= 0;
    virtual DataPackMgr::ID	dpMgrID() const			= 0;

public:

			// just don't
    void		setName(const char*);

			// Use if you are a ray tracer
    void		setRayModels(const RayModelSet&);
    RayModelSet&	rayMdls()		{ return raymodels_; }
    void		updateD2TModels();

			// For models directly from RayModel
    void		adjustD2TModelsToSRD(D2TModelSet&);

			// Used for obtaining an ID by a managing object
    static MgrID	getNewID();

			// Used by managing object
    void		setID( MgrID newid )	{ id_ = newid; }
    void		getD2TFrom(const DataSet&);

};



mExpClass(Seis) PostStackDataSet : public DataSet
{
public:

    typedef SeisTrcBufDataPack	DPType;

			PostStackDataSet(const GenParams&,DPType&);
			~PostStackDataSet();

    virtual bool	isPS() const		{ return false; }

    DPType&		postStackPack();
    const DPType&	postStackPack() const;

protected:

    virtual const SeisTrc*	gtTrace(idx_type) const;
    virtual DataPackMgr::ID	dpMgrID() const;

};


mExpClass(Seis) StratPropDataSet : public PostStackDataSet
{
public:

			StratPropDataSet(const GenParams&,DPType&,
					 const PropertyRef&);

    const PropertyRef&	propRef() const		{ return prop_; }

protected:

    const PropertyRef&		prop_;

};


mExpClass(Seis) PSBasedPostStackDataSet : public PostStackDataSet
{
public:

			PSBasedPostStackDataSet(const GenParams&,DPType&);

};


mExpClass(Seis) AVOGradDataSet : public PSBasedPostStackDataSet
{
public:

			AVOGradDataSet( const GenParams& gp, DPType& dp )
			    : PSBasedPostStackDataSet(gp,dp) {}

};


mExpClass(Seis) AngleStackDataSet : public PSBasedPostStackDataSet
{
public:

			AngleStackDataSet(const GenParams& gp, DPType& dp )
			    : PSBasedPostStackDataSet(gp,dp) {}

};


} // namespace SynthSeis
