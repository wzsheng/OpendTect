/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          31/05/2000
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uimainwin.h"
#include "uidialog.h"

#include "uibody.h"
#include "uidesktopservices.h"
#include "uidockwin.h"
#include "uifont.h"
#include "uigroup.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimenu.h"
#include "uiobjbody.h"
#include "uiparentbody.h"
#include "uistatusbar.h"
#include "uiseparator.h"
#include "uitoolbar.h"
#include "uitoolbutton.h"

#include "envvars.h"
#include "filepath.h"
#include "helpview.h"
#include "iopar.h"
#include "keyboardevent.h"
#include "msgh.h"
#include "oddirs.h"
#include "odver.h"
#include "pixmap.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "strmprov.h"
#include "uitexttranslator.h"
#include "thread.h"
#include "timer.h"

#include <iostream>

#include <QAbstractButton>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QDesktopWidget>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QPixmap>
#include <QSettings>
#include <QStatusBar>
#include <QWidget>

mUseQtnamespace

class uiMainWinBody : public uiParentBody , public QMainWindow
{
friend class		uiMainWin;
public:
			uiMainWinBody(uiMainWin& handle,uiParent* parnt,
				      const char* nm,bool modal);

    void		construct(int nrstatusflds,bool wantmenubar);

    virtual		~uiMainWinBody();

#define mHANDLE_OBJ     uiMainWin
#define mQWIDGET_BASE   QMainWindow
#define mQWIDGET_BODY   QMainWindow
#define UIBASEBODY_ONLY
#define UIPARENT_BODY_CENTR_WIDGET
#include                "i_uiobjqtbody.h"

public:

    uiStatusBar*	uistatusbar();
    uiMenuBar*		uimenubar();

    virtual void        polish();
    void		reDraw(bool deep);
    void		go(bool showminimized=false);
    virtual void	show()				{ doShow(); }

    void		move(uiMainWin::PopupArea);
    void		move(int,int);

    void		close();
    bool		poppedUp() const		{ return poppedup_; }
    bool		touch();

    void		removeDockWin(uiDockWin*);
    void		addDockWin(uiDockWin&,uiMainWin::Dock);

    virtual QMenu*	createPopupMenu();
    void		addToolBar(uiToolBar*);
    uiToolBar*		removeToolBar(uiToolBar*);
    uiMenu&		getToolbarsMenu()		{ return *toolbarsmnu_;}
    void		updateToolbarsMenu();

    const ObjectSet<uiToolBar>& toolBars() const	{ return toolbars_; }
    const ObjectSet<uiDockWin>& dockWins() const	{ return dockwins_; }

    void		setModal(bool yn);
    bool		isModal() const			{ return modal_; }

    void		setWindowTitle(const char*);

    void		activateInGUIThread(const CallBack&,bool busywait);

protected:

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool trigger_finalise_start_stop);
    void		closeEvent(QCloseEvent*);
    bool		event(QEvent*);

    void		keyPressEvent(QKeyEvent*);

    void		doShow(bool minimized=false);
    void		managePopupPos();


    void		renewToolbarsMenu();
    void		toggleToolbar(CallBacker*);

    void		saveSettings();
    void		readSettings();

    bool		exitapponclose_;

    Threads::Mutex	activatemutex_;
    ObjectSet<CallBack>	activatecbs_;
    int			nractivated_;

    int			eventrefnr_;

    uiStatusBar*	statusbar_;
    uiMenuBar*		menubar_;
    uiMenu*		toolbarsmnu_;

    ObjectSet<uiToolBar> toolbars_;
    ObjectSet<uiDockWin> dockwins_;

private:

    QEventLoop		eventloop_;

    int			iconsz_;
    bool		modal_;
    Qt::WindowFlags	getFlags(bool hasparent,bool modal) const;

    void		popTimTick(CallBacker*);
    Timer		poptimer_;
    bool		poppedup_;
    uiSize		prefsz_;
    uiPoint		prefpos_;
    bool		moved_;
    bool		createtbmenu_;

    bool		deletefrombody_;
    bool		deletefromod_;
};


#define mParent p && p->pbody() ? p->pbody()->qwidget() : 0
uiMainWinBody::uiMainWinBody( uiMainWin& uimw, uiParent* p,
			      const char* nm, bool modal )
	: uiParentBody(nm)
	, QMainWindow(mParent,getFlags(p,modal) )
	, handle_(uimw)
	, initing_(true)
	, centralwidget_(0)
	, statusbar_(0)
	, menubar_(0)
	, toolbarsmnu_(0)
	, modal_(p && modal)
	, poptimer_("Popup timer")
	, poppedup_(false)
	, exitapponclose_(false)
        , prefsz_(-1,-1)
	, prefpos_(uiPoint::udf())
	, nractivated_(0)
	, moved_(false)
	, createtbmenu_(false)
{
    if ( nm && *nm )
	setObjectName( nm );

    poptimer_.tick.notify( mCB(this,uiMainWinBody,popTimTick) );

    iconsz_ = uiObject::iconSize();
    setIconSize( QSize(iconsz_,iconsz_) );

    setWindowModality( p && modal ? Qt::WindowModal
				  : Qt::NonModal );

    setDockOptions( VerticalTabs | AnimatedDocks );

    deletefrombody_ = deletefromod_ = false;
}


uiMainWinBody::~uiMainWinBody()
{
    deleteAllChildren(); //delete them now to make sure all ui objects
			 //are deleted before their body counterparts

    deepErase( toolbars_ );

    if ( toolbarsmnu_ )
    {
	toolbarsmnu_->clear();
	if ( toolbarsmnu_->isStandAlone() )
	    delete toolbarsmnu_;
    }

    if ( !deletefromod_ )
    {
	deletefrombody_ = true;
	delete &handle_;
    }

    delete statusbar_;
    delete menubar_;
}


void uiMainWinBody::setModal( bool yn )
{
    modal_ = yn;
    setWindowModality( yn ? Qt::WindowModal
			  : Qt::NonModal );
}


Qt::WindowFlags uiMainWinBody::getFlags( bool hasparent, bool modal ) const
{ return Qt::WindowFlags( Qt::Window ); }


void uiMainWinBody::doShow( bool minimized )
{
    setWindowTitle( handle_.caption(false) );
    eventrefnr_ = handle_.beginCmdRecEvent("WinPopUp");
    managePopupPos();

    if ( minimized )
	QMainWindow::showMinimized();
    else
    {
	if ( isMinimized() ) showNormal();
	raise();
	QMainWindow::show();
    }

    if( poptimer_.isActive() )
	poptimer_.stop();

    poppedup_ = false;
    poptimer_.start( 100, true );

    QEvent* ev = new QEvent( mUsrEvPopUpReady );
    QApplication::postEvent( this, ev );

    if ( modal_ )
	eventloop_.exec();
}


