/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Sep 2008
 RCS:		$Id: uisegyread.cc,v 1.14 2008-10-17 13:06:53 cvsbert Exp $
________________________________________________________________________

-*/

#include "uisegyread.h"
#include "uisegydef.h"
#include "uisegydefdlg.h"
#include "uisegyimpdlg.h"
#include "uisegyscandlg.h"
#include "uisegyexamine.h"
#include "uiioobjsel.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uifileinput.h"
#include "uitaskrunner.h"
#include "uitextedit.h"
#include "uiobjdisposer.h"
#include "uimsg.h"
#include "survinfo.h"
#include "seistrctr.h"
#include "segyscanner.h"
#include "seisioobjinfo.h"
#include "ptrman.h"
#include "ioman.h"
#include "ioobj.h"
#include "ctxtioobj.h"
#include "oddirs.h"
#include "filepath.h"
#include "timefun.h"
#include <sstream>

static const char* sKeySEGYRev1Pol = "SEG-Y Rev. 1 policy";

#define mSetState(st) { state_ = st; nextAction(); return; }


void uiSEGYRead::Setup::getDefaultTypes( TypeSet<Seis::GeomType>& geoms )
{
    if ( SI().has3D() )
    {
	geoms += Seis::Vol;
	geoms += Seis::VolPS;
    }
    if ( SI().has2D() )
    {
	geoms += Seis::Line;
	geoms += Seis::LinePS;
    }
}


uiSEGYRead::uiSEGYRead( uiParent* p, const uiSEGYRead::Setup& su )
    : setup_(su)
    , parent_(p)
    , geom_(SI().has3D()?Seis::Vol:Seis::Line)
    , state_(BasicOpts)
    , scanner_(0)
    , rev_(Rev0)
    , revpolnr_(2)
    , defdlg_(0)
    , examdlg_(0)
    , impdlg_(0)
    , scandlg_(0)
    , rev1qdlg_(0)
    , processEnded(this)
{
    nextAction();
}

// Destructor at end of file (deleting local class)


void uiSEGYRead::closeDown()
{
    uiOBJDISP()->go( this );
    processEnded.trigger();
}


void uiSEGYRead::nextAction()
{
    if ( state_ <= Finished )
	{ closeDown(); return; }

    switch ( state_ )
    {
    case Wait4Dialog:
	return;
    break;
    case BasicOpts:
	getBasicOpts();
    break;
    case SetupImport:
	setupImport();
    break;
    case SetupScan:
	setupScan();
    break;
    }
}


void uiSEGYRead::setGeomType( const IOObj& ioobj )
{
    bool is2d = false; bool isps = false;
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIs2D, is2d );
    ioobj.pars().getYN( SeisTrcTranslator::sKeyIsPS, isps );
    geom_ = Seis::geomTypeOf( is2d, isps );
}


void uiSEGYRead::use( const IOObj* ioobj, bool force )
{
    if ( !ioobj ) return;

    pars_.merge( ioobj->pars() );
    SeisIOObjInfo oinf( ioobj );
    if ( oinf.isOK() )
	setGeomType( *ioobj );
}


void uiSEGYRead::fillPar( IOPar& iop ) const
{
    iop.merge( pars_ );
    if ( rev_ == Rev0 )
	iop.setYN( SEGY::FileDef::sKeyForceRev0, true );
}


void uiSEGYRead::usePar( const IOPar& iop )
{
    pars_.merge( iop );
    if ( iop.isTrue( SEGY::FileDef::sKeyForceRev0 ) ) rev_ = Rev0;
}


void uiSEGYRead::writeReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    if ( !rddlg ) { pErrMsg("Huh"); return; }

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    BufferString objnm( rddlg->saveObjName() );
    ctio->ctxt.setName( objnm );
    if ( !ctio->fillObj(false) ) return;
    else
    {
	BufferString translnm = ctio->ioobj->translator();
	if ( translnm != "SEG-Y" )
	{
	    ctio->setObj( 0 );
	    objnm += " SGY";
	    if ( !ctio->fillObj(false) ) return;
	    translnm = ctio->ioobj->translator();
	    if ( translnm != "SEG-Y" )
	    {
		uiMSG().error( "Cannot write setup under this name - sorry" );
		return;
	    }
	}
    }

    rddlg->updatePars();
    fillPar( ctio->ioobj->pars() );
    ctio->ioobj->pars().removeWithKey( uiSEGYExamine::Setup::sKeyNrTrcs );
    ctio->ioobj->pars().removeWithKey( sKey::Geometry );
    SEGY::FileSpec::ensureWellDefined( *ctio->ioobj );
    IOM().commitChanges( *ctio->ioobj );
    delete ctio->ioobj;
}


