/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Ranojay Sen
 Date:          Dec 2009
________________________________________________________________________

-*/


#include "uisurveyselect.h"

#include "ctxtioobj.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisurvey.h"
#include "uiseparator.h"
#include "surveydisklocation.h"
#include "survinfo.h"
#include "uistrings.h"

#define mErrRet(s) { uiMSG().error(s); return; }

static bool checkIfDataDir( const char* path, bool forread )
{
    if ( SurveyInfo::isValidDataRoot(path).isOK() )
	return true;

    DirList surveydirlist( path, File::DirsInDir );
    for ( int idx=0; idx<surveydirlist.size(); idx++ )
    {
	if ( SurveyInfo::isValidSurveyDir(surveydirlist.fullPath(idx)).isOK() )
	    return forread || File::isWritable( path );
    }

    return false;
}


uiSurveySelectDlg::uiSurveySelectDlg( uiParent* p,
				      const char* survnm, const char* dataroot,
				      bool forread, bool needvalidrootdir,
				      bool selmultiplesurveys )
    : uiDialog(p,uiDialog::Setup(tr("Select Data Root and Survey"),
				 mNoDlgTitle,
				 mODHelpKey(mSurveySelectDlgHelpID)))
    , forread_(forread)
    , needvalidrootdir_(needvalidrootdir)
{
    datarootfld_ = new uiFileInput( this, tr("%1 Root").arg(uiStrings::sData()),
		uiFileInput::Setup(uiFileDialog::Gen,dataroot)
		.directories(true) );
    setDataRoot( dataroot );
    datarootfld_->valuechanged.notify(
		mCB(this,uiSurveySelectDlg,rootSelCB) );

    auto* sep = new uiSeparator( this, "Well2Log Sep" );
    sep->attach( stretchedBelow, datarootfld_ );
    surveylistfld_ = new uiListBox( this, "Survey list",
				    selmultiplesurveys ? OD::ChooseOnlyOne
						       : OD::ChooseAtLeastOne );
    surveylistfld_->setNrLines( 10 );
    surveylistfld_->attach( alignedBelow, sep );
    surveylistfld_->selectionChanged.notify(
		mCB(this,uiSurveySelectDlg,surveySelCB) );

    if ( !forread_ && !selmultiplesurveys)
    {
	surveyfld_ = new uiGenInput( this, uiStrings::sName() );
	surveyfld_->setStretch( 2, 0 );
	surveyfld_->attach( alignedBelow, surveylistfld_ );
    }

    fillSurveyList();
    setSurveyName( survnm );
}


uiSurveySelectDlg::~uiSurveySelectDlg()
{}


void uiSurveySelectDlg::setDataRoot( const char* dataroot )
{
    BufferString basedatadir( dataroot );
    if ( basedatadir.isEmpty() )
	basedatadir = GetBaseDataDir();

    datarootfld_->setText( dataroot );
}


const char* uiSurveySelectDlg::getDataRoot() const
{
    return datarootfld_->text();
}


void uiSurveySelectDlg::setSurveyName( const char* nm )
{
    surveylistfld_->setCurrentItem( nm );
    if ( surveyfld_ )
	surveyfld_->setText( nm );
}


const char* uiSurveySelectDlg::getSurveyName() const
{
    return surveyfld_ ? surveyfld_->text() : surveylistfld_->getText();
}


void uiSurveySelectDlg::getSurveyNames( BufferStringSet& survnms ) const
{
    surveylistfld_->getChosen( survnms );
}


void uiSurveySelectDlg::getSurveyPaths( BufferStringSet& survpaths ) const
{
    FilePath fps(getDataRoot(),getSurveyName());
    survpaths.addIfNew( fps.fullPath() );
}


const BufferString uiSurveySelectDlg::getSurveyPath() const
{
    return FilePath(getDataRoot(),getSurveyName()).fullPath();
}


bool uiSurveySelectDlg::continueAfterErrMsg()
{
    if ( needvalidrootdir_ )
    {
	uiMSG().error( tr("Selected folder is not a valid Data Root") );
	return false;
    }

    const bool res = uiMSG().askGoOn(
	    tr("Selected folder is not a valid Data Root. Do you still "
	       "want to search for OpendTect Surveys in this location") );
    return res;

}