void uiMainWinBody::construct( int nrstatusflds, bool wantmenubar )
{
    centralwidget_ = new uiGroup( &handle(), "OpendTect Main Window" );
    setCentralWidget( centralwidget_->body()->qwidget() );

    centralwidget_->setIsMain(true);
    centralwidget_->setBorder(10);
    centralwidget_->setStretch(2,2);

    if ( nrstatusflds != 0 )
    {
	QStatusBar* mbar= statusBar();
	if ( mbar )
	    statusbar_ = new uiStatusBar( &handle(),
					  "MainWindow StatusBar handle", *mbar);
	else
	    { pErrMsg("No statusbar returned from Qt"); }

	if ( nrstatusflds > 0 )
	{
	    for( int idx=0; idx<nrstatusflds; idx++ )
		statusbar_->addMsgFld();
	}
    }
    if ( wantmenubar )
    {
	QMenuBar* qmenubar = menuBar();
	if ( qmenubar )
	    menubar_ = new uiMenuBar( &handle(), "MenuBar", qmenubar );
	else
	    { pErrMsg("No menubar returned from Qt"); }

	toolbarsmnu_ = new uiMenu( &handle(), "Toolbars" );
    }

    initing_ = false;
}


void uiMainWinBody::move( uiMainWin::PopupArea pa )
{
    QDesktopWidget wgt;
    const int xpos = wgt.screen()->width() - QMainWindow::width();
    const int ypos = wgt.screen()->height() - QMainWindow::height();

    switch( pa )
    {
	case uiMainWin::TopLeft :
	    move( 0, 0 ); break;
	case uiMainWin::TopRight :
	    move( xpos, 0 ); break;
	case uiMainWin::BottomLeft :
	    move( 0, ypos ); break;
	case uiMainWin::BottomRight :
	    move( xpos, ypos ); break;
	case uiMainWin::Middle :
	    move( mNINT32(((float) xpos)/2), mNINT32(((float) ypos) / 2));break;
	default:
	    break;
    }
}


void uiMainWinBody::move( int xdir, int ydir )
{
    QWidget::move( xdir, ydir );
    moved_ = true;
}


void uiMainWinBody::polish()
{ QMainWindow::ensurePolished(); }


void uiMainWinBody::reDraw( bool deep )
{
    update();
    centralwidget_->reDraw( deep );
}


void uiMainWinBody::go( bool showminimized )
{
    finalise( true );
    doShow( showminimized );
    move( handle_.popuparea_ );
}


bool uiMainWinBody::touch()
{
    if ( poppedup_ || !finalised() )
	return false;

    if ( poptimer_.isActive() )
	poptimer_.stop();

    if ( !poppedup_ )
	poptimer_.start( 100, true );

    return true;
}


QMenu* uiMainWinBody::createPopupMenu()
{ return createtbmenu_ ? QMainWindow::createPopupMenu() : 0; }


void uiMainWinBody::popTimTick( CallBacker* )
{
    if ( poppedup_ )
	{ pErrMsg( "huh?" ); return; }
    poppedup_ = true;

// TODO: Remove when we can get rid of the popTimTick
    if ( prefsz_.hNrPics()>0 && prefsz_.vNrPics()>0 )
	resize( prefsz_.hNrPics(), prefsz_.vNrPics() );
    if ( prefpos_ != uiPoint::udf() )
	move( prefpos_.x, prefpos_.y );
}


void uiMainWinBody::finalise( bool trigger_finalise_start_stop )
{
    if ( trigger_finalise_start_stop )
	handle_.preFinalise().trigger( handle_ );

    centralwidget_->finalise();
    finaliseChildren();

    if ( trigger_finalise_start_stop )
	handle_.postFinalise().trigger( handle_ );
}


void uiMainWinBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    if ( handle_.closeOK() )
    {
	handle_.windowClosed.trigger( handle_ );
	ce->accept();

	if ( modal_ )
	    eventloop_.exit();
    }
    else
	ce->ignore();

     handle_.endCmdRecEvent( refnr, "Close" );
}


void uiMainWinBody::close()
{
    if ( !handle_.closeOK() ) return;

    handle_.windowClosed.trigger( handle_ );

    if ( testAttribute(Qt::WA_DeleteOnClose) )
    {
	QMainWindow::close();
	return;
    }

    if ( modal_ )
	eventloop_.exit();

    QMainWindow::hide();

    if ( exitapponclose_ )
	qApp->quit();
}


uiStatusBar* uiMainWinBody::uistatusbar()
{ return statusbar_; }

uiMenuBar* uiMainWinBody::uimenubar()
{ return menubar_; }


void uiMainWinBody::removeDockWin( uiDockWin* dwin )
{
    if ( !dwin ) return;

    removeDockWidget( dwin->qwidget() );
    dockwins_ -= dwin;
}


void uiMainWinBody::addDockWin( uiDockWin& dwin, uiMainWin::Dock dock )
{
    Qt::DockWidgetArea dwa = Qt::LeftDockWidgetArea;
    if ( dock == uiMainWin::Right ) dwa = Qt::RightDockWidgetArea;
    else if ( dock == uiMainWin::Top ) dwa = Qt::TopDockWidgetArea;
    else if ( dock == uiMainWin::Bottom ) dwa =
					     Qt::BottomDockWidgetArea;
    addDockWidget( dwa, dwin.qwidget() );
    if ( dock == uiMainWin::TornOff )
	dwin.setFloating( true );
    dockwins_ += &dwin;
}


void uiMainWinBody::toggleToolbar( CallBacker* cb )
{
    mDynamicCastGet( uiAction*, action, cb );
    if ( !action ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	if ( tb.name()==action->text() )
	    tb.display( tb.isHidden() );
    }
}


void uiMainWinBody::updateToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    const ObjectSet<uiAction>& items = toolbarsmnu_->actions();

    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	const uiToolBar& tb = *toolbars_[idx];
	uiAction& action = *const_cast<uiAction*>( items[idx] );
	if ( tb.name()==tb.name() )
	    action.setChecked( !tb.isHidden() );
    }
}


void uiMainWinBody::addToolBar( uiToolBar* tb )
{
    if ( toolbars_.isPresent(tb) )
	{ pErrMsg("Toolbar is already added"); return; }
    QMainWindow::addToolBar( (Qt::ToolBarArea)tb->prefArea(), tb->qwidget() );
    toolbars_ += tb;
    renewToolbarsMenu();
}


uiToolBar* uiMainWinBody::removeToolBar( uiToolBar* tb )
{
    if ( !toolbars_.indexOf(tb) )
	return 0;

    QMainWindow::removeToolBar( tb->qwidget() );
    toolbars_ -= tb;
    renewToolbarsMenu();
    return tb;
}


void uiMainWinBody::renewToolbarsMenu()
{
    if ( !toolbarsmnu_ ) return;

    for ( int idx=0; idx<toolbars_.size(); idx++ )
	toolbars_[idx]->setToolBarMenuAction( 0 );

    toolbarsmnu_->clear();
    for ( int idx=0; idx<toolbars_.size(); idx++ )
    {
	uiToolBar& tb = *toolbars_[idx];
	uiAction* itm =
	    new uiAction( tb.name(), mCB(this,uiMainWinBody,toggleToolbar) );
	toolbarsmnu_->insertItem( itm );
	tb.setToolBarMenuAction( itm );
	itm->setCheckable( true );
    }
}


