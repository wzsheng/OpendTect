#ifndef SoRGBATextureChannel2RGBA_h
#define SoRGBATextureChannel2RGBA_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoRGBATextureChannel2RGBA.h,v 1.7 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/


#include "SoMFImage.h"
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"

class SoFieldSensor;
class SoSensor;
class SoElement;
class SoState;

/*!
Reads any number of texture channels from the state, merges them with one
colorsequence per channel, and outputs 4 texture channels (RGBA) on the state.
*/


mClass SoRGBATextureChannel2RGBA : public SoNode
{ SO_NODE_HEADER(SoRGBATextureChannel2RGBA);
public:
    static		void initClass();
			SoRGBATextureChannel2RGBA();

    SoMFBool		enabled;

    const SbImage&	getRGBA(int rgba) const;
			/*!<\param rgba is 0 for red, 1 for green, 2 for
			           blue and 3 for opacity. */
protected:
			~SoRGBATextureChannel2RGBA();
    void		sendRGBA(SoState*);
    void		getTransparencyStatus( const SbImage* channels,
	    		    long size, int channelidx, char& fullyopaque,
			    char& fullytrans) const;
    void		computeRGBA( const SbImage* channels,
	    			     int start, int stop,
				     int firstchannel, int lastchannel );
    static void		fieldChangeCB( void* data, SoSensor* );

    void		GLRender(SoGLRenderAction*);

    SbImage		rgba_[4];
    char		ti_;
    SoElement*		matchinfo_;
    int			prevnodeid_;
};

#endif