void uiSEGYRead::readReq( CallBacker* cb )
{
    uiDialog* parnt = defdlg_;
    uiSEGYReadDlg* rddlg = 0;
    if ( parnt )
	geom_ = defdlg_->geomType();
    else
    {
	mDynamicCastGet(uiSEGYReadDlg*,dlg,cb)
	if ( !dlg ) { pErrMsg("Huh"); return; }
	rddlg = dlg;
    }

    PtrMan<CtxtIOObj> ctio = getCtio( true );
    uiIOObjSelDlg dlg( parnt, *ctio, "Select SEG-Y setup" );
    dlg.setModal( true );
    PtrMan<IOObj> ioobj = dlg.go() && dlg.ioObj() ? dlg.ioObj()->clone() : 0;
    if ( !ioobj ) return;

    if ( rddlg )
	rddlg->use( ioobj, false );
    else
    {
	pars_.merge( ioobj->pars() );
	defdlg_->use( ioobj, false );
    }
}


class uiSEGYReadPreScanner : public uiDialog
{
public:

uiSEGYReadPreScanner( uiParent* p, Seis::GeomType gt, const IOPar& pars )
    : uiDialog(p,uiDialog::Setup("SEG-Y Scan",0,mNoHelpID))
    , pars_(pars)
    , geom_(gt)
    , scanner_(0)
    , res_(false)
    , rep_("SEG-Y scan report")
{
    nrtrcsfld_ = new uiGenInput( this, "Limit to number of traces",
	    			 IntInpSpec(1000) );
    nrtrcsfld_->setWithCheck( true );
    nrtrcsfld_->setChecked( true );

    SEGY::FileSpec fs; fs.usePar( pars );
    BufferString fnm( fs.fname_ );
    replaceCharacter( fnm.buf(), '*', 'x' );
    FilePath fp( fnm );
    fp.setExtension( "txt" );
    saveasfld_ = new uiFileInput( this, "Save report as",
	    			  GetProcFileName(fp.fileName()) );
    saveasfld_->setWithCheck( true );
    saveasfld_->attach( alignedBelow, nrtrcsfld_ );
    saveasfld_->setChecked( true );

    setModal( true );
}

~uiSEGYReadPreScanner()
{
    delete scanner_;
}

bool acceptOK( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->isChecked() ? nrtrcsfld_->getIntValue() : 0;
    const char* fnm = saveasfld_->isChecked() ? saveasfld_->fileName() : "";

    scanner_= new SEGY::Scanner( pars_, geom_ );
    scanner_->setMaxNrtraces( nrtrcs );
    uiTaskRunner uitr( this );
    uitr.execute( *scanner_ );
    res_ = true;
    if ( scanner_->fileData().isEmpty() )
    {
	uiMSG().error( "No traces found" );
	res_ = false;
    }
    else
    {
	scanner_->getReport( rep_ );
	if ( *fnm && ! rep_.write(fnm,IOPar::sKeyDumpPretty) )
	    uiMSG().warning( "Cannot write report to specified file" );
    }

    uiDialog* dlg = new uiDialog( parent(), uiDialog::Setup("SEG-Y Scan Report",
				  mNoDlgTitle,mNoHelpID).modal(false) );
    dlg->setCtrlStyle( uiDialog::LeaveOnly );
    std::ostringstream strstrm; rep_.dumpPretty( strstrm );
    uiTextEdit* te = new uiTextEdit( dlg, "SEG-Y Scan Report" );
    te->setText( strstrm.str().c_str() );
    dlg->setDeleteOnClose( true ); dlg->go();
    return true;
}

    const Seis::GeomType geom_;
    const IOPar&	pars_;