static BufferString getSettingsFileName()
{
    BufferString fnm( "qtsettings_", (int)mODVersion );
    FilePath fp( GetSettingsDir(), fnm );
    const char* swusr = GetSoftwareUser();
    if ( swusr )
	fp.setExtension( swusr );
    return fp.fullPath();
}


void uiMainWinBody::saveSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    settings.setValue( "size", size() );
    settings.setValue( "pos", pos() );
    settings.setValue( "state", saveState() );
    settings.endGroup();
}


void uiMainWinBody::readSettings()
{
    const BufferString fnm = getSettingsFileName();
    QSettings settings( fnm.buf(), QSettings::IniFormat );
    settings.beginGroup( NamedObject::name().buf() );
    QSize qsz( settings.value("size", QSize(200,200)).toSize() );
    prefsz_ = uiSize( qsz.width(), qsz.height() );
    QPoint qpt( settings.value("pos", QPoint(200,200)).toPoint() );
    prefpos_.setXY( qpt.x(), qpt.y() );
    restoreState( settings.value("state").toByteArray() );
    settings.endGroup();

    updateToolbarsMenu();
}


void uiMainWinBody::setWindowTitle( const char* txt )
{
    QMainWindow::setWindowTitle(
		uiMainWin::uniqueWinTitle(txt,this,0).getQtString() );
}


#define mExecMutex( statements ) \
    activatemutex_.lock(); statements; activatemutex_.unLock();


void uiMainWinBody::activateInGUIThread( const CallBack& cb, bool busywait )
{
    CallBack* actcb = new CallBack( cb );
    mExecMutex( activatecbs_ += actcb );

    QEvent* guithreadev = new QEvent( mUsrEvGuiThread );
    QApplication::postEvent( this, guithreadev );

    float sleeptime = 0.01;
    while ( busywait )
    {
	mExecMutex( const int idx = activatecbs_.indexOf(actcb) );
	if ( idx < 0 )
	    break;

	Threads::sleep( sleeptime );
	if ( sleeptime < 1.28 )
	    sleeptime *= 2;
    }
}


void uiMainWinBody::keyPressEvent( QKeyEvent* ev )
{
    OD::KeyboardKey key = OD::KeyboardKey( ev->key() );
    OD::ButtonState modifier = OD::ButtonState( (int)ev->modifiers() );

    if ( ev && ev->key() == Qt::Key_F12 )
	handle_.translate();

    if ( key == OD::C && modifier == OD::ControlButton )
	handle_.ctrlCPressed.trigger();

    return QMainWindow::keyPressEvent( ev );
}


bool uiMainWinBody::event( QEvent* ev )
{
    if ( ev->type() == mUsrEvGuiThread )
    {
	mExecMutex( CallBack* actcb = activatecbs_[nractivated_++] );
	actcb->doCall( this );
	handle_.activatedone.trigger( actcb->cbObj() );
	mExecMutex( activatecbs_ -= actcb; nractivated_-- );
	delete actcb;
    }
    else if ( ev->type() == mUsrEvPopUpReady )
    {
	handle_.endCmdRecEvent( eventrefnr_, "WinPopUp" );
    }
    else
	return QMainWindow::event( ev );

    return true;
}


void uiMainWinBody::managePopupPos()
{
    uiParent* myparent = handle_.parent();
    uiMainWin* myparentsmw = myparent ? myparent->mainwin() : 0;
    if ( myparentsmw && !myparentsmw->isHidden() )
	return;

    uiMainWin* parwin = handle_.programmedActiveWindow();
    while ( parwin && parwin->isHidden() )
	parwin = parwin->parent() ? parwin->parent()->mainwin() : 0;

    if ( !parwin || moved_ )
	return;

    const uiRect pwrect = parwin->geometry( false );
    handle_.setCornerPos( pwrect.get(uiRect::Left), pwrect.get(uiRect::Top) );
    moved_ = false;
}


// ----- uiMainWin -----


uiMainWin::uiMainWin( uiParent* p, const uiMainWin::Setup& setup )
    : uiParent(setup.caption_,0)
    , body_(0)
    , parent_(p)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(setup.caption_)
{
    body_ = new uiMainWinBody( *this, p, setup.caption_, setup.modal_ );
    setBody( body_ );
    body_->construct( setup.nrstatusflds_, setup.withmenubar_ );
    body_->setWindowIconText(
	    setup.caption_.isEmpty() ? "OpendTect" : setup.caption_.buf() );
    body_->setAttribute( Qt::WA_DeleteOnClose, setup.deleteonclose_ );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
}


uiMainWin::uiMainWin( uiParent* parnt, const char* nm,
		      int nrstatusflds, bool withmenubar, bool modal )
    : uiParent(nm,0)
    , body_(0)
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(nm)
{
    body_ = new uiMainWinBody( *this, parnt, nm, modal );
    setBody( body_ );
    body_->construct( nrstatusflds, withmenubar );
    body_->setWindowIconText( nm && *nm ? nm : "OpendTect" );
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
}


uiMainWin::uiMainWin( const char* nm, uiParent* parnt )
    : uiParent(nm,0)
    , body_(0)
    , parent_(parnt)
    , popuparea_(Auto)
    , windowClosed(this)
    , activatedone(this)
    , ctrlCPressed(this)
    , caption_(nm)
{
    ctrlCPressed.notify( mCB(this,uiMainWin,copyToClipBoardCB) );
}


static Threads::Mutex		winlistmutex_;
static ObjectSet<uiMainWin>	orderedwinlist_;
static uiMainWin*		programmedactivewin_ = 0;

uiMainWin::~uiMainWin()
{
    if ( !body_->deletefrombody_ )
    {
	body_->deletefromod_ = true;
	delete body_;
    }

    winlistmutex_.lock();
    orderedwinlist_ -= this;

    if ( programmedactivewin_ == this )
	programmedactivewin_ = parent() ? parent()->mainwin() : 0;

    winlistmutex_.unLock();
}


QWidget* uiMainWin::qWidget() const
{ return body_; }

void uiMainWin::provideHelp( const char* winid )
{
    const BufferString fnm = HelpViewer::getURLForWinID( winid );
    mDefineStaticLocalObject( bool, shwonly,
			      = GetEnvVarYN("DTECT_SHOW_HELPINFO_ONLY") );
    if ( shwonly ) return;

    BufferString browser;
    Settings::common().get( "dTect.Browser", browser );
    if ( browser.isEmpty() )
	uiDesktopServices::openUrl( fnm );
    else
    {
	BufferString cmd( browser, " ", fnm );
	OS::ExecCommand( cmd, OS::RunInBG );
    }
}


void uiMainWin::showCredits( const char* winid )
{
    const BufferString fnm = HelpViewer::getCreditsURLForWinID( winid );
    uiDesktopServices::openUrl( fnm );
}

uiStatusBar* uiMainWin::statusBar()		{ return body_->uistatusbar(); }
uiMenuBar* uiMainWin::menuBar()			{ return body_->uimenubar(); }


#define mAddToOrderedWinList( uimw ) \
    winlistmutex_.lock(); \
    orderedwinlist_ -= uimw; \
    orderedwinlist_ += uimw; \
    winlistmutex_.unLock();

void uiMainWin::show()
{
    mAddToOrderedWinList( this );
    body_->go();
}


