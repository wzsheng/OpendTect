#ifndef tableposprovider_h
#define tableposprovider_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: tableposprovider.h,v 1.3 2008-02-13 13:28:00 cvsbert Exp $
________________________________________________________________________


-*/

#include "posprovider.h"
#include "binidvalset.h"
#include "bufstring.h"
class IOObj;

namespace Pos
{

/*!\brief Provider based on BinIDValueSet table */

class TableProvider3D : public Provider3D
{
public:

			TableProvider3D()
			: bvs_(1,true)	{}
			TableProvider3D(const IOObj& psioobj);
			TableProvider3D(const char* filenm);
			TableProvider3D( const TableProvider3D& tp )
			: bvs_(1,true)	{ *this = tp; }
    TableProvider3D&	operator =(const TableProvider3D&);
    const char*		type() const;	//!< sKey::Table
    TableProvider3D*	clone() const	{ return new TableProvider3D(*this); }

    virtual void	reset()		{ pos_.reset(); }

    virtual bool	toNextPos()	{ return bvs_.next(pos_,true); }
    virtual bool	toNextZ()	{ return bvs_.next(pos_,false); }

    virtual BinID	curBinID() const { return bvs_.getBinID(pos_); }
    virtual float	curZ() const	{ return *bvs_.getVals(pos_); }
    virtual bool	includes(const BinID&,float) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    void		getExtent(BinID&,BinID&) const;
    void		getZRange(Interval<float>&) const;
    int			estNrPos() const	{ return bvs_.totalSize(); }
    int			estNrZPerPos() const	{ return 1; }

    BinIDValueSet&	binidValueSet()		{ return bvs_; }
    const BinIDValueSet& binidValueSet() const	{ return bvs_; }

    static void		getBVSFromPar(const IOPar&,BinIDValueSet&);

protected:

    BinIDValueSet	bvs_;
    BinIDValueSet::Pos	pos_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new TableProvider3D; }

};


} // namespace

#endif
