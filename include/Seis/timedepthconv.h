#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
________________________________________________________________________

*/


#include "seismod.h"
#include "zaxistransform.h"
#include "uistring.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "multidimstorage.h"
#include "veldesc.h"


class IOObj;
class SeisTrc;
class SeisTrcReader;
template <class T> class Array3D;
template <class T> class ValueSeries;


mExpClass(Seis) VelocityStretcher : public ZAxisTransform
{ mODTextTranslationClass(VelocityStretcher);
public:
    virtual bool		setVelData(const MultiID&)		= 0;

    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

    static const char*		sKeyTopVavg()	{ return "Top Vavg"; }
    static const char*		sKeyBotVavg()	{ return "Bottom Vavg"; }

protected:
				VelocityStretcher(const ZDomain::Def& from,
						  const ZDomain::Def& to);
};


/*!ZAxisstretcher that converts from time to depth (or back) using a
   velocity model on disk. */

mExpClass(Seis) Time2DepthStretcher : public VelocityStretcher
{ mODTextTranslationClass(Time2DepthStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Time2DepthStretcher,
				  "VelocityT2D",
				  toUiString(sFactoryKeyword()));

			Time2DepthStretcher();
    bool		setVelData(const MultiID&) override;
    bool		isOK() const override;

    bool		needsVolumeOfInterest() const override	{ return true; }
    int			addVolumeOfInterest(const TrcKeyZSampling&,
					    bool) override;
    void		setVolumeOfInterest(int,
					const TrcKeyZSampling&,bool) override;
    void		removeVolumeOfInterest(int) override;
    bool		loadDataIfMissing(int,TaskRunner* =0) override;
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				  int,float*) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int,float*) const override;
    Interval<float>	getZInterval(bool from) const override;
    float		getGoodZStep() const override;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    MultiID		getZDomainID() const;

    const Interval<float>& getVavgRg(bool start) const;
    static Interval<float> getDefaultVAvg();

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
    friend		class TimeDepthDataLoader;
			~Time2DepthStretcher();
    void		releaseData();
    Interval<float>	getTimeInterval(const BinID&,int voiidx) const;
    Interval<float>	getDepthInterval(const BinID&,int voiidx) const;

    static void				udfFill(ValueSeries<float>&,int);

    ObjectSet<Array3D<float> >		voidata_;
    TypeSet<TrcKeyZSampling>		voivols_;
    BoolTypeSet				voiintime_;
    TypeSet<int>			voiids_;

    SeisTrcReader*			velreader_;
    VelocityDesc			veldesc_;
    bool				velintime_;

    Interval<float>			topvavg_; //Used to compute ranges
    Interval<float>			botvavg_; //Used to compute ranges
};


/*!ZAxisstretcher that converts from depth to time (or back). Uses
   an Time2Depth converter to do the job. */


mExpClass(Seis) Depth2TimeStretcher : public VelocityStretcher
{ mODTextTranslationClass(Depth2TimeStretcher);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, Depth2TimeStretcher,
				  "VelocityD2T",
				  toUiString(sFactoryKeyword()));

			Depth2TimeStretcher();
    bool		setVelData(const MultiID&) override;
    bool		isOK() const override;

    bool		needsVolumeOfInterest() const override;
    int			addVolumeOfInterest(const TrcKeyZSampling&,
					    bool) override;
    void		setVolumeOfInterest(int,
					const TrcKeyZSampling&,bool) override;
    void		removeVolumeOfInterest(int) override;
    bool		loadDataIfMissing(int,TaskRunner* =0) override;
    void		transformTrc(const TrcKey&,const SamplingData<float>&,
				     int sz,float*) const override;
    void		transformTrcBack(const TrcKey&,
					 const SamplingData<float>&,
					 int sz,float*) const override;
    Interval<float>	getZInterval(bool from) const override;
    float		getGoodZStep() const override;
    const char*		getToZDomainString() const;
    const char*		getFromZDomainString() const;
    MultiID		getZDomainID() const;

    void		fillPar(IOPar&) const override;
    bool		usePar(const IOPar&) override;

protected:
			~Depth2TimeStretcher()				{}

    RefMan<Time2DepthStretcher>		stretcher_;
};

/*! Scans a velocity model for minimum top/bottom average velocity. */

mExpClass(Seis) VelocityModelScanner : public SequentialTask
{ mODTextTranslationClass(VelocityModelScanner);
public:
			VelocityModelScanner(const IOObj&,
				const VelocityDesc&);
			~VelocityModelScanner();

    uiString		uiMessage() const override	{ return msg_; }
    od_int64		totalNr() const override { return subsel_.totalNr(); }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiNrDoneText() const override
			{ return tr("Position scanned"); }

    const Interval<float>&	getTopVAvg() const	{ return startavgvel_; }
    const Interval<float>&	getBotVAvg() const	{ return stopavgvel_; }

    int				nextStep() override;

protected:

    uiString			msg_;
    TrcKeySampling		subsel_;
    TrcKeySamplingIterator	hsiter_;
    bool			definedv0_;
    bool			definedv1_;
    bool			zistime_;
    int				nrdone_;

    const IOObj&		obj_;
    const VelocityDesc&		vd_;

    SeisTrcReader*		reader_;

    Interval<float>		startavgvel_;
    Interval<float>		stopavgvel_;
};


mExpClass(Seis) LinearVelTransform : public ZAxisTransform
{ mODTextTranslationClass(LinearVelTransform);
public:
    bool			usePar(const IOPar&) override;
    void			fillPar(IOPar&) const override;

    bool			canTransformSurv(OD::GeomSystem) const override
				{ return true; }

protected:
				LinearVelTransform(const ZDomain::Def& from,
						   const ZDomain::Def& to,
						   float v0, float dv);
    void			transformT2D(const SamplingData<float>&,
					     int sz,float* res) const;
    void			transformD2T(const SamplingData<float>&,
					     int sz,float* res) const;

    float			startvel_;
    float			dv_;

};


mExpClass(Seis) LinearT2DTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearT2DTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearT2DTransform,
				  "LinearT2D", tr("Linear velocity") );

				LinearT2DTransform(float v0=0, float dv=0);

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;

    Interval<float>		getZInterval(bool time) const override;
    float			getGoodZStep() const override;

    bool			needsVolumeOfInterest() const override
				{ return false; }
};


mExpClass(Seis) LinearD2TTransform : public LinearVelTransform
{ mODTextTranslationClass(LinearD2TTransform);
public:
    mDefaultFactoryInstantiation( ZAxisTransform, LinearD2TTransform,
				  "LinearD2T", tr("Linear velocity") );

				LinearD2TTransform(float v0=0, float dv=0);

    void			transformTrc(const TrcKey&,
					  const SamplingData<float>&,
					  int sz,float* res) const override;
    void			transformTrcBack(const TrcKey&,
					      const SamplingData<float>&,
					      int sz,float* res) const override;
    Interval<float>		getZInterval(bool depth) const override;
    float			getGoodZStep() const override;

    bool			needsVolumeOfInterest() const override
				{ return false; }
};

