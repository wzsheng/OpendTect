/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		September 2005
 RCS:		$Id: uichangesurfacedlg.cc,v 1.1 2006-11-20 16:57:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "uichangesurfacedlg.h"

#include "uiarray2dinterpol.h"
#include "uicursor.h"
#include "uiexecutor.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "ctxtioobj.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "emposid.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioobj.h"
#include "ptrman.h"
#include "errh.h"


uiChangeSurfaceDlg::uiChangeSurfaceDlg( uiParent* p, EM::Horizon* hor )
    : uiDialog(p,Setup("Horizon manipulation","",""))
    , horizon_(hor)
    , inputfld_(0)
    , outputfld_(0)
    , savefld_(0)
    , ctioin_(0)
    , ctioout_(0)
    , parsgrp_(0)
{
    if ( !horizon_ )
    {
	ctioin_ = mMkCtxtIOObj( EMHorizon );
	ctioin_->ctxt.forread = true;
	inputfld_ = new uiIOObjSel( this, *ctioin_, "Select Horizon" );
    }

    if ( !horizon_ )
    {
	savefld_ = new uiGenInput( this, "Save interpolated horizon",
				   BoolInpSpec("As new","Overwrite",false) );
	savefld_->valuechanged.notify( mCB(this,uiChangeSurfaceDlg,saveCB) );

	ctioout_ = mMkCtxtIOObj( EMHorizon );
	ctioout_->ctxt.forread = false;
	outputfld_ = new uiIOObjSel( this, *ctioout_, "Output Horizon" );
	outputfld_->attach( alignedBelow, savefld_ );
	saveCB(0);
    }

    if ( horizon_ ) horizon_->ref();
}

void uiChangeSurfaceDlg::attachPars()
{
    if ( !parsgrp_ ) return;

    if ( inputfld_ )
	parsgrp_->attach( alignedBelow, inputfld_ );
    if ( savefld_ )
	savefld_->attach( alignedBelow, parsgrp_ );
}


uiChangeSurfaceDlg::~uiChangeSurfaceDlg()
{
    if ( ctioin_ ) { delete ctioin_->ioobj; delete ctioin_; }
    if ( ctioout_ ) { delete ctioout_->ioobj; delete ctioout_; }
    if ( horizon_ ) horizon_->unRef();
}


void uiChangeSurfaceDlg::saveCB( CallBacker* )
{
    outputfld_->display( savefld_->getBoolValue() );
}


bool uiChangeSurfaceDlg::readHorizon()
{
    const MultiID& mid = inputfld_->ctxtIOObj().ioobj->key();
    EM::ObjectID oid = EM::EMM().getObjectID( mid );
    EM::EMObject* emobj = EM::EMM().getObject( oid );

    Executor* reader = 0;
    if ( !emobj || !emobj->isFullyLoaded() )
    {
	reader = EM::EMM().objectLoader( mid );
	if ( !reader ) return false;

	uiExecutor dlg( this, *reader );
	if ( !dlg.go() )
	{
	    delete reader;
	    return false;
	}

	oid = EM::EMM().getObjectID( mid );
	emobj = EM::EMM().getObject( oid );
    }

    mDynamicCastGet(EM::Horizon*,hor,emobj)
    horizon_ = hor;
    horizon_->ref();
    delete reader;
    return true;
}


bool uiChangeSurfaceDlg::doProcessing()
{
    uiCursorChanger chgr( uiCursor::Wait );
    for ( int idx=0; idx<horizon_->geometry().nrSections(); idx++ )
    {
	EM::SectionID sid = horizon_->geometry().sectionID( idx );
	const StepInterval<int> rowrg = horizon_->geometry().rowRange( sid );
	const StepInterval<int> colrg = horizon_->geometry().colRange( sid );

	Array2D<float>* arr = horizon_->createArray2D( sid );
	if ( !arr )
	{
	    BufferString msg( "Not enough horizon data for section " );
	    msg += sid;
	    ErrMsg( msg ); continue;
	}

	PtrMan<Executor> worker = getWorker( *arr );
	uiExecutor dlg( this, *worker );
	if ( !dlg.go() )
	{
	    delete arr;
	    horizon_->unRef();
	    return false;
	}

	if ( !savefld_ )
	{
	    BufferString msg = infoMsg();
	    if ( msg != "" )
		uiMSG().message( msg );
	}

	horizon_->setArray2D( *arr, sid, true );
	delete arr;
    }

    return true;
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiChangeSurfaceDlg::saveHorizon()
{
    PtrMan<Executor> exec = 0;
    const bool saveas = savefld_ && savefld_->getBoolValue();
    if ( !saveas )
	exec = horizon_->saver();
    else
    {
	const MultiID& mid = outputfld_->ctxtIOObj().ioobj->key();
	horizon_->setMultiID( mid );
	exec = horizon_->geometry().saver( 0, &mid );
    }

    if ( !exec )
    {
	uiMSG().error( "Cannot save horizon" );
	return false;
    }

    uiExecutor dlg( this, *exec );
    const bool res = dlg.go();
    return res;
}


bool uiChangeSurfaceDlg::acceptOK( CallBacker* )
{
    if ( inputfld_ && !inputfld_->commitInput(false) )
	mErrRet( "Please select input horizon" )

    const bool savetodisk = outputfld_;
    const bool saveasnew = savefld_ && savefld_->getBoolValue();
    if ( savetodisk && savefld_->getBoolValue() && 
	 !outputfld_->commitInput(true) )
	mErrRet( "Please select output horizon" )

    if ( !horizon_ && !readHorizon() )
	mErrRet( "Cannot read horizon" )

    if ( !doProcessing() )
	return false;

    if ( savetodisk && !saveHorizon() )
	return false;

    return true;
}
