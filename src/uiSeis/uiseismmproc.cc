/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          April 2002
 RCS:		$Id: uiseismmproc.cc,v 1.13 2002-05-09 17:50:35 bert Exp $
________________________________________________________________________

-*/

#include "uiseismmproc.h"
#include "seismmjobman.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uiprogressbar.h"
#include "uibutton.h"
#include "uitextedit.h"
#include "uiseparator.h"
#include "uifilebrowser.h"
#include "uiiosel.h"
#include "uimsg.h"
#include "uistatusbar.h"
#include "uislider.h"
#include "hostdata.h"
#include "iopar.h"
#include <stdlib.h>


uiSeisMMProc::uiSeisMMProc( uiParent* p, const char* prognm, const IOPar& iop )
	: uiExecutor(p,getFirstJM(prognm,iop),true)
    	, running(false)
    	, finished(false)
    	, jmfinished(false)
	, logvwer(0)
{
    setCancelText( "Quit" );
    setOkText( "" );
    delay = 500;

    const char* res = iop.find( "Target value" );
    BufferString txt( "Manage processing" );
    if ( res && *res )
	{ txt += ": "; txt += res; }
    setTitleText( txt );

    tmpstordirfld = new uiIOFileSelect( this, "Temporary storage directory",
	    				false, jm->tempStorageDir() );
    tmpstordirfld->usePar( uiIOFileSelect::tmpstoragehistory );
    tmpstordirfld->selectDirectory( true );

    machgrp = new uiGroup( this, "Machine handling" );

    avmachfld = new uiLabeledListBox( machgrp, "Available hosts", true,
				      uiLabeledListBox::AboveMid );
    HostDataList hdl;
    for ( int idx=0; idx<hdl.size(); idx++ )
	avmachfld->box()->addItem( hdl[idx]->shortestName() );

    addbut = new uiPushButton( machgrp, ">> Add >>" );
    addbut->activated.notify( mCB(this,uiSeisMMProc,addPush) );
    addbut->attach( rightOf, avmachfld );

    uiGroup* usedmachgrp = new uiGroup( machgrp, "Machine handling" );
    usedmachfld = new uiLabeledListBox( usedmachgrp, "Used hosts", false,
				        uiLabeledListBox::AboveMid );
    stopbut = new uiPushButton( usedmachgrp, "Stop" );
    stopbut->activated.notify( mCB(this,uiSeisMMProc,stopPush) );
    stopbut->attach( alignedBelow, usedmachfld );
    vwlogbut = new uiPushButton( usedmachgrp, "View log" );
    vwlogbut->activated.notify( mCB(this,uiSeisMMProc,vwLogPush) );
    vwlogbut->attach( rightAlignedBelow, usedmachfld );

    usedmachgrp->attach( rightOf, addbut );
    avmachfld->attach( heightSameAs, usedmachgrp );
    machgrp->setHAlignObj( addbut );
    machgrp->attach( alignedBelow, tmpstordirfld );

    uiSeparator* sep = new uiSeparator( this, "Hor sep", true );
    sep->attach( stretchedBelow, machgrp );
    uiLabel* lbl = new uiLabel( this, "Progress" );
    lbl->attach( alignedBelow, sep );
    nicefld = new uiSlider( this, "Nice level" );
    nicefld->attach( ensureBelow, sep );
    nicefld->attach( rightBorder );
    nicefld->valueChanged.notify( mCB(this,uiSeisMMProc,niceValChg) );
    nicefld->setMinValue( -0.5 ); nicefld->setMaxValue( 19.5 );
    uiLabel* nicelbl = new uiLabel( this, "'Nice' level (0-19)", nicefld );
    progrfld = new uiTextEdit( this, "Processing progress", true );
    progrfld->attach( alignedBelow, lbl );
    progrfld->attach( widthSameAs, sep );
    progrfld->setPrefHeightInChar( 7 );
}


Executor& uiSeisMMProc::getFirstJM( const char* prognm, const IOPar& iopar )
{
    const char* res = iopar.find( "Output Seismics Key" );
    BufferString seisoutkey( res ? res : "Output.1.Seismic ID" );
    res = iopar.find( "Inline Range Key" );
    BufferString ilrgkey( res ? res : "Output.1.In-line range" );
    jm = new SeisMMJobMan( prognm, iopar, seisoutkey, ilrgkey );
    newJM();
    return *jm;
}


uiSeisMMProc::~uiSeisMMProc()
{
    delete logvwer;
    delete jm;
}


void uiSeisMMProc::newJM()
{
    if ( !jm ) return;
    jm->poststep.notify( mCB(this,uiSeisMMProc,postStep) );
}


void uiSeisMMProc::doFinalise()
{
    progbar->attach( widthSameAs, machgrp );
    progbar->attach( alignedBelow, progrfld );

    // But we start processing when at least one machine is added.
}