void uiMainWin::close()				{ body_->close(); }
void uiMainWin::reDraw(bool deep)		{ body_->reDraw(deep); }
bool uiMainWin::poppedUp() const		{ return body_->poppedUp(); }
bool uiMainWin::touch()			{ return body_->touch(); }
bool uiMainWin::finalised() const		{ return body_->finalised(); }
void uiMainWin::setExitAppOnClose( bool yn )	{ body_->exitapponclose_ = yn; }
void uiMainWin::showMaximized()			{ body_->showMaximized(); }
void uiMainWin::showMinimized()			{ body_->showMinimized(); }
void uiMainWin::showNormal()			{ body_->showNormal(); }
bool uiMainWin::isMaximized() const		{ return body_->isMaximized(); }
bool uiMainWin::isMinimized() const		{ return body_->isMinimized(); }
bool uiMainWin::isHidden() const		{ return body_->isHidden(); }
bool uiMainWin::isModal() const			{ return body_->isModal(); }

void uiMainWin::setCaption( const char* txt )
{
    caption_ = txt;
    body_->setWindowTitle( txt );
}


const char* uiMainWin::caption( bool unique ) const
{
    mDeclStaticString( capt );
    capt = unique ? body_->windowTitle() : caption_.buf();
    return capt;
}


void uiMainWin::setDeleteOnClose( bool yn )
{ body_->setAttribute( Qt::WA_DeleteOnClose, yn ); }


void uiMainWin::removeDockWindow( uiDockWin* dwin )
{ body_->removeDockWin( dwin ); }


void uiMainWin::addDockWindow( uiDockWin& dwin, Dock d )
{ body_->addDockWin( dwin, d ); }


void uiMainWin::addToolBar( uiToolBar* tb )
{ body_->addToolBar( tb ); }


uiToolBar* uiMainWin::removeToolBar( uiToolBar* tb )
{ return body_->removeToolBar( tb ); }


void uiMainWin::addToolBarBreak()
{ body_->addToolBarBreak(); }


uiMenu& uiMainWin::getToolbarsMenu() const
{ return body_->getToolbarsMenu(); }


const ObjectSet<uiToolBar>& uiMainWin::toolBars() const
{ return body_->toolBars(); }


const ObjectSet<uiDockWin>& uiMainWin::dockWins() const
{ return body_->dockWins(); }


uiGroup* uiMainWin::topGroup()		   { return body_->uiCentralWidg(); }


void uiMainWin::setShrinkAllowed(bool yn)
    { if ( topGroup() ) topGroup()->setShrinkAllowed(yn); }


bool uiMainWin::shrinkAllowed()
    { return topGroup() ? topGroup()->shrinkAllowed() : false; }


uiObject* uiMainWin::mainobject()
    { return body_->uiCentralWidg()->mainObject(); }


void uiMainWin::toStatusBar( const char* txt, int fldidx, int msecs )
{
    if ( !txt ) txt = "";
    uiStatusBar* sb = statusBar();
    if ( sb )
	sb->message( txt, fldidx, msecs );
    else if ( *txt )
	UsrMsg(txt);
}


void uiMainWin::setSensitive( bool yn )
{
    if ( menuBar() ) menuBar()->setSensitive( yn );
    body_->setEnabled( yn );
}


uiMainWin* uiMainWin::gtUiWinIfIsBdy(QWidget* mwimpl)
{
    if ( !mwimpl ) return 0;

    uiMainWinBody* _mwb = dynamic_cast<uiMainWinBody*>( mwimpl );
    if ( !_mwb ) return 0;

    return &_mwb->handle();
}


void uiMainWin::setCornerPos( int x, int y )
{ body_->move( x, y ); }


uiRect uiMainWin::geometry( bool frame ) const
{
    // Workaround for Qt-bug: top left of area sometimes translates to origin!
    QRect qarea = body_->geometry();
    QRect qframe = body_->frameGeometry();
    QPoint correction = body_->mapToGlobal(QPoint(0,0)) - qarea.topLeft();
    qframe.translate( correction );
    qarea.translate( correction );
    QRect qrect = frame ? qframe : qarea;

    //QRect qrect = frame ? body_->frameGeometry() : body_->geometry();
    uiRect rect( qrect.left(), qrect.top(), qrect.right(), qrect.bottom() );
    return rect;
}


void uiMainWin::setIcon( const ioPixmap& pm )
{ body_->setWindowIcon( *pm.qpixmap() ); }

void uiMainWin::setIconText( const char* txt)
{ body_->setWindowIconText( txt ); }

void uiMainWin::saveSettings()
{ body_->saveSettings(); }

void uiMainWin::readSettings()
{ body_->readSettings(); }

void uiMainWin::raise()
{ body_->raise(); }


void uiMainWin::programActiveWindow( uiMainWin* mw )
{ programmedactivewin_ = mw; }


uiMainWin* uiMainWin::programmedActiveWindow()
{ return programmedactivewin_; }

uiMainWin* uiMainWin::activeWindow()
{
    if ( programmedactivewin_ )
	return programmedactivewin_;

    QWidget* _aw = qApp->activeWindow();
    if ( !_aw )		return 0;

    uiMainWinBody* _awb = dynamic_cast<uiMainWinBody*>(_aw);
    if ( !_awb )	return 0;

    return &_awb->handle();
}


uiMainWin::ActModalTyp uiMainWin::activeModalType()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )					return None;

    if ( dynamic_cast<uiMainWinBody*>(amw) )	return Main;
    if ( dynamic_cast<QMessageBox*>(amw) )	return Message;
    if ( dynamic_cast<QFileDialog*>(amw) )	return File;
    if ( dynamic_cast<QColorDialog*>(amw) )	return Colour;
    if ( dynamic_cast<QFontDialog*>(amw) )	return Font;

    return Unknown;
}


uiMainWin* uiMainWin::activeModalWindow()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )	return 0;

    uiMainWinBody* mwb = dynamic_cast<uiMainWinBody*>( amw );
    if ( !mwb )	return 0;

    return &mwb->handle();
}


const char* uiMainWin::activeModalQDlgTitle()
{
    QWidget* amw = qApp->activeModalWidget();
    if ( !amw )
	return 0;

    mDeclStaticString( title );
    title = amw->windowTitle();
    return title;
}


#define mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut ) \
\
    int stdbutcount = 0; \
    QMessageBox::StandardButton stdbut = QMessageBox::NoButton; \
    for ( unsigned int idx=QMessageBox::Ok; \
	  qmb && idx<=QMessageBox::RestoreDefaults; idx+=idx ) \
    { \
	const QAbstractButton* abstrbut; \
        abstrbut = qmb->button( (QMessageBox::StandardButton) idx ); \
	if ( !abstrbut ) \
	    continue; \
	if ( stdbutcount == buttonnr ) \
	    stdbut = (QMessageBox::StandardButton) idx; \
	stdbutcount++; \
    }

// buttons() function to get all buttons only available from Qt4.5 :-(

