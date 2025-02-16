#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki/Bruno
 Date:		July 2013
________________________________________________________________________

-*/

#include "wellattribmod.h"

#include "datapack.h"
#include "flatview.h"
#include "sharedobject.h"
#include "stratsynthgenparams.h"
#include "synthseis.h"


mExpClass(WellAttrib) SynthFVSpecificDispPars
{
public:
			SynthFVSpecificDispPars();

    ColTab::MapperSetup	vdmapper_;
    ColTab::MapperSetup	wvamapper_;
    BufferString	ctab_;
    float		overlap_ = 1.f;
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);
};


/*! brief the basic synthetic dataset. contains the data cubes*/
mExpClass(WellAttrib) SyntheticData : public SharedObject
{
public:

    typedef int SynthID;

    static ConstRefMan<SyntheticData>	get(const SynthGenParams&,
					    Seis::RaySynthGenerator&);

    virtual void			setName(const char*) override;

    bool				isOK() const;
    virtual const SeisTrc*		getTrace(int seqnr) const	= 0;
    virtual int				nrPositions() const		= 0;

    float				getTime(float dpt,int seqnr) const;
    float				getDepth(float time,int seqnr) const;

    const DataPack&			getPack() const {return datapack_;}
    DataPack&				getPack()	{return datapack_;}

    const Seis::SynthGenDataPack&	synthGenDP() const;
    const ReflectivityModelSet&		getRefModels() const;
    const ReflectivityModelBase*	getRefModel(int imdl) const;
    const TimeDepthModel*		getTDModel(int imdl) const;
    const TimeDepthModel*		getTDModel(int imdl,int ioff) const;

    DataPack::FullID			datapackid_;

    SynthID				id_ = -1;
    virtual bool			isPS() const		= 0;
    virtual bool			hasOffset() const	= 0;
    virtual bool			isAngleStack() const  { return false; }
    virtual bool			isAVOGradient() const { return false; }
    virtual bool			isStratProp() const   { return false; }
    virtual bool			isAttribute() const   { return false; }
    virtual SynthGenParams::SynthType	synthType() const	= 0;

    const SynthGenParams&		getGenParams() const	{ return sgp_; }
    void				useGenParams(const SynthGenParams&);
    void				useDispPar(const IOPar&);
    void				fillDispPar(IOPar&) const;
    const char*				waveletName() const;
    SynthFVSpecificDispPars&		dispPars()	{ return disppars_; }
    const SynthFVSpecificDispPars&	dispPars() const
							{ return disppars_; }

protected:
					SyntheticData(const SynthGenParams&,
						  const Seis::SynthGenDataPack&,
						  DataPack&);
					~SyntheticData();

    SynthFVSpecificDispPars		disppars_;

    void				removePack();

    SynthGenParams			sgp_;
    DataPack&				datapack_;
    ConstRefMan<Seis::SynthGenDataPack> synthgendp_;
};


