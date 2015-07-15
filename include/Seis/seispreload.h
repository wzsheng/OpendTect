#ifndef seispreload_h
#define seispreload_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2009
 RCS:           $Id$
________________________________________________________________________


-*/

#include "seismod.h"
#include "bufstringset.h"
#include "datachar.h"
#include "datapackbase.h"
#include "multiid.h"
#include "ranges.h"
#include "task.h"

class IOObj;
class Scaler;

namespace Seis
{

mExpClass(Seis) PreLoader
{ mODTextTranslationClass(PreLoader);
public:
			PreLoader(const MultiID&,TaskRunner* tr=0);

    void		setID( const MultiID& ky )	{ id_ = ky; }
    const MultiID&	id() const			{ return id_; }
    void		setTaskRunner( TaskRunner& t )	{ tr_ = &t; }

    IOObj*		getIOObj() const;
    Interval<int>	inlRange() const;
    			//!< PS 3D only. If nothing there: ret.start==mUdf(int)
    void		getLineNames(BufferStringSet&) const;
    			//!< Line 2D only.

    bool		loadVol(const TrcKeyZSampling&,
				DataCharacteristics::UserType=
					DataCharacteristics::Auto,
				Scaler* =0) const;
    bool		loadLine(Pos::GeomID,const TrcKeyZSampling&) const;
    bool		loadPS3D(const Interval<int>* inlrg=0) const;
    bool		loadPS2D(const char* lnm=0) const;	//!< null => all
    bool		loadPS2D(const BufferStringSet&) const;

    void		unLoad() const;
    uiString		errMsg() const			{ return errmsg_; }

    static void		load(const IOPar&,TaskRunner* tr=0);
    			//!< Seis.N.[loadObj_fmt]
    static void		loadObj(const IOPar&,TaskRunner* tr=0);
    			//!< sKey::ID() and optional subselections
    void		fillPar(IOPar&) const;

    static const char*	sKeyLines();

protected:

    MultiID		id_;
    TaskRunner*		tr_;
    TaskRunner		deftr_;
    mutable uiString	errmsg_;

    TaskRunner&		getTr() const
    			{ return *((TaskRunner*)(tr_ ? tr_ : &deftr_)); }
};



mExpClass(Seis) PreLoadDataManager
{
public:
    void		add(const MultiID&,DataPack*);
    void		remove(const MultiID&);
    void		remove(int dpid);
    void		removeAll();

    DataPack*		get(const MultiID&);
    DataPack*		get(int dpid);
    const DataPack*	get(const MultiID&) const;
    const DataPack*	get(int dpid) const;
    void		getInfo(const MultiID&,BufferString&) const;

    void		getIDs(TypeSet<MultiID>&) const;
    bool		isPresent(const MultiID&) const;

protected:

    DataPackMgr&	dpmgr_;
    TypeSet<MultiID>	mids_;
    TypeSet<Pos::GeomID> geomids_;
    TypeSet<int>	dpids_;

public:
			PreLoadDataManager();
			~PreLoadDataManager();
};

mGlobal(Seis) PreLoadDataManager& PLDM();

} // namespace Seis

#endif