    uiGenInput*		nrtrcsfld_;
    uiFileInput*	saveasfld_;

    bool		res_;
    IOPar		rep_;
    SEGY::Scanner*	scanner_;

};


void uiSEGYRead::preScanReq( CallBacker* cb )
{
    mDynamicCastGet(uiSEGYReadDlg*,rddlg,cb)
    if ( !rddlg ) { pErrMsg("Huh"); return; }
    rddlg->updatePars();
    fillPar( pars_ );

    uiSEGYReadPreScanner dlg( rddlg, geom_, pars_ );
    if ( !dlg.go() || !dlg.res_ ) return;
}


CtxtIOObj* uiSEGYRead::getCtio( bool forread ) const
{
    CtxtIOObj* ret = mMkCtxtIOObj( SeisTrc );
    IOObjContext& ctxt = ret->ctxt;
    ctxt.trglobexpr = "SEG-Y";
    ctxt.forread = forread;
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIs2D, Seis::is2D(geom_) );
    ctxt.parconstraints.setYN( SeisTrcTranslator::sKeyIsPS, Seis::isPS(geom_) );
    ctxt.includeconstraints = ctxt.allownonreaddefault = true;
    ctxt.allowcnstrsabsent = false;
    return ret;
}


static const char* rev1info =
    "The file was marked as SEG-Y Revision 1 by the producer."
    "\nUnfortunately, not all files are correct in this respect."
    "\n\nPlease specify:";
static const char* rev1txts[] =
{
    "Yes - I know the file is 100% correct SEG-Y Rev.1",
    "Yes - It's Rev. 1 but I may need to overrule some things",
    "No - the file is not SEG-Y Rev.1 - treat as legacy SEG-Y Rev. 0",
    "Cancel - Something must be wrong - take me back",
    0
};

class uiSEGYReadRev1Question : public uiDialog
{
public:

uiSEGYReadRev1Question( uiParent* p, int pol )
    : uiDialog(p,Setup("Determine SEG-Y revision",rev1info,mNoHelpID)
	    	.modal(false) )
    , initialpol_(pol)
{
    uiButtonGroup* bgrp = new uiButtonGroup( this, "" );
    for ( int idx=0; rev1txts[idx]; idx++ )
	buts_ += new uiRadioButton( bgrp, rev1txts[idx] );
    bgrp->setRadioButtonExclusive( true );
    buts_[pol-1]->setChecked( true );

    dontaskfld_ = new uiCheckBox( this, "Don't ask again for this survey" );
    dontaskfld_->attach( alignedBelow, bgrp );
}

bool acceptOK( CallBacker* )
{
    pol_ = 3;
    for ( int idx=0; idx<buts_.size(); idx++ )
    {
	if ( buts_[idx]->isChecked() )
	    { pol_ = idx + 1; break; }
    }
    int storepol = dontaskfld_->isChecked() ? -pol_ : pol_;
    if ( storepol != initialpol_ )
    {
	SI().getPars().set( sKeySEGYRev1Pol, storepol );
	SI().savePars();
    }
    return true;
}

bool isGoBack() const
{
    return pol_ == buts_.size();
}

    ObjectSet<uiRadioButton>	buts_;
    uiCheckBox*			dontaskfld_;
    int				pol_, initialpol_;

};

#define mSetreadReqCB() readParsReq.notify( mCB(this,uiSEGYRead,readReq) )
#define mSetwriteReqCB() writeParsReq.notify( mCB(this,uiSEGYRead,writeReq) )
#define mSetpreScanReqCB() preScanReq.notify( mCB(this,uiSEGYRead,preScanReq) )
#define mLaunchDlg(dlg,fn) \
	dlg->windowClosed.notify( mCB(this,uiSEGYRead,fn) ); \
	dlg->setDeleteOnClose( true ); dlg->go()

void uiSEGYRead::getBasicOpts()
{
    delete defdlg_;
    uiSEGYDefDlg::Setup bsu; bsu.geoms_ = setup_.geoms_;
    bsu.defgeom( geom_ ).modal( false );
    defdlg_ = new uiSEGYDefDlg( parent_, bsu, pars_ );
    defdlg_->mSetreadReqCB();
    mLaunchDlg(defdlg_,defDlgClose);
    mSetState(Wait4Dialog);
}


