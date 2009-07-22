#ifndef staticsdesc_h
#define staticsdesc_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2009
 RCS:		$Id: staticsdesc.h,v 1.3 2009-07-22 16:01:14 cvsbert Exp $
________________________________________________________________________

*/

#include "multiid.h"

class IOPar;

/*!Specifies Statics as a horizon and either a horizon attribute or 
   a constant velocity. Velocity is always in m/s. */

mClass StaticsDesc
{
public:
			StaticsDesc();

    MultiID		horizon_;
    float		vel_;
    BufferString	velattrib_;	//attrib on statichorizon_
    					//if empty, use vel

    bool		operator==(const StaticsDesc&) const;
    bool		operator!=(const StaticsDesc&) const;

    static void		removePars(IOPar&);
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    static const char*	sKeyHorizon();
    static const char*	sKeyVelocity();
    static const char*	sKeyVelocityAttrib();
};


#endif