const char* uiMainWin::activeModalQDlgButTxt( int buttonnr )
{
    const ActModalTyp typ = activeModalType();
    QWidget* amw = qApp->activeModalWidget();

    if ( typ == Message )
    {
	const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
	mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

	mDeclStaticString( buttext );
        if ( stdbut )
	    buttext = qmb->button(stdbut)->text();
	else if ( !stdbutcount )
	    buttext = qmb->buttonText( buttonnr );
	else
	    buttext = "";

	return buttext;
    }

    if ( typ==Colour || typ==Font || typ==File )
    {
	if ( buttonnr == 0 ) return "Cancel";
	if ( buttonnr == 1 ) return typ==File ? "Ok" : "OK";
	return "";
    }

    return 0;
}


int uiMainWin::activeModalQDlgRetVal( int buttonnr )
{
    QWidget* amw = qApp->activeModalWidget();
    const QMessageBox* qmb = dynamic_cast<QMessageBox*>( amw );
    mGetStandardButton( qmb, buttonnr, stdbutcount, stdbut );

    return stdbut ? ((int) stdbut) : buttonnr;
}


void uiMainWin::closeActiveModalQDlg( int retval )
{
    if ( activeModalWindow() )
	return;

    QWidget* _amw = qApp->activeModalWidget();
    if ( !_amw )
	return;

    QDialog* _qdlg = dynamic_cast<QDialog*>(_amw);
    if ( !_qdlg )
	return;

    _qdlg->done( retval );
}


void uiMainWin::getTopLevelWindows( ObjectSet<uiMainWin>& windowlist,
				    bool visibleonly )
{
    windowlist.erase();
    winlistmutex_.lock();
    for ( int idx=0; idx<orderedwinlist_.size(); idx++ )
    {
	if ( !visibleonly || !orderedwinlist_[idx]->isHidden() )
	    windowlist += orderedwinlist_[idx];
    }
    winlistmutex_.unLock();
}


void uiMainWin::getModalSignatures( BufferStringSet& signatures )
{
    signatures.erase();
    QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int idx=0; idx<toplevelwigs.count(); idx++ )
    {
	const QWidget* qw = toplevelwigs.at( idx );
	if ( qw->isWindow() && !qw->isHidden() && qw->isModal() )
	{
	    BufferString qwptrstr;
	    sprintf( qwptrstr.getCStr(), "%p", qw );
	    signatures.add( qwptrstr );
	}
    }
}


uiString uiMainWin::uniqueWinTitle( const uiString& txt,
				    QWidget* forwindow,
				    BufferString* outputaddendum )
{
    BufferString wintitle;
    const QWidgetList toplevelwigs = qApp->topLevelWidgets();

    for ( int count=1; true; count++ )
    {
	bool unique = true;
	wintitle = txt.getFullString();

	if ( wintitle.isEmpty() )
	    wintitle = "<no title>";

	if ( count>1 )
	{
	    BufferString addendum( "  {", toString(count), "}" );
	    wintitle += addendum.buf();
	    if ( outputaddendum ) *outputaddendum = addendum;
	}

	for ( int idx=0; idx<toplevelwigs.count(); idx++ )
	{
	    const QWidget* qw = toplevelwigs.at( idx );
	    if ( !qw->isWindow() || qw->isHidden() || qw==forwindow )
		continue;

	    if ( qw->windowTitle() == wintitle )
		unique = false;
	}

	if ( unique ) break;
    }

    return uiString(wintitle.str());
}


bool uiMainWin::grab( const char* filenm, int zoom,
		      const char* format, int quality ) const
{
    const WId desktopwinid = QApplication::desktop()->winId();
    const QPixmap desktopsnapshot = QPixmap::grabWindow( desktopwinid );
    QPixmap snapshot = desktopsnapshot;
    if ( zoom > 0 )
    {
	QWidget* qwin = qApp->activeModalWidget();
	if ( !qwin || zoom==1 )
	    qwin = body_;

	const int width = qwin->frameGeometry().width();
	const int height = qwin->frameGeometry().height();
	snapshot = desktopsnapshot.copy( qwin->x(), qwin->y(), width, height );
    }

    return snapshot.save( QString(filenm), format, quality );
}


void uiMainWin::activateInGUIThread( const CallBack& cb, bool busywait )
{ body_->activateInGUIThread( cb, busywait ); }


static void doTranslate( const uiBaseObject* obj )
{
    uiBaseObject* baseobj = const_cast<uiBaseObject*>( obj );
    mDynamicCastGet(uiObject*,uiobj,baseobj);
    mDynamicCastGet(uiGroupObj*,uigrpobj,baseobj);
    if ( !uigrpobj && uiobj ) uiobj->translate();

    if ( uigrpobj )
    {
	const ObjectSet<uiBaseObject>* children = uigrpobj->childList();
	for ( int idx=0; idx<children->size(); idx++ )
	    doTranslate( (*children)[idx] );
    }

    mDynamicCastGet(uiParent*,uipar,baseobj);
    if ( !uipar ) return;

    const ObjectSet<uiBaseObject>* children = uipar->childList();
    for ( int idx=0; idx<children->size(); idx++ )
	doTranslate( (*children)[idx] );
}


void uiMainWin::translate()
{
    doTranslate( body_->centralwidget_ );

    for ( int idx=0; idx<body_->toolbars_.size(); idx++ )
    {
	body_->toolbars_[idx]->translate();
    }

    for ( int idx=0; idx<body_->dockwins_.size(); idx++ )
	doTranslate( body_->dockwins_[idx] );

    if ( menuBar() )
	menuBar()->translate();
}


void uiMainWin::copyToClipBoardCB( CallBacker* )
{
    copyToClipBoard();
}

class ImageSaver : public CallBacker
{
public:

ImageSaver()
{
    timer_.tick.notify( mCB(this,ImageSaver,shootImageCB) );
}


void setImageProp( WId qwid, int w, int h, int r )
{
    qwinid_ = qwid;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = true;
    timer_.start( 500, true );
}


void setImageProp( WId qwid, const char* fnm, int w, int h, int r )
{
    qwinid_ = qwid;
    fname_ = fnm;
    width_ = w;
    height_ = h;
    res_ = r;
    copytoclipboard_ = false;
    timer_.start( 500, true );
}


protected:
void shootImageCB( CallBacker* )
{
    const QPixmap snapshot = QPixmap::grabWindow( qwinid_ );

    QImage image = snapshot.toImage();
    image = image.scaledToWidth( width_ );
    image = image.scaledToHeight( height_ );
    image.setDotsPerMeterX( (int)(res_/0.0254) );
    image.setDotsPerMeterY( (int)(res_/0.0254) );
    if ( copytoclipboard_ )
    {
	QClipboard* clipboard = QApplication::clipboard();
	clipboard->clear();
	clipboard->setImage( image );
    }
    else
	image.save( fname_ );
    timer_.stop();
}

    int		width_;
    int		height_;
    int		res_;
    bool	copytoclipboard_;
    QString	fname_;
    WId		qwinid_;
    Timer	timer_;
};


void uiMainWin::copyToClipBoard()
{
    QWidget* qwin = qWidget();
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    const int width = qwin->frameGeometry().width();
    const int height = qwin->frameGeometry().height();
    const int dpi = uiMain::getDPI();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, width, height, dpi );
}


