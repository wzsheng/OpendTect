#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.7 2002-04-19 08:57:52 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "sets.h"
#include "draw.h"

class SoCoordinate3;
class SoText2;
class SoSwitch;
class SoTranslation;
class SoDrawStyle;
class AxisInfo;

namespace visBase
{

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

class Annotation : public VisualObjectImpl
{
public:
    static Annotation*		create()
				mCreateDataObj0arg(Annotation);

    void			showText( bool yn );
    bool			isTextShown() const;
    void			setCorner( int, float, float, float );
    void			setText( int dim, const char * );
    void			setLineStyle( const LineStyle& );
    				/*!< Color in Linestyle is ignored, must be
				     set separately 
				*/
    const LineStyle&		lineStyle() const { return linestyle; }

protected:
    LineStyle			linestyle;
    void			updateLineStyle();

    void			updateTextPos();
    SoCoordinate3*		coords;

    ObjectSet<SoText2>		texts;
    ObjectSet<SoTranslation>	textpositions;
    SoSwitch*			textswitch;

};

};

#endif
