#ifndef hilbertattrib_h
#define hilbertattrib_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: hilbertattrib.h,v 1.14 2008-11-28 13:10:50 cvsjaap Exp $
________________________________________________________________________

-*/

#include "attribprovider.h"

namespace Attrib
{

/*!\brief Hilbert attribute.

  Calculates Hilbert transform
*/


class Hilbert : public Provider
{
public:
    static void			initClass();
				Hilbert(Desc&);

    static const char*		attribName()	{ return "Hilbert"; }
    static const char*		halflenStr()	{ return "halflen"; }

protected:
    				~Hilbert()      { delete [] hilbfilter_; }

    static Provider*		createInstance(Desc&);

    bool			getInputOutput(int inp,TypeSet<int>& res) const;
    bool			getInputData(const BinID&, int);
    bool			computeData(const DataHolder&,const BinID& pos,
					    int z0,int nrsamples,
					    int threadid) const;

    bool			allowParallelComputation() const
    				{ return false; }
    const Interval<int>*        desZSampMargin(int input,int output) const
				{ return &zmargin_; }

    const DataHolder*		inputdata_;
    int				dataidx_;

    Interval<int>		zmargin_;
    int				halflen_;

    int				hilbfilterlen_;
    const float*		hilbfilter_;

    static float*		makeHilbFilt(int);
};

}; // namespace Attrib

#endif