void uiMainWin::saveImage( const char* fnm, int width, int height, int res )
{
    QWidget* qwin = qWidget();
    if ( !qwin )
	qwin = body_;
    WId wid = qwin->winId();
    mDefineStaticLocalObject( ImageSaver, imagesaver, );
    imagesaver.setImageProp( wid, fnm, width, height, res );
}


/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

*/

#define mHandle static_cast<uiDialog&>(handle_)

class uiDialogBody : public uiMainWinBody
{
public:
			uiDialogBody(uiDialog&,uiParent*,
				     const uiDialog::Setup&);
			~uiDialogBody();

    int			exec( bool showminimized );

    void		reject( CallBacker* s )
			{
			    mHandle.cancelpushed_ = s == cnclbut_;
			    if ( mHandle.rejectOK(s) )
				_done(0);
			    else
				uiSetResult( -1 );
			}
                        //!< to be called by a 'cancel' button
    void		accept( CallBacker* s )
			    { if ( mHandle.acceptOK(s) ) _done(1); }
                        //!< to be called by a 'ok' button
    void		done( int i )
			    { if ( mHandle.doneOK(i) ) _done(i); }

    void		uiSetResult( int v )	{ result_ = v; }
    int			uiResult()		{ return result_; }

    void		setTitleText(const char* txt);
    void		setOkText(const char*);
			//!< OK button disabled when set to empty
    void		setCancelText(const char*);
			//!< Cancel button disabled when set to empty
    void		setApplyText(const char*);
    void		enableSaveButton(const char* txt);
    void		setSaveButtonChecked(bool yn);
    void		setButtonSensitive(uiDialog::Button,bool yn);
    bool		saveButtonChecked() const;
    bool		hasSaveButton() const;
    uiButton*		button(uiDialog::Button);

			//! Separator between central dialog and Ok/Cancel bar?
    void		setSeparator( bool yn )	{ setup_.separator_ = yn; }
    bool		separator() const	{ return setup_.separator_; }
    void		setHelpID( const char* id ) { setup_.helpid_ = id; }
    const char*		helpID() const		{ return setup_.helpid_; }

    void		setDlgGrp( uiGroup* cw )	{ dlggrp_=cw; }

    void		setHSpacing( int spc )	{ dlggrp_->setHSpacing(spc); }
    void		setVSpacing( int spc )	{ dlggrp_->setVSpacing(spc); }
    void		setBorder( int b )	{ dlggrp_->setBorder( b ); }

    virtual void        addChild(uiBaseObject& child);
    virtual void        manageChld_(uiBaseObject&,uiObjectBody&);
    virtual void	attachChild(constraintType,uiObject* child,
				    uiObject* other,int margin,bool reciprocal);
    void		provideHelp(CallBacker*);
    void		showCredits(CallBacker*);
    void		doTranslate(CallBacker*);

    const uiDialog::Setup& getSetup() const	{ return setup_; }

protected:

    virtual const QWidget* managewidg_() const
			{
			    if ( !initing_ )
				return dlggrp_->pbody()->managewidg();
			    return uiMainWinBody::managewidg_();
			}

    int			result_;
    bool		initchildrendone_;

    uiGroup*            dlggrp_;
    uiDialog::Setup	setup_;

    uiPushButton*	okbut_;
    uiPushButton*	cnclbut_;
    uiPushButton*	applybut_;
    uiButton*		helpbut_;
    uiToolButton*	creditsbut_;

    uiCheckBox*		savebutcb_;
    uiToolButton*	savebuttb_;

    uiLabel*		titlelbl_;

    void		_done(int);

    virtual void	finalise()	{ finalise(false); }
    virtual void	finalise(bool);
    void		closeEvent(QCloseEvent*);

private:

    void		initChildren();
    uiObject*		createChildren();
    void		layoutChildren(uiObject*);
    void		layoutChildrenOld(uiObject*);

};


uiDialogBody::uiDialogBody( uiDialog& hndle, uiParent* parnt,
			    const uiDialog::Setup& s )
    : uiMainWinBody(hndle,parnt,s.wintitle_,s.modal_)
    , dlggrp_(0)
    , setup_(s)
    , okbut_(0), cnclbut_(0), applybut_(0)
    , savebutcb_(0),  savebuttb_(0)
    , helpbut_(0), creditsbut_(0)
    , titlelbl_(0), result_(0)
    , initchildrendone_(false)
{
    setContentsMargins( 10, 2, 10, 2 );
}


uiDialogBody::~uiDialogBody()
{
    if ( okbut_ )
	okbut_->activated.remove( mCB(this,uiDialogBody,accept) );

    if ( cnclbut_ )
	cnclbut_->activated.remove( mCB(this,uiDialogBody,reject) );
}



int uiDialogBody::exec( bool showminimized )
{
    uiSetResult( 0 );

    if ( setup_.fixedsize_ )
	setSizePolicy( QSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed) );

    move( handle_.getPopupArea() );
    go( showminimized );

    return uiResult();
}


void uiDialogBody::setTitleText( const char* txt )
{
    setup_.dlgtitle_ = txt;
    if ( titlelbl_ )
    {
	titlelbl_->setText(txt);
	uiObjectBody* tb = dynamic_cast<uiObjectBody*>( titlelbl_->body() );
	if ( tb && !tb->itemInited() )
	    titlelbl_->setPrefWidthInChar(
		    mMAX( tb->prefWidthInCharSet(), strlen(txt) + 2 ));
    }
}


void uiDialogBody::setOkText( const char* txt )
{
    setup_.oktext_ = txt;
    if ( okbut_ ) okbut_->setText(txt);
}


void uiDialogBody::setCancelText( const char* txt )
{
    setup_.canceltext_ = txt;
    if ( cnclbut_ ) cnclbut_->setText( txt );
}


void uiDialogBody::setApplyText( const char* txt )
{ if ( applybut_ ) applybut_->setText( txt ); }


bool uiDialogBody::hasSaveButton() const
{ return savebutcb_; }


bool uiDialogBody::saveButtonChecked() const
{ return savebutcb_ ? savebutcb_->isChecked() : false; }


/*! Hides the box, which also exits the event loop in case of a modal box.  */
void uiDialogBody::_done( int v )
{
    uiSetResult( v );
    close();
}


void uiDialogBody::closeEvent( QCloseEvent* ce )
{
    const int refnr = handle_.beginCmdRecEvent( "Close" );

    reject(0);
    if ( result_ == -1 )
	ce->ignore();
    else
	ce->accept();

    handle_.endCmdRecEvent( refnr, "Close" );
}


void uiDialogBody::enableSaveButton( const char* txt )
{ setup_.savetext_ = txt; setup_.savebutton_ = true; }

void uiDialogBody::setSaveButtonChecked( bool yn )
{
    setup_.savechecked_ = yn;
    if ( savebutcb_ ) savebutcb_->setChecked(yn);
}


