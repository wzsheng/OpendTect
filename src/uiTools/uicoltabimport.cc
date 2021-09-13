/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          April 2008
________________________________________________________________________

-*/

#include "uicoltabimport.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uistrings.h"

#include "coltabsequence.h"
#include "file.h"
#include "filepath.h"
#include "iopar.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "settings.h"


static uiStringSet getImportOptions()
{
    uiStringSet opts;
    opts.add( uiStrings::sOtherUser() );
    opts.add( uiStrings::sODTColTab() );
    opts.add( uiStrings::sPetrelAlut() );
    return opts;
}
#define mOtherUser 0
#define mODTColTab 1
#define mPetrelAlut 2

static BufferString sHomePath;
static BufferString sFilePath;

uiColTabImport::uiColTabImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(uiStrings::sColorTable()),
				  mNoDlgTitle, mODHelpKey(mColTabImportHelpID)))
    , dirfld_(0)
    , dtectusrfld_(0)
{
    setOkText( uiStrings::sImport() );

    choicefld_ = new uiGenInput( this, tr("Import from"),
				 StringListInpSpec(getImportOptions()) );
    choicefld_->valuechanged.notify( mCB(this,uiColTabImport,choiceSel) );

    sHomePath = sFilePath = GetPersonalDir();
    dirfld_ = new uiFileInput( this, getLabelStr(mOtherUser),
			       uiFileInput::Setup(sHomePath)
			       .directories(true) );
    dirfld_->setReadOnly();
    dirfld_->valuechanged.notify( mCB(this,uiColTabImport,usrSel) );
    dirfld_->attach( alignedBelow, choicefld_ );

    dtectusrfld_ = new uiGenInput( this, tr("DTECT_USER (if any)") );
    dtectusrfld_->attach( alignedBelow, dirfld_ );
    dtectusrfld_->updateRequested.notify( mCB(this,uiColTabImport,usrSel) );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Color table(s) to add") );
    su.lblpos( uiListBox::LeftTop );
    listfld_ = new uiListBox( this, su );
    listfld_->attach( alignedBelow, dtectusrfld_ );

    messagelbl_ = new uiLabel( this, uiString::emptyString() );
    messagelbl_->setTextColor( Color::Red() );
    messagelbl_->setHSzPol( uiObject::Wide );
    messagelbl_->attach( alignedBelow, dtectusrfld_ );
    messagelbl_->display( false );

    choiceSel( 0 );
}


uiColTabImport::~uiColTabImport()
{
    deepErase( seqs_ );
}


const char* uiColTabImport::getCurrentSelColTab() const
{
    return listfld_->getText();
}


void uiColTabImport::choiceSel( CallBacker* )
{
    switch( choicefld_->getIntValue() )
    {
	case mOtherUser:
	    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
	    dirfld_->setTitleText( tr("User's HOME folder") );
	    dirfld_->setFileName( sHomePath );
	    dtectusrfld_->display( true );
	    break;
	case mODTColTab:
	    dirfld_->setSelectMode( uiFileDialog::ExistingFile );
	    dirfld_->setTitleText( uiStrings::sODTColTab() );
	    dirfld_->setFileName( sFilePath );
	    dtectusrfld_->display( false );
	    break;
	case mPetrelAlut:
	    dirfld_->setSelectMode( uiFileDialog::DirectoryOnly );
	    dirfld_->setTitleText( uiStrings::sPetrelAlut() );
	    dirfld_->setFileName( sHomePath );
	    dtectusrfld_->display( false );
	    break;
    }

    usrSel(0);
}


uiString uiColTabImport::getLabel( bool fromusr )
{
    uiString ret = fromusr ? tr("User's HOME folder") : uiStrings::sFile();
    return ret;
}


uiString uiColTabImport::getLabelStr( int imptype )
{
    switch( imptype )
    {
	case mOtherUser: return tr("User's HOME folder");
			break;
	case mODTColTab: return uiStrings::sODTColTab();
			break;
	case mPetrelAlut: return uiStrings::sPetrelAlut();
			break;
    }
    return uiString::empty();
}


#define mErrRet(s1) { uiMSG().error(s1); return; }

void uiColTabImport::usrSel( CallBacker* )
{
    PtrMan<IOPar> ctabiop = nullptr;
    listfld_->setEmpty();

    const int imptype = choicefld_->getIntValue();

    FilePath fp( dirfld_->fileName() );
    if ( !File::exists(fp.fullPath()) )
    {
	uiMSG().error(tr("Please select an existing %1")
		    .arg(imptype==mODTColTab ? uiStrings::sFolder().toLower()
				  : uiStrings::sFile().toLower() ));
	return;
    }

    if ( imptype==mOtherUser )
    {
	sHomePath = fp.fullPath();

	fp.add( ".od" );
	if ( !File::exists(fp.fullPath()) )
	{
	    showMessage( tr("No '.od' folder found in selected home folder.") );
	    return;
	}
	else
	    showList();

	BufferString settdir( fp.fullPath() );
	const char* dtusr = dtectusrfld_->text();
	ctabiop = Settings::fetchExternal( "coltabs", dtusr, settdir );
	if ( !ctabiop )
	{
	    showMessage( tr("No user-defined color tables found") );
	    return;
	}
	else
	    showList();

	getFromSettingsPar( *ctabiop );
    }
    else if ( imptype==mODTColTab )
    {
	const BufferString fnm = fp.fullPath();
	if ( File::isDirectory(fnm) )
	{
	    showList();
	    return;
	}

	sFilePath = fnm;
	ctabiop = new IOPar;
	bool res = ctabiop->read( fnm, "Default settings" );
	if ( !res )
	{
	    res = ctabiop->read( fnm, 0 );
	    if ( !res )
	    {
		showMessage(uiStrings::phrCannotRead(uiStrings::phrJoinStrings
			   (uiStrings::sColorTable(), tr("from Selected"),
			    uiStrings::sFile())));
		return;
	    }
	}

	getFromSettingsPar( *ctabiop );
    }
    else if ( imptype==mPetrelAlut )
    {
	BufferStringSet filenms;
	File::listDir( fp.fullPath(), File::FilesInDir, filenms, "*.alut" );
	if ( filenms.isEmpty() )
	{
	    showMessage( tr("No *.alut files found in selected folder") );
	    return;
	}
	getFromAlutFiles( filenms );
    }
}