void uiSeisMMProc::postStep( CallBacker* )
{
    const char* txt = jm->progressMessage();
    if ( *txt ) progrfld->append( txt );
    updateCurMachs();
}


void uiSeisMMProc::niceValChg( CallBacker* )
{
    if ( !jm ) return;
    int v = nicefld->getIntValue();
    if ( v > 19 ) v = 19;
    if ( v < 0 ) v = 0;
    jm->setNiceNess( v );
}


void uiSeisMMProc::setDataTransferrer( SeisMMJobMan* newjm )
{
    delete newjm; newjm = 0;
    jmfinished = true;
    task_ = jm->dataTransferrer();
    delay = 0;
}


void uiSeisMMProc::execFinished()
{
    if ( jmfinished )
    {
	if ( !jm->removeTempSeis() )
	    uiMSG().warning( "Could not remove temporary seismics" );
	statusBar()->message( "Finished", 0 );
	finished = true;
    }
    else
    {
	updateCurMachs();
	SeisMMJobMan* newjm = new SeisMMJobMan( *jm );
	const int nrlines = newjm->totalNr();
	if ( nrlines < 1 )
	    setDataTransferrer( newjm );
	else
	{
	    BufferString msg( "The following inlines were not calculated.\n" );
	    msg += "This may be due to gaps or an unexpected error.\n";
	    for ( int idx=0; idx<nrlines; idx++ )
	    {
		msg += newjm->lineToDo(idx);
		if ( idx != nrlines-1 ) msg += " ";
	    }
	    msg += "\nDo you want to try to calculate these lines?";
	    int res = uiMSG().askGoOnAfter( msg, "Quit program" );
	    if ( res == 2 )
		reject(this);
	    else if ( res == 1 )
		setDataTransferrer( newjm );
	    else
	    {
		delete jm; jm = newjm;
		task_ = newjm;
	    }
	}
	first_time = true;
	timerTick(0);
    }
}


void uiSeisMMProc::updateCurMachs()
{
    ObjectSet<BufferString> machs;
    jm->getActiveMachines( machs );
    sort( machs );
    int curit = usedmachfld->box()->currentItem();
    usedmachfld->box()->empty();
    bool havemachs = machs.size();
    if ( havemachs )
    {
	usedmachfld->box()->addItems( machs );
	deepErase( machs );
	if ( curit >= usedmachfld->box()->size() )
	    curit = usedmachfld->box()->size() - 1;
	usedmachfld->box()->setCurrentItem(curit);
    }
    stopbut->setSensitive( havemachs );
    vwlogbut->setSensitive( havemachs );
}


bool uiSeisMMProc::rejectOK( CallBacker* )
{
    BufferString msg;

    if ( !running ) return true;

    if ( finished )
	msg = "Do you want to discard log files?";
    else
    {
	msg = "This will stop all processing!\n\n";
	msg += "Do you want to remove already processed data?";
    }

    int res = uiMSG().askGoOnAfter( msg );
    if ( res == 2 )
	return false;

    if ( res == 0 )
    {
	if ( !jm->removeTempSeis() )
	{
	    msg = "Could not remove all tempory seismic data from\n";
	    msg += jm->tempStorageDir();
	    uiMSG().warning( msg );
	}
	jm->cleanup();
    }

    return true;
}


#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeisMMProc::addPush( CallBacker* )
{
    for( int idx=0; idx<avmachfld->box()->size(); idx++ )
    {
	if ( avmachfld->box()->isSelected(idx) )
	    jm->addHost( avmachfld->box()->textOfItem(idx) );
    }

    if ( !running && jm->nrHostsInQueue() )
    {
	tmpstordirfld->setSensitive(false);
	jm->setTempStorageDir( tmpstordirfld->getInput() );
	running = true;
	prepareNextCycle(0);
    }
}


int uiSeisMMProc::getCurMach( BufferString& mach ) const
{
    int curit = usedmachfld->box()->currentItem();
    if ( curit < 0 ) return -1;

    mach = usedmachfld->box()->textOfItem(curit);
    int occ = 0;
    for( int idx=0; idx<curit-1; idx++ )
	if ( mach == usedmachfld->box()->textOfItem(idx) )
	    occ++;

    return occ;
}


void uiSeisMMProc::stopPush( CallBacker* )
{
    BufferString mach;
    if ( getCurMach(mach) < 0 ) { pErrMsg("Can't find machine"); return; }
    jm->removeHost( mach );
    updateCurMachs();
}


void uiSeisMMProc::vwLogPush( CallBacker* )
{
    BufferString mach;
    int occ = getCurMach( mach );
    if ( occ < 0 ) return;

    BufferString fname;
    if ( !jm->getLogFileName(mach,occ,fname) )
	mErrRet("Cannot find log file")

    delete logvwer;
    logvwer = new uiFileBrowser( this, fname );
    logvwer->go();
}