void uiDialogBody::setButtonSensitive( uiDialog::Button but, bool yn )
{
    switch ( but )
    {
    case uiDialog::OK:		if ( okbut_ ) okbut_->setSensitive(yn);
    break;
    case uiDialog::CANCEL:	if ( cnclbut_ ) cnclbut_->setSensitive(yn);
    break;
    case uiDialog::APPLY:	if ( applybut_ ) applybut_->setSensitive(yn);
    break;
    case uiDialog::HELP:	if ( helpbut_ ) helpbut_->setSensitive(yn);
    break;
    case uiDialog::SAVE:
	if ( savebutcb_ ) savebutcb_->setSensitive(yn);
	if ( savebuttb_ ) savebuttb_->setSensitive(yn);
    break;
    case uiDialog::CREDITS:
	if ( creditsbut_ ) creditsbut_->setSensitive(yn);
    break;
    }
}


uiButton* uiDialogBody::button( uiDialog::Button but )
{
    switch ( but )
    {
    case uiDialog::OK:		return okbut_; break;
    case uiDialog::CANCEL:	return cnclbut_; break;
    case uiDialog::APPLY:	return applybut_; break;
    case uiDialog::HELP:	return helpbut_; break;
    case uiDialog::SAVE:
	return savebutcb_
	    ? (uiButton*)savebutcb_ : (uiButton*)savebuttb_;
    break;
    case uiDialog::CREDITS:	return creditsbut_; break;
    }

    return 0;
}


void uiDialogBody::addChild( uiBaseObject& child )
{
    if ( !initing_ )
	dlggrp_->addChild( child );
    else
	uiMainWinBody::addChild( child );
}


void uiDialogBody::manageChld_( uiBaseObject& o, uiObjectBody& b )
{
    if ( !initing_ )
	dlggrp_->manageChld( o, b );
}


void uiDialogBody::attachChild( constraintType tp, uiObject* child,
				uiObject* other, int margin, bool reciprocal )
{
    if ( !child || initing_ ) return;

    dlggrp_->attachChild( tp, child, other, margin, reciprocal );
}


/*!
    Construct OK and Cancel buttons just before the first show.
    This gives chance not to construct them in case OKtext and CancelText have
    been set to ""
*/
void uiDialogBody::finalise( bool )
{
    uiMainWinBody::finalise( false );

    handle_.preFinalise().trigger( handle_ );

    dlggrp_->finalise();

    if ( !initchildrendone_ )
	initChildren();

    finaliseChildren();

    handle_.postFinalise().trigger( handle_ );
}


void uiDialogBody::initChildren()
{
    uiObject* lowestobject = createChildren();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
	layoutChildrenOld( lowestobject );
    else
	layoutChildren( lowestobject );

    if ( okbut_ )
    {
	okbut_->activated.notify( mCB(this,uiDialogBody,accept) );
	okbut_->setDefault();
    }
    if ( cnclbut_ )
    {
	cnclbut_->activated.notify( mCB(this,uiDialogBody,reject) );
	if ( !okbut_ )
	    cnclbut_->setDefault();
    }

    initchildrendone_ = true;
}


uiObject* uiDialogBody::createChildren()
{
    if ( !setup_.oktext_.isEmpty() )
	okbut_ = new uiPushButton( centralwidget_, setup_.oktext_, true );
    if ( !setup_.canceltext_.isEmpty() )
	cnclbut_ = new uiPushButton( centralwidget_, setup_.canceltext_, true );
    if ( setup_.applybutton_ )
	applybut_ = new uiPushButton( centralwidget_, sApply(), true );

    if ( setup_.savebutton_ && !setup_.savetext_.isEmpty() )
    {
	if ( setup_.savebutispush_ )
	    savebuttb_ = new uiToolButton( centralwidget_, "save",
			  setup_.savetext_, CallBack() );
	else
	{
	    savebutcb_ = new uiCheckBox( centralwidget_, setup_.savetext_ );
	    savebutcb_->setChecked( setup_.savechecked_ );
	}
    }
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    const BufferString hid( dlg.helpID() );
    if ( !hid.isEmpty() && hid != "-" )
    {
	mDefineStaticLocalObject( bool, shwhid,
				  = GetEnvVarYN("DTECT_SHOW_HELP") );
#ifdef __debug__
	shwhid = true;
#endif
	if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
	{
	    helpbut_ = new uiToolButton( centralwidget_, "contexthelp",
			    shwhid ? hid.buf() : "Help on this window",
			    mCB(this,uiDialogBody,provideHelp) );
	    helpbut_->setPrefWidthInChar( 5 );
	}
	else
	{
	    helpbut_ = new uiPushButton( centralwidget_, sHelp(), true );
	    helpbut_->setToolTip( shwhid ? hid.buf() : "Help on this window" );
	    helpbut_->activated.notify( mCB(this,uiDialogBody,provideHelp) );
	}

	if ( dlg.haveCredits() )
	{
	    creditsbut_ = new uiToolButton( centralwidget_, "credits",
		    "Show credits", mCB(this,uiDialogBody,showCredits) );
	    creditsbut_->setPrefWidthInChar( 5 );
	}
    }

    if ( !setup_.menubar_ && !setup_.dlgtitle_.isEmpty() )
    {
	titlelbl_ = new uiLabel( centralwidget_, setup_.dlgtitle_ );
	uiObject* obj = setup_.separator_
			    ? (uiObject*) new uiSeparator(centralwidget_)
			    : (uiObject*) titlelbl_;

	if ( obj != titlelbl_ )
	{
	    if ( uiDialog::titlePos() == 0 )
		titlelbl_->attach( centeredAbove, obj );
	    else if ( uiDialog::titlePos() > 0 )
		titlelbl_->attach( rightBorder );
	    obj->attach( stretchedBelow, titlelbl_, -2 );
	}
	if ( setup_.mainwidgcentered_ )
	    dlggrp_->attach( centeredBelow, obj );
	else
	    dlggrp_->attach( stretchedBelow, obj );
    }

    uiObject* lowestobj = dlggrp_->mainObject();
    if ( setup_.separator_ && ( okbut_ || cnclbut_ || savebutcb_ ||
			       savebuttb_ || helpbut_) )
    {
	uiSeparator* horSepar = new uiSeparator( centralwidget_ );
	horSepar->attach( stretchedBelow, dlggrp_, -2 );
	lowestobj = horSepar;
    }

    return lowestobj;
}


static const int hborderdist = 1;
static const int vborderdist = 5;

static void attachButton( uiObject* but, uiObject*& prevbut,
			  uiObject* lowestobj )
{
    if ( !but ) return;

    but->attach( ensureBelow, lowestobj );
    but->attach( bottomBorder, vborderdist );
    if ( prevbut )
	but->attach( leftOf, prevbut );
    else
	but->attach( rightBorder, hborderdist );

    prevbut = but;
}


void uiDialogBody::layoutChildren( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;

    uiObject* prevbut = 0;
    attachButton( helpbut_, prevbut, lowestobj );
    attachButton( applybut_, prevbut, lowestobj );
    attachButton( rightbut, prevbut, lowestobj );
    attachButton( leftbut, prevbut, lowestobj );
    attachButton( creditsbut_, prevbut, lowestobj );

    uiObject* savebut = savebutcb_;
    if ( !savebut ) savebut = savebuttb_;
    if ( savebut )
    {
	savebut->attach( ensureBelow, lowestobj );
	savebut->attach( bottomBorder, vborderdist );
	savebut->attach( leftBorder, hborderdist );
    }
}