void uiSEGYRead::basicOptsGot()
{
    if ( !defdlg_->uiResult() )
	mSetState(Cancelled);
    geom_ = defdlg_->geomType();

    uiSEGYExamine::Setup exsu( defdlg_->nrTrcExamine() );
    exsu.modal( false ); exsu.usePar( pars_ );
    delete examdlg_; examdlg_ = new uiSEGYExamine( parent_, exsu );
    mLaunchDlg(examdlg_,examDlgClose);
    const int exrev = examdlg_->getRev();
    if ( exrev < 0 )
	{ rev_ = Rev0; mSetState(BasicOpts); }

    rev_ = exrev ? WeakRev1 : Rev0;
    if ( rev_ != Rev0 )
    {
	SI().pars().get( sKeySEGYRev1Pol, revpolnr_ );
	if ( revpolnr_ < 0 )
	    revpolnr_ = -revpolnr_;
	else
	{
	    delete rev1qdlg_;
	    rev1qdlg_ = new uiSEGYReadRev1Question( parent_, revpolnr_ );
	    mLaunchDlg(rev1qdlg_,rev1qDlgClose);
	    mSetState(Wait4Dialog);
	}
    }

    determineRevPol();
}



void uiSEGYRead::determineRevPol()
{
    if ( rev1qdlg_ )
    {
	if ( !rev1qdlg_->uiResult() || rev1qdlg_->isGoBack() )
	    mSetState(BasicOpts)
	revpolnr_ = rev1qdlg_->pol_;
    }
    rev_ = revpolnr_ == 1 ? Rev1 : (revpolnr_ == 2 ? WeakRev1 : Rev0);
    mSetState( setup_.purpose_ == Import ? SetupImport : SetupScan );
}



void uiSEGYRead::setupScan()
{
    delete scanner_; scanner_ = 0;
    delete scandlg_;
    uiSEGYReadDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    if ( setup_.purpose_ == SurvSetup && Seis::is2D(geom_) )
	uiMSG().warning(
	"Scanning a 2D file can only provide some info on your survey.\n"
	"To actually set up your survey, you need to select\n"
	"'Set for 2D only'\n"
	"In the survey setup window" );
    scandlg_ = new uiSEGYScanDlg( parent_, su, pars_ );
    scandlg_->mSetreadReqCB();
    scandlg_->mSetwriteReqCB();
    scandlg_->mSetpreScanReqCB();
    mLaunchDlg(scandlg_,scanDlgClose);
    mSetState( Wait4Dialog );
}


void uiSEGYRead::setupImport()
{
    delete impdlg_;
    uiSEGYImpDlg::Setup su( geom_ ); su.rev( rev_ ).modal(false);
    impdlg_ = new uiSEGYImpDlg( parent_, su, pars_ );
    impdlg_->mSetreadReqCB();
    impdlg_->mSetwriteReqCB();
    impdlg_->mSetpreScanReqCB();
    mLaunchDlg(impdlg_,impDlgClose);
    mSetState( Wait4Dialog );
}


void uiSEGYRead::defDlgClose( CallBacker* )
{
    basicOptsGot();
    defdlg_ = 0;
}


void uiSEGYRead::examDlgClose( CallBacker* )
{
    examdlg_ = 0;
}


void uiSEGYRead::scanDlgClose( CallBacker* )
{
    if ( !scandlg_->uiResult() )
	mSetState( BasicOpts );

    scanner_ = scandlg_->getScanner();
    scandlg_ = 0;
    mSetState( Finished );
}


void uiSEGYRead::impDlgClose( CallBacker* )
{
    State newstate = impdlg_->uiResult() ? Finished : BasicOpts;
    impdlg_ = 0;
    mSetState( newstate );
}


void uiSEGYRead::rev1qDlgClose( CallBacker* )
{
    determineRevPol();
    rev1qdlg_ = 0;
}


uiSEGYRead::~uiSEGYRead()
{
    delete defdlg_;
    delete impdlg_;
    delete scandlg_;
    delete rev1qdlg_;
}
