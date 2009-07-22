#ifndef zaxistransformer_h
#define zaxistransformer_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2007
 RCS:           $Id: zaxistransformer.h,v 1.8 2009-07-22 16:01:13 cvsbert Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "cubesampling.h"

class TaskRunner;
class ZAxisTransform;

template <class T> class Array3D;

/*!Transforms an Array3D with a zaxistransform. It is assumed that the first
   dimension in the array is inline, the second is crossline and that the third
   is z.
*/


mClass ZAxisTransformer : public ParallelTask
{
public:
    			ZAxisTransformer(ZAxisTransform&,bool forward = true);
			~ZAxisTransformer();
    void		setInterpolate(bool yn);
    bool		getInterpolate() const;
    bool		setInput(const Array3D<float>&,const CubeSampling&);
    void		setOutputRange(const CubeSampling&);
    const CubeSampling&	getOutputRange() const	{ return outputcs_; }
    Array3D<float>*	getOutput(bool transfer);
    			/*!<\param transfer specifies whether the caller will
			                    take over the array.  */
    bool		loadTransformData(TaskRunner* =0);

protected:
    bool		doPrepare(int);
    od_int64		nrIterations() const;
    bool		doWork( od_int64, od_int64, int );

    ZAxisTransform&		transform_;
    int				voiid_;
    bool			forward_;
    bool			interpolate_;

    const Array3D<float>*	input_;
    CubeSampling		inputcs_;

    Array3D<float>*		output_;
    CubeSampling		outputcs_;
};



#endif