void uiDialogBody::layoutChildrenOld( uiObject* lowestobj )
{
    uiObject* leftbut = setup_.okcancelrev_ ? cnclbut_ : okbut_;
    uiObject* rightbut = setup_.okcancelrev_ ? okbut_ : cnclbut_;
    uiObject* exitbut = okbut_ ? okbut_ : cnclbut_;
    uiObject* centerbut = helpbut_;
    uiObject* extrabut = savebuttb_;

    if ( !okbut_ || !cnclbut_ )
    {
	leftbut = rightbut = 0;
	if ( exitbut )
	{
	    centerbut = exitbut;
	    extrabut = helpbut_;
	    leftbut = savebuttb_;
	}
    }

    if ( !centerbut )
    {
	centerbut = extrabut;
	extrabut = 0;
    }

#define mCommonLayout(but) \
    but->attach( ensureBelow, lowestobj ); \
    but->attach( bottomBorder, vborderdist )

    if ( leftbut )
    {
	mCommonLayout(leftbut);
	leftbut->attach( leftBorder, hborderdist );
    }

    if ( rightbut )
    {
	mCommonLayout(rightbut);
	rightbut->attach( rightBorder, hborderdist );
	if ( leftbut )
	    rightbut->attach( ensureRightOf, leftbut );
    }

    if ( centerbut )
    {
	mCommonLayout(centerbut);
	centerbut->attach( hCentered );
	if ( leftbut )
	    centerbut->attach( ensureRightOf, leftbut );
	if ( rightbut )
	    centerbut->attach( ensureLeftOf, rightbut );
    }

    if ( savebutcb_ )
    {
	savebutcb_->attach( extrabut ? leftOf : rightOf, exitbut );
	if ( centerbut && centerbut != exitbut )
	    centerbut->attach( ensureRightOf, savebutcb_ );
	if ( rightbut && rightbut != exitbut )
	    rightbut->attach( ensureRightOf, savebutcb_ );
    }

    if ( extrabut )
	extrabut->attach( rightOf, centerbut );
}


void uiDialogBody::provideHelp( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::provideHelp( dlg.helpID() );
}


void uiDialogBody::doTranslate( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    dlg.translate();
}


void uiDialogBody::showCredits( CallBacker* )
{
    mDynamicCastGet( uiDialog&, dlg, handle_ );
    uiMainWin::showCredits( dlg.helpID() );
}



// uiDialog
#define mBody static_cast<uiDialogBody*>(body_)

uiDialog::uiDialog( uiParent* p, const uiDialog::Setup& s )
	: uiMainWin( s.wintitle_, p )
	, cancelpushed_(false)
{
    body_= new uiDialogBody( *this, p, s );
    setBody( body_ );
    body_->construct( s.nrstatusflds_, s.menubar_ );
    uiGroup* cw = new uiGroup( body_->uiCentralWidg(), "Dialog central widget");
    cw->setStretch( 2, 2 );
    mBody->setDlgGrp( cw );
    setTitleText( s.dlgtitle_ );
    ctrlstyle_ = OkAndCancel;
}


void uiDialog::setButtonText( Button but, const char* txt )
{
    switch ( but )
    {
        case OK		: setOkText( txt ); break;
        case CANCEL	: setCancelText( txt ); break;
	case APPLY	: mBody->setApplyText( txt ); break;
        case HELP	: pErrMsg("set help txt but"); break;
	case SAVE	: enableSaveButton( txt ); break;
        case CREDITS	: pErrMsg("set credits txt but"); break;
    }
}


void uiDialog::setCtrlStyle( uiDialog::CtrlStyle cs )
{
    const char* oktext = sRun();
    const char* canceltext = sClose();
    if ( GetEnvVarYN("DTECT_OLD_BUTTON_LAYOUT") )
    {
	oktext = "&Go";
	canceltext = "&Dismiss";
    }

    switch ( cs )
    {
    case OkAndCancel:
	setOkText( sOk() );
	setCancelText( sCancel() );
    break;
    case RunAndClose:
	setOkText( oktext );
	setCancelText( canceltext );
    break;
    case CloseOnly:
	setOkText( mBody->finalised() ? canceltext : "" );
	setCancelText( canceltext );
    break;
    }

    ctrlstyle_ = cs;
}


void uiDialog::showMinMaxButtons()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowMinMaxButtonsHint;
    body_->setWindowFlags( flags );
}


void uiDialog::showAlwaysOnTop()
{
    Qt::WindowFlags flags = body_->windowFlags();
    flags |= Qt::WindowStaysOnTopHint;
    body_->setWindowFlags( flags );
}


bool uiDialog::haveCredits() const
{
    return HelpViewer::hasSpecificCredits( helpID() );
}


int uiDialog::go()
{
    mAddToOrderedWinList( this );
    return mBody->exec( false );
}


int uiDialog::goMinimized()
{
    mAddToOrderedWinList( this );
    return mBody->exec( true );
}


const uiDialog::Setup& uiDialog::setup() const	{ return mBody->getSetup(); }
void uiDialog::reject( CallBacker* cb)		{ mBody->reject( cb ); }
void uiDialog::accept( CallBacker*cb)		{ mBody->accept( cb ); }
void uiDialog::done( int i )			{ mBody->done( i ); }
void uiDialog::setHSpacing( int s )		{ mBody->setHSpacing(s); }
void uiDialog::setVSpacing( int s )		{ mBody->setVSpacing(s); }
void uiDialog::setBorder( int b )		{ mBody->setBorder(b); }
void uiDialog::setTitleText( const char* txt )	{ mBody->setTitleText(txt); }
void uiDialog::setOkText( const char* txt )	{ mBody->setOkText(txt); }
void uiDialog::setCancelText( const char* txt )	{ mBody->setCancelText(txt);}
void uiDialog::enableSaveButton(const char* t)  { mBody->enableSaveButton(t); }
uiButton* uiDialog::button(Button b)		{ return mBody->button(b); }
void uiDialog::setSeparator( bool yn )		{ mBody->setSeparator(yn); }
bool uiDialog::separator() const		{ return mBody->separator(); }
void uiDialog::setHelpID( const char* id )	{ mBody->setHelpID(id); }
const char* uiDialog::helpID() const		{ return mBody->helpID(); }
int uiDialog::uiResult() const			{ return mBody->uiResult(); }
void uiDialog::setModal( bool yn )		{ mBody->setModal( yn ); }
bool uiDialog::isModal() const			{ return mBody->isModal(); }

void uiDialog::setButtonSensitive(uiDialog::Button b, bool s )
    { mBody->setButtonSensitive(b,s); }
void uiDialog::setSaveButtonChecked(bool b)
    { mBody->setSaveButtonChecked(b); }
bool uiDialog::saveButtonChecked() const
    { return mBody->saveButtonChecked(); }
bool uiDialog::hasSaveButton() const
    { return mBody->hasSaveButton(); }
void uiDialog::setCaption( const char* txt )
    { caption_ = txt; mBody->setWindowTitle( txt ); }

int uiDialog::titlepos_ = 0; // default is centered.
int uiDialog::titlePos()			{ return titlepos_; }
void uiDialog::setTitlePos( int p )		{ titlepos_ = p; }

