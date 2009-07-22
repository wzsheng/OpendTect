#ifndef visrotationdragger_h
#define visrotationdragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		July 2006
 RCS:		$Id: visrotationdragger.h,v 1.4 2009-07-22 16:01:25 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"
#include "trigonometry.h"

class SoRotateDiscDragger;
class SoRotateSphericalDragger;
class SoSwitch;
class SoDragger;
class Quaternion;

template <class T> class Interval;

namespace visBase
{

/*! Dragger for rotations. Rotation can either be free (i.e. a trackball type),
    or bound to be around the z axis. */

mClass RotationDragger : public DataObject
{
public:
    static RotationDragger*	create()
				mCreateDataObj(RotationDragger);
    
    void			doAxisRotate();
    				/*!<\note Must be called before getInventorNode
				          is called first time.*/
    void			useSwitch();
    				/*!<\note Must be called before getInventorNode
				          is called first time.*/

    Quaternion			get() const;
    void			set(const Quaternion&);
    void			turnOn(bool yn);
    bool			isOn() const;
    void			setOwnFeedback(DataObject*,bool active);
    				/*!<\note do->getInventorNode() must return a
				 	  SoSeparator. */

    Notifier<RotationDragger>	started;
    Notifier<RotationDragger>	motion;
    Notifier<RotationDragger>	changed;
    Notifier<RotationDragger>	finished;

    SoNode*			getInventorNode();

protected:
				~RotationDragger();
    SoDragger*			getDragger();
    void			setCallbacks( SoDragger* );

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSwitch*			onoff_;
    SoRotateDiscDragger*	cyldragger_;
    SoRotateSphericalDragger*	spheredragger_;

    DataObject*			feedback_;
    DataObject*			activefeedback_;
};

};
	
#endif
