#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2004
________________________________________________________________________

-*/

#include "generalmod.h"
#include "posidxpairvalset.h"
#include "binid.h"
#include "bin2d.h"


/*!\brief A Pos::IdxPairValueSet with BinIDs or Bin2Ds */


mExpClass(General) BinIDValueSet : public Pos::IdxPairValueSet
{
public:

    mUseType( OD,	GeomSystem );
    mUseType( Pos,	GeomID );

			BinIDValueSet(int nrvals,bool allowdup,
				      GeomSystem gs=OD::VolBasedGeom)
			    : Pos::IdxPairValueSet(nrvals,allowdup)
			    , geomsystem_(gs)				{}
			BinIDValueSet(const BinIDValueSet& bvs)
			    : Pos::IdxPairValueSet(bvs)
			    , geomsystem_(bvs.geomsystem_)		{}
    BinIDValueSet&	operator =( const BinIDValueSet& oth )
			{
			    Pos::IdxPairValueSet::operator=(oth);
			    geomsystem_ = oth.geomsystem_;
			    return *this;
			}

    inline void		allowDuplicatePositions( bool yn )
					{ allowDuplicateIdxPairs(yn); }
    inline bool		hasDuplicatePositions() const
					{ return hasDuplicateIdxPairs(); }
    inline bool		nrDuplicatePositions() const
					{ return nrDuplicateIdxPairs(); }

    inline BinID	getBinID( const SPos& spos ) const
					{ return mkBinID(getIdxPair(spos)); }
    inline BinID	firstBinID() const
					{ return mkBinID(firstIdxPair()); }
    inline Bin2D	getBin2D( const SPos& spos ) const
					{ return mkBin2D(getIdxPair(spos)); }
    inline Bin2D	firstBin2D() const
					{ return mkBin2D(firstIdxPair()); }

    inline GeomSystem	geomSystem() const
			{ return geomsystem_; }
    inline bool		is2D() const
			{ return geomsystem_ == OD::LineBasedGeom; }
    inline bool		is3D() const
			{ return geomsystem_ == OD::VolBasedGeom; }
    inline bool		isSynthetic() const
			{ return geomsystem_ == OD::SynthGeom; }
    inline void		setGeomSystem( GeomSystem gs )
			{ geomsystem_ = gs; }
    inline void		setIs2D( bool yn )
			{ geomsystem_ = yn ? OD::LineBasedGeom
					   : OD::VolBasedGeom; }

protected:

    friend class	DataPointSet;
    friend class	PosVecDataSet;

    GeomSystem		geomsystem_;

    inline static BinID	mkBinID( const Pos::IdxPair& ip )
			{ return BinID( ip.first, ip.second ); }
    inline static Bin2D	mkBin2D( const Pos::IdxPair& ip )
			{ return Bin2D( GeomID(ip.first), ip.second ); }

public:

    mDeprecated void	allowDuplicateBinIDs( bool yn )
			{ allowDuplicatePositions( yn ); }

};
