/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegyscandlg.cc,v 1.2 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyscandlg.h"

#include "uisegydef.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "segyfiledef.h"
#include "segyscanner.h"


uiSEGYScanDlg::uiSEGYScanDlg( uiParent* p, const uiSEGYReadDlg::Setup& su,
				IOPar& iop )
    : uiSEGYReadDlg(p,su,iop)
    , scanner_(0)
{
    if ( setup_.dlgtitle_.isEmpty() )
    {
	BufferString ttl( "Scanning " );
	ttl += Seis::nameOf( setup_.geom_ );
	SEGY::FileSpec fs; fs.usePar( iop );
	ttl += " '"; ttl += fs.fname_; ttl += "'";
	setTitleText( ttl );
    }
    if ( !optsgrp_ )
	new uiLabel( this, "Press OK or hit enter to start SEG-Y scan" );
}


uiSEGYScanDlg::~uiSEGYScanDlg()
{
    delete scanner_;
}


SEGY::Scanner* uiSEGYScanDlg::getScanner()
{
    SEGY::Scanner* ret = scanner_;
    scanner_ = 0;
    return ret;
}


bool uiSEGYScanDlg::doWork( const IOObj& ioobj )
{
    SEGY::FileSpec fs; fs.usePar( pars_ );
    delete scanner_; scanner_ = new SEGY::Scanner( fs, setup_.geom_, pars_ );
    if ( setup_.rev_ == uiSEGYRead::Rev0 )
	scanner_->setForceRev0( true );
    uiTaskRunner tr( parent_ );
    return tr.execute(*scanner_);
}
