/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseispsman.cc,v 1.16 2009-11-23 11:54:42 cvsbert Exp $";


#include "uiseispsman.h"
#include "uiprestkmergedlg.h"
#include "pixmap.h"
#include "seispsioprov.h"
#include "uitextedit.h"
#include "uiioobjmanip.h"
#include "uiioobjsel.h"
#include "uiseismulticubeps.h"
#include "posinfo.h"
#include "keystrs.h"


uiSeisPreStackMan::uiSeisPreStackMan( uiParent* p, bool is2d )
    : uiObjFileMan(p,uiDialog::Setup("Pre-stack seismics management",
                                     "Manage Pre-stack seismics",
                                     "103.4.0").nrstatusflds(1),
	    	   is2d ? SeisPS2DTranslatorGroup::ioContext()
		        : SeisPS3DTranslatorGroup::ioContext())
    , is2d_(is2d)
{
    createDefaultUI();
    selgrp->setPrefWidthInChar( 50 );
    infofld->setPrefWidthInChar( 60 );
    uiIOObjManipGroup* manipgrp = selgrp->getManipGroup();
    if ( !is2d )
    {
	manipgrp->addButton( ioPixmap("copyobj.png"),
			     mCB(this,uiSeisPreStackMan,copyPush),
			     "Copy data store" );
	manipgrp->addButton( ioPixmap("mergeseis.png"),
			     mCB(this,uiSeisPreStackMan,mergePush),
			     "Merge data stores" );
	manipgrp->addButton( ioPixmap("mkmulticubeps.png"),
			     mCB(this,uiSeisPreStackMan,mkMultiPush),
			     "Create Multi-Cube data store" );
    }

    selChg(0);
}


uiSeisPreStackMan::~uiSeisPreStackMan()
{
}


void uiSeisPreStackMan::mkFileInfo()
{
    BufferString txt;
    if ( is2d_ )
    {
	BufferStringSet nms;
	SPSIOPF().getLineNames( *curioobj_, nms );
	if ( nms.size() > 0 )
	{
	    if ( nms.size() > 4 )
		{ txt = "Number of lines: "; txt += nms.size(); }
	    else
	    {
		if ( nms.size() == 1 )
		    { txt = "Line: "; txt += nms.get( 0 ); }
		else
		{
		    txt = "Lines: ";
		    for ( int idx=0; idx<nms.size(); idx++ )
		    {
			if ( idx ) txt += ", ";
			txt += nms.get( idx );
		    }
		}
	    }
	    txt += "\n\n";
	}
    }
    else
    {
	SeisPS3DReader* rdr = SPSIOPF().get3DReader( *curioobj_ );
	const PosInfo::CubeData& cd = rdr->posData();
	txt = "Total number of gathers: "; txt += cd.totalSize(); txt += "\n";
	const bool haveinlstep = cd.haveInlStepInfo();
	const bool havecrlstep = cd.haveCrlStepInfo();
	const bool havebothsteps = haveinlstep && havecrlstep;
	StepInterval<int> rg; cd.getInlRange( rg );
	txt += "Inline range: ";
	txt += rg.start; txt += " - "; txt += rg.stop;
	if ( cd.haveInlStepInfo() )
	{ txt += " step "; txt += rg.step; }
	txt += "\n";
	cd.getCrlRange( rg );
	txt += "Crossline range: ";
	txt += rg.start; txt += " - "; txt += rg.stop;
	if ( cd.haveCrlStepInfo() )
	{ txt += " step "; txt += rg.step; }
	txt += "\n\n";
	delete rdr;
    }
    txt += getFileInfo();
    infofld->setText( txt );
}


void uiSeisPreStackMan::copyPush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackCopyDlg dlg( this, key );
    dlg.go();
    selgrp->fullUpdate( key );
}


void uiSeisPreStackMan::mergePush( CallBacker* )
{
    if ( !curioobj_ ) return;

    const MultiID key( curioobj_->key() );
    uiPreStackMergeDlg dlg( this );
    dlg.go();
    selgrp->fullUpdate( key );
}


void uiSeisPreStackMan::mkMultiPush( CallBacker* )
{
    const MultiID key( curioobj_ ? curioobj_->key() : MultiID("") );
    uiSeisMultiCubePS dlg( this );
    dlg.go();
    selgrp->fullUpdate( key );
}