void uiSurveySelectDlg::fillSurveyList()
{
    surveylistfld_->setEmpty();
    BufferString dataroot = getDataRoot();
    bool datarootisvalid = false;
    bool useparent = false;
    while ( !datarootisvalid && !dataroot.isEmpty() )
    {
	datarootisvalid = checkIfDataDir( dataroot, forread_ );
	if ( !datarootisvalid )
	{
	    dataroot = FilePath(dataroot).pathOnly();
	    useparent = true;
	}
    }

    if ( !datarootisvalid && !continueAfterErrMsg() )
	return;

    if ( useparent )
	setDataRoot( dataroot );

    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, dataroot );
    surveylistfld_->addItems( surveylist );
    surveylistfld_->resizeWidthToContents();
}


void uiSurveySelectDlg::rootSelCB( CallBacker* )
{
    fillSurveyList();
}


void uiSurveySelectDlg::surveySelCB( CallBacker* )
{
    if ( surveyfld_ )
	surveyfld_->setText( surveylistfld_->getText() );
}


bool uiSurveySelectDlg::isNewSurvey() const
{
   return surveyfld_ && !surveylistfld_->isPresent( surveyfld_->text() );
}


// uiSurveySelect
uiSurveySelect::uiSurveySelect( uiParent* p, bool forread,
				bool needvalidrootdir, const char* lbl )
    : uiIOSelect(p,uiIOSelect::Setup( lbl && *lbl ? mToUiStringTodo(lbl)
						  : uiStrings::sSurvey() )
						  .keepmytxt(true),
		 mCB(this,uiSurveySelect,selectCB))
    , dataroot_(GetBaseDataDir())
    , forread_(forread)
    , needvalidrootdir_(needvalidrootdir)
    , surveyname_(0)
    , isnewsurvey_(false)
{
    setReadOnly( forread );
}


uiSurveySelect::~uiSurveySelect()
{}


void uiSurveySelect::selectCB( CallBacker* )
{
    uiSurveySelectDlg dlg( this, surveyname_, dataroot_, forread_,
			   needvalidrootdir_ );
    if( !dlg.go() ) return;

    isnewsurvey_ = dlg.isNewSurvey();
    surveyname_ = dlg.getSurveyName();
    dataroot_ = dlg.getDataRoot();
    updateList();
    setInputText( surveyname_ );
    selok_ = true;
}


void uiSurveySelect::updateList()
{
    BufferStringSet surveylist;
    uiSurvey::getSurveyList( surveylist, dataroot_ );
    setEntries( surveylist, surveylist );
}


static BufferString makeFullSurveyPath( const char* survnm,
					const char* dataroot )
{
    BufferString surveyname( survnm );
    surveyname.clean( BufferString::AllowDots );
    return FilePath(dataroot,surveyname).fullPath();
}


bool uiSurveySelect::getFullSurveyPath( BufferString& fullpath ) const
{
    BufferString input = getInput();
    if ( input.isEmpty() )
	return false;

    FilePath fp( input );
    if ( fp.fileName() == input )
    {
	fullpath = makeFullSurveyPath( input, dataroot_ );
	return fullpath.isEmpty() ? false : true;
    }

    BufferString path( fp.pathOnly() ), survnm( fp.fileName() );
    const bool isdatadir = checkIfDataDir( path, forread_ );
    fullpath = makeFullSurveyPath( survnm, path );
    return isdatadir && !fullpath.isEmpty() ? true : false;
}


void uiSurveySelect::setSurveyPath( const char* fullpath )
{
    if ( !File::exists(fullpath) )
	mErrRet(tr("Selected folder does not exist.\n"
		   "Please specify the full path."));
    if ( !File::isDirectory(fullpath) )
	mErrRet(uiStrings::phrSelect(tr("a valid folder.")));
    if ( !File::exists(
		FilePath(fullpath,SurveyInfo::sKeySetupFileName()).fullPath()) )
	mErrRet(tr("This is not an OpendTect survey."));

    SurveyDiskLocation sdl;
    sdl.set( fullpath );
    setSurveyDiskLocation( sdl );
}


void uiSurveySelect::setSurveyDiskLocation( const SurveyDiskLocation& sdl )
{
    dataroot_ = sdl.basePath();
    surveyname_ = sdl.dirName();
    updateList();
    setInputText( surveyname_ );
}


SurveyDiskLocation uiSurveySelect::surveyDiskLocation() const
{
    BufferString fpstr;
    getFullSurveyPath( fpstr );
    const FilePath fp( fpstr );
    SurveyDiskLocation sdl( fp );
    return sdl;
}