void uiColTabImport::getFromSettingsPar( const IOPar& par )
{
    deepErase( seqs_ );
    int nrinvalididx = 0;
    for ( int idx=0; ; idx++ )
    {
	IOPar* subpar = par.subselect( idx );
	if ( !subpar || !subpar->size() )
	{
	    delete subpar;
	    nrinvalididx++;
	    if ( nrinvalididx>1000 ) break;
	    else
		continue;
	}
	const char* nm = subpar->find( sKey::Name() );
	if ( !nm )
	    { delete subpar; nrinvalididx++; continue; }

	ColTab::Sequence* seq = new ColTab::Sequence;
	seq->usePar( *subpar );
	seqs_ += seq;
	uiPixmap coltabpix( 16, 10 );
	coltabpix.fill( *seq, true );
	listfld_->addItem( ::toUiString(nm), coltabpix );
    }

    if ( listfld_->isEmpty() )
	showMessage(uiStrings::phrCannotRead(uiStrings::phrJoinStrings
		   (uiStrings::sColorTable(), tr("from Selected"),
		    uiStrings::sFile())));
    else
	showList();
}


void uiColTabImport::getFromAlutFiles( const BufferStringSet& filenms )
{
    deepErase( seqs_ );
    for ( const auto* filenm : filenms )
    {
	FilePath fp( dirfld_->fileName(), filenm->str() );
	od_istream strm( fp.fullPath() );
	if ( !strm.isOK() )
	    continue;

	auto* seq = new ColTab::Sequence;
	seq->setName( fp.baseName() );
	TypeSet<int> r, g, b, a;
	BufferString line;
	BufferStringSet nums;
	while ( strm.isOK() )
	{
	    strm.getLine( line );
	    if ( line.isEmpty() || strm.isBad() )
		break;

	    nums.setEmpty();
	    nums.unCat( line, "," );
	    if ( nums.size()!=4 )
		break;

	    r += nums[0]->toInt();
	    g += nums[1]->toInt();
	    b += nums[2]->toInt();
	    a += nums[3]->toInt();
	}
	if ( r.isEmpty() )
	{
	    delete seq;
	    continue;
	}
	StepInterval<float> si( 0.f, 1.f, 1.f/(r.size()-1) );
	for ( int idx=0; idx<r.size(); idx++ )
	{
	    const float pos = si.atIndex( idx );
	    seq->setColor( pos, r[idx], g[idx], b[idx] );
	    if ( idx==0 || idx==r.size()-1 || a[idx]!=a[idx-1] )
		seq->setTransparency( Geom::Point2D<float>(pos,255-a[idx]) );
	}
	seqs_ += seq;
	uiPixmap coltabpix( 16, 10 );
	coltabpix.fill( *seq, true );
	listfld_->addItem( ::toUiString(fp.baseName()), coltabpix );
    }
    if ( !listfld_->isEmpty() )
	showList();
}


bool uiColTabImport::acceptOK( CallBacker* )
{
    bool oneadded = false;

    ObjectSet<const ColTab::Sequence> tobeadded;
    for ( int idx=0; idx<listfld_->size(); idx++ )
    {
	if ( listfld_->isChosen(idx) )
	    tobeadded += seqs_[idx];
    }

    for ( int idx=0; idx<tobeadded.size(); idx++ )
    {
	ColTab::Sequence seq( *tobeadded[idx] );
	bool doset = true;
	const int seqidx = ColTab::SM().indexOf( seq.name() );
	if ( seqidx >= 0 )
	{
	    uiString msg = tr("User colortable '%1' "
			      "will replace the existing.\nOverwrite?")
			 .arg(seq.name());
	    doset = uiMSG().askOverwrite( msg );
	}

	if ( doset )
	{
	    oneadded = true;
	    seq.setType( ColTab::Sequence::User );
	    ColTab::SM().set( seq );
	}
    }

    if ( oneadded )
	ColTab::SM().write( false );

    uiString msg = tr( "Color table successfully imported."
		      "\n\nDo you want to import more Color tables?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


void uiColTabImport::showMessage( const uiString& msg )
{
    messagelbl_->setText( msg );
    messagelbl_->display( true );
    listfld_->display( false );
}


void uiColTabImport::showList()
{
    messagelbl_->display( false );
    listfld_->display( true );
}
