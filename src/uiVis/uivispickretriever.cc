/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Jan 2005
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uivispickretriever.cc,v 1.9 2009-07-22 16:01:43 cvsbert Exp $";

#include "uivispickretriever.h"

#include "visevent.h"
#include "vissurvscene.h"
#include "vistransform.h"

uiVisPickRetriever::uiVisPickRetriever()
    : status_( Idle )
    , finished_( this )
{}


uiVisPickRetriever::~uiVisPickRetriever()
{
    while ( scenes_.size() )
	removeScene( scenes_[0] );
}


bool uiVisPickRetriever::enable(  const TypeSet<int>* scenes )
{
    if ( status_!=Idle ) return false;

    status_ = Waiting;
    if ( scenes ) allowedscenes_ = *scenes;
    else allowedscenes_.erase();

    return true;
}


void uiVisPickRetriever::addScene( visSurvey::Scene* scene )
{
    scene->eventCatcher().eventhappened.notify(
	    mCB( this, uiVisPickRetriever, pickCB ) );
    scenes_ += scene;
}


void uiVisPickRetriever::removeScene( visSurvey::Scene* scene )
{
    scene->eventCatcher().eventhappened.remove(
	    mCB( this, uiVisPickRetriever, pickCB ) );
    scenes_ -= scene;
}


void uiVisPickRetriever::pickCB( CallBacker* cb )
{
    if ( status_!=Waiting )
	return;

    mCBCapsuleUnpackWithCaller( const visBase::EventInfo&,
	    			eventinfo, caller, cb );
    if ( eventinfo.type!=visBase::MouseClick ||
	 !OD::leftMouseButton(eventinfo.buttonstate_) )
	return;

    visSurvey::Scene* scene = 0;
    for ( int idx=0; idx<scenes_.size(); idx++ )
    {
	if ( &scenes_[idx]->eventCatcher()==caller )
	{
	    scene = scenes_[idx];
	    break;
	}
    }

    if ( !scene || scene->eventCatcher().isHandled() )
	return;

    scene->eventCatcher().setHandled();

    if ( eventinfo.pressed ) //Block other objects from taking action
	return;

    if ( (!allowedscenes_.isEmpty() &&
	  allowedscenes_.indexOf( scene->id() )==-1)
	  || eventinfo.pickedobjids.isEmpty() )
	status_ = Failed;
    else
    {
	pickedpos_ = eventinfo.worldpickedpos;
	pickedscene_ = scene->id();
	status_ = Success;
    }


    finished_.trigger();

    status_ = Idle;
}


void uiVisPickRetriever::reset()
{
    status_ = Idle;
    allowedscenes_.erase();
}
