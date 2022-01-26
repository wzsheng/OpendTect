/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uisynthgendlg.h"

#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uisplitter.h"
#include "uisynthseis.h"
#include "uitoolbutton.h"

#include "ctxtioobj.h"
#include "file.h"
#include "instantattrib.h"
#include "od_helpids.h"
#include "seistrctr.h"
#include "stratsynth.h"
#include "syntheticdataimpl.h"
#include "synthseis.h"


#define mErrRet(s,act) \
{ \
    uiMsgMainWinSetter mws( mainwin() ); \
    if ( !s.isEmpty() ) \
	uiMSG().error(s); \
    act; \
}

uiSynthParsGrp::uiSynthParsGrp( uiParent* p, StratSynth& gp )
    : uiGroup(p,"Synthetic Seismic Parameters")
    , stratsynth_(gp)
    , synthAdded(this)
    , synthChanged(this)
    , synthRemoved(this)
    , synthDisabled(this)
    , elPropSel(this)
{
    auto* leftgrp = new uiGroup( this, "left group" );
    auto* butgrp = new uiButtonGroup( leftgrp, "actions", OD::Horizontal );

    new uiToolButton( butgrp, "defraytraceprops",
		      tr("Specify inputs for synthetic creation"),
		      mCB(this,uiSynthParsGrp,elPropSelCB) );
    new uiToolButton( butgrp, "new", uiStrings::sNew(),
				     mCB(this,uiSynthParsGrp,newCB) );
    new uiToolButton( butgrp, "open", uiStrings::sOpen(),
				      mCB(this,uiSynthParsGrp,openCB) );
    new uiToolButton( butgrp, "save", uiStrings::sSave(),
				      mCB(this,uiSynthParsGrp,saveCB) );
    new uiToolButton( butgrp, "saveas", uiStrings::sSaveAs(),
				      mCB(this,uiSynthParsGrp,saveAsCB) );
    /*TODO: ensure we can open/save not only directly the synthetic data
      parameters, but also the ones saved within SynthRock simulations */

    auto* syntlistgrp = new uiGroup( leftgrp, "Synthetics List" );
    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Synthetics"),
			 uiListBox::AboveMid );
    synthnmlb_ = new uiListBox( syntlistgrp, su );
    synthnmlb_->setHSzPol( uiObject::SmallVar );
    mAttachCB( synthnmlb_->selectionChanged, uiSynthParsGrp::newSynthSelCB );
    updatefld_ = new uiPushButton( syntlistgrp, tr("Update selected"),
			mCB(this,uiSynthParsGrp,updateSyntheticsCB), true );
    updatefld_->attach( leftAlignedBelow, synthnmlb_ );
    removefld_ = new uiPushButton( syntlistgrp, tr("Remove selected"),
			mCB(this,uiSynthParsGrp,removeSyntheticsCB), true );
    removefld_->attach( rightOf, updatefld_ );

    auto* rightgrp = new uiGroup( this, "Parameter Group" );
    rightgrp->setStretch( 1, 1 );

    auto* toppargrp = new uiGroup( rightgrp, "Parameter Group - top part" );
    BufferStringSet types( SynthGenParams::SynthTypeNames() );
    const int stratpropidx =
	types.indexOf( SynthGenParams::toString(SynthGenParams::StratProp) );
    types.removeSingle( stratpropidx );
    auto* lblcbx =
	new uiLabeledComboBox( toppargrp, types, tr("Synthetic type") );
    typefld_ = lblcbx->box();
    mAttachCB( typefld_->selectionChanged, uiSynthParsGrp::typeChg );

    psselfld_ = new uiLabeledComboBox( toppargrp, tr("Input Prestack") );
    psselfld_->attach( alignedBelow, lblcbx );

    inpselfld_ = new uiLabeledComboBox( toppargrp, uiStrings::sInput() );
    inpselfld_->attach( alignedBelow, lblcbx );
    mAttachCB(inpselfld_->box()->selectionChanged, uiSynthParsGrp::parsChanged);


    EnumDef attribs = Attrib::Instantaneous::OutTypeDef();
    attribs.remove( attribs.getKeyForIndex(Attrib::Instantaneous::RotatePhase));
    instattribfld_ = new uiLabeledListBox( toppargrp, uiStrings::sAttribute(),
					    OD::ChooseAtLeastOne );
    instattribfld_->box()->addItems( attribs.strings() );
    instattribfld_->attach( alignedBelow, inpselfld_ );
    mAttachCB(instattribfld_->box()->selectionChanged,
						  uiSynthParsGrp::parsChanged);

    FloatInpIntervalSpec finpspec(false);
    finpspec.setLimits( Interval<float>(0,90) );
    finpspec.setDefaultValue( Interval<float>(0,30) );
    angleinpfld_ = new uiGenInput( toppargrp, tr("Angle range"), finpspec );
    angleinpfld_->attach( alignedBelow, psselfld_ );
    mAttachCB( angleinpfld_->valuechanged, uiSynthParsGrp::parsChanged );

    uiRayTracer1D::Setup rsu;
    rsu.dooffsets(true).convertedwaves(true).showzerooffsetfld(false);
    synthseis_ = new uiSynthSeisGrp( toppargrp, rsu );
    synthseis_->attach( alignedBelow, lblcbx );
    toppargrp->setHAlignObj( synthseis_ );

    butgrp->attach( leftAlignedAbove, syntlistgrp );

    auto* botpargrp = new uiGroup( rightgrp, "Parameter Group - Last Part" );
    botpargrp->attach( alignedBelow, toppargrp );

    namefld_ = new uiGenInput( botpargrp, uiStrings::sName() );
    namefld_->setElemSzPol( uiObject::Wide );
    mAttachCB( namefld_->valuechanged, uiSynthParsGrp::nameChanged );
    botpargrp->setHAlignObj( namefld_ );

    addnewfld_ = new uiPushButton( botpargrp, tr("Add as new"),
			mCB(this,uiSynthParsGrp,addSyntheticsCB), true );
    addnewfld_->attach( alignedBelow, namefld_ );

    auto* splitter = new uiSplitter( this, "Splitter", true );
    splitter->addGroup( leftgrp );
    splitter->addGroup( rightgrp );

    mAttachCB( postFinalise(), uiSynthParsGrp::initGrp );
}


uiSynthParsGrp::~uiSynthParsGrp()
{
    detachAllNotifiers();
}


void uiSynthParsGrp::initGrp( CallBacker* )
{
    NotifyStopper ns( synthnmlb_->selectionChanged );
    synthnmlb_->setEmpty();
    BufferStringSet synthnms;
    for ( const auto* sd : stratsynth_.synthetics() )
    {
	if ( !sd->isStratProp() )
	    synthnms.add( sd->name().buf() );
    }

    synthnmlb_->addItems( synthnms );
    ns.enableNotification();
    if ( !synthnmlb_->isEmpty() )
	synthnmlb_->setCurrentItem( 0 );

    newSynthSelCB( nullptr );
    //Keep last, as we do not want earlier notifications:
    mAttachCB( synthseis_->parsChanged, uiSynthParsGrp::parsChanged );
}


void uiSynthParsGrp::getPSNames( BufferStringSet& synthnms )
{
    synthnms.erase();
    stratsynth_.getSyntheticNames( synthnms, SynthGenParams::PreStack );
    for ( int idx=synthnms.size(); idx>=0; idx-- )
    {
	SyntheticData* sd = stratsynth_.getSynthetic( synthnms.get(idx) );
	bool donmo = true;
	if ( sd &&
	     sd->getGenParams().raypars_.getYN(Seis::SynthGenBase::sKeyNMO(),
					       donmo) && donmo )
	    synthnms.add( sd->name() );
    }
}


void uiSynthParsGrp::getInpNames( BufferStringSet& synthnms )
{
    synthnms.erase();

    for ( int synthidx=0; synthidx<stratsynth_.nrSynthetics(); synthidx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( synthidx );
	if ( sd && sd->getGenParams().canBeAttributeInput() )
	    synthnms.add( sd->name() );
    }
}


void uiSynthParsGrp::parsChanged( CallBacker* )
{
    if ( !getFromScreen() )
	return;

    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
    updatefld_->setSensitive( true );
    addnewfld_->setSensitive( true );
}


void uiSynthParsGrp::nameChanged( CallBacker* )
{
    stratsynth_.genParams().name_ = namefld_->text();
    updatefld_->setSensitive( true );
    addnewfld_->setSensitive( true );
}


bool uiSynthParsGrp::prepareSyntheticToBeChanged( bool toberemoved )
{
    if ( synthnmlb_->size()==1 && toberemoved )
	mErrRet( tr("Cannot remove all synthetics"), return false );

    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	mErrRet( tr("No synthetic selected"), return false );

    const BufferString synthtochgnm( synthnmlb_->getText() );
    const SyntheticData* sdtochg = stratsynth_.getSynthetic( synthtochgnm );
    if ( !sdtochg )
	mErrRet( tr("Cannot find synthetic data '%1'").arg(synthtochgnm),
		 return false );

    BufferStringSet synthstobedisabled;
    const SynthGenParams& sgptorem = stratsynth_.genParams();
    for ( const auto* sd : stratsynth_.synthetics() )
    {
	if ( !sd->isAngleStack() && !sd->isAVOGradient() && !sd->isAttribute() )
	    continue;

	const SynthGenParams& sgp = sd->getGenParams();
	if ( sgptorem.isPreStack() && sgp.isPSBased() &&
	     sgp.inpsynthnm_ == sgptorem.name_ )
	    synthstobedisabled.add( sgp.name_ );
    }

    if ( !synthstobedisabled.isEmpty() )
    {
	uiString chgstr = toberemoved ? tr( "remove" ) : tr( "change" );
	uiString msg = tr("%1 will become uneditable as it is dependent on "
			  "'%2'.\n\nDo you want to %3 the synthetics?")
			  .arg(synthstobedisabled.getDispString())
			  .arg(sgptorem.name_.buf()).arg(chgstr);
	if ( !uiMSG().askGoOn(msg) )
	    return false;

	for ( int idx=0; idx<synthstobedisabled.size(); idx++ )
	{
	    const BufferString& synthnm = synthstobedisabled.get( idx );
	    synthDisabled.trigger( synthnm );
	}
    }

    return true;
}


bool uiSynthParsGrp::checkSyntheticName( bool isupdate )
{
    const SynthGenParams& gp = stratsynth_.genParams();

    if ( gp.name_ == SynthGenParams::sKeyInvalidInputPS() )
	mErrRet( tr("Please enter a different name"), return false);

    if ( !isupdate && synthnmlb_->isPresent(gp.name_) )
    {
	uiString msg = tr("Synthetic data of name '%1' is already present. "
			  "Please choose a different name" )
		     .arg(gp.name_);
	mErrRet( msg, return false );
    }

    return true;
}


bool uiSynthParsGrp::doAddSynthetic( bool isupdate )
{

    checkSyntheticName( isupdate );

    MouseCursorChanger mcchger( MouseCursor::Wait );
    SyntheticData* sd = stratsynth_.addSynthetic( stratsynth_.genParams() );
    if ( !sd )
	mErrRet( stratsynth_.errMsg(), return false )

    return true;
}


void uiSynthParsGrp::newSynthSelCB( CallBacker* )
{
    const BufferString synthnm( synthnmlb_->getText() );
    SynthGenParams& sgp = stratsynth_.genParams();
    if ( synthnm.isEmpty() )
    {
	sgp.setWavelet( synthseis_->getWaveletName() );
	sgp.createName( sgp.name_ );
    }
    else
    {
	SyntheticData* sd = stratsynth_.getSynthetic( synthnm.buf() );
	if ( sd )
	    sgp = sd->getGenParams();
    }

    putToScreen();
    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( synthnmlb_->size() < 1 );
}


void uiSynthParsGrp::updateSyntheticsCB( CallBacker* )
{
    const int selidx = synthnmlb_->currentItem();
    if ( selidx<0 )
	return;

    if ( !prepareSyntheticToBeChanged(false) )
	return;

    const BufferString synthtochgnm( synthnmlb_->getText() );
    const SyntheticData* sdtochg = stratsynth_.getSynthetic( synthtochgnm );
    const SynthGenParams& sdtochgsgp = sdtochg->getGenParams();
    const SynthGenParams& cursgp = stratsynth_.genParams();
    if ( cursgp == sdtochgsgp )
    {
	updatefld_->setSensitive( false );
	addnewfld_->setSensitive( false );
	return;
    }

    if ( !getFromScreen() )
	return;

    const BufferString synthname( synthnmlb_->getText() );
    if ( !stratsynth_.removeSynthetic(synthname) )
	mErrRet( stratsynth_.errMsg(), return )

    if ( !doAddSynthetic(true) )
    {
	synthnmlb_->removeItem( synthname );
	mErrRet( stratsynth_.errMsg(), return )
    }

    const BufferString& newsynthnm = stratsynth_.genParams().name_;
    for ( int idx=0; idx<stratsynth_.nrSynthetics(); idx++ )
    {
	SyntheticData* sd = stratsynth_.getSyntheticByIdx( idx );
	if ( !sd || (!sd->isAngleStack() && !sd->isAVOGradient() &&
		     !sd->isAttribute()) )
	    continue;

	const SynthGenParams& sgp = sd->getGenParams();
	if ( sgp.inpsynthnm_ != synthname )
	    continue;

	SynthGenParams newsgp( sgp );
	newsgp.inpsynthnm_.set( newsynthnm );
	sd->useGenParams( newsgp );
    }

    synthnmlb_->setItemText( selidx, newsynthnm );
    updatefld_->setSensitive( false );
    addnewfld_->setSensitive( false );
    synthChanged.trigger( stratsynth_.genParams().name_ );
}


void uiSynthParsGrp::removeSyntheticsCB( CallBacker* )
{
    if ( !prepareSyntheticToBeChanged(true) )
	return;

    const BufferString synthtoremnm( stratsynth_.genParams().name_ );
    stratsynth_.removeSynthetic( synthtoremnm );
    synthRemoved.trigger( synthtoremnm );

    synthnmlb_->removeItem( synthnmlb_->indexOf(synthtoremnm) );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
}


void uiSynthParsGrp::addSyntheticsCB( CallBacker* )
{
    if ( !getFromScreen() )
	return;

    SynthGenParams& gp = stratsynth_.genParams();
    if ( gp.isAttribute() )
    {
	BufferStringSet attribs;
	instattribfld_->box()->getChosen( attribs );
	for ( const auto* attrib : attribs )
	{
	    Attrib::Instantaneous::parseEnum( *attrib, gp.attribtype_ );
	    gp.createName( gp.name_ );
	    if ( !checkSyntheticName() )
		return;
	}
	MouseCursorChanger mcchger( MouseCursor::Wait );
	if ( !stratsynth_.addInstAttribSynthetics( attribs, gp ) )
	    mErrRet( stratsynth_.errMsg(), return );

	for ( const auto* attrib : attribs )
	{
	    Attrib::Instantaneous::parseEnum( *attrib, gp.attribtype_ );
	    gp.createName( gp.name_ );
	    synthAdded.trigger( gp.name_ );
	    NotifyStopper ns( synthnmlb_->selectionChanged );
	    synthnmlb_->addItem( gp.name_ );
	}
	synthnmlb_->selectionChanged.trigger();
    }
    else
    {
	if ( !doAddSynthetic() )
	    return;

	const BufferString& newsynthnm = gp.name_;
	synthAdded.trigger( newsynthnm );
	NotifyStopper ns( synthnmlb_->selectionChanged );
	synthnmlb_->addItem( newsynthnm );
    }

    updatefld_->setSensitive( false );
    removefld_->setSensitive( synthnmlb_->size() > 1 );
    addnewfld_->setSensitive( false );
}


void uiSynthParsGrp::updateFieldDisplay()
{
    SynthGenParams::SynthType synthtype =
		SynthGenParams::parseEnumSynthType( typefld_->text() );
    const bool psbased = synthtype == SynthGenParams::AngleStack ||
			 synthtype == SynthGenParams::AVOGradient;
    const bool attrib = synthtype == SynthGenParams::InstAttrib;
    synthseis_->display( !psbased && !attrib );
    psselfld_->display( psbased );
    instattribfld_->display( attrib );
    inpselfld_->display( attrib );
    angleinpfld_->display( psbased );
}


void uiSynthParsGrp::fillPar( IOPar& par ) const
{
    stratsynth_.fillPar( par );
}


bool uiSynthParsGrp::usePar( const IOPar& par )
{
    const bool res = stratsynth_.usePar( par );
    if ( !res )
	return false;

    for ( int idx=synthnmlb_->size()-1; idx>=0; idx-- )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	synthRemoved.trigger( synthnm );
    }

    initGrp( nullptr );
    for ( int idx=0; idx<synthnmlb_->size(); idx++ )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	const SyntheticData* sd = stratsynth_.getSynthetic( synthnm.buf() );
	if ( sd )
	    synthAdded.trigger( synthnm );
    }

    return res;
}


bool uiSynthParsGrp::confirmSave()
{
    const int dosave = uiMSG().askSave( tr("%1 are not saved. Save now?")
					.arg( uiStrings::sParameter(mPlural)));
    if ( dosave == -1 )
	return false;
    else if ( dosave == 1 )
	saveCB( nullptr );
    return true;
}


void uiSynthParsGrp::elPropSelCB( CallBacker* )
{
    elPropSel.trigger();
}


void uiSynthParsGrp::newCB( CallBacker* )
{
    if ( !confirmSave() )
	return;

    const SyntheticData* defsd = stratsynth_.addDefaultSynthetic();

    bool newtriggered = false;
    for ( int idx=synthnmlb_->size()-1; idx>=0; idx-- )
    {
	const BufferString synthnm( synthnmlb_->textOfItem( idx ) );
	if ( stratsynth_.removeSynthetic(synthnm) )
	{
	    if ( defsd && synthnm == defsd->name() )
	    {
		synthChanged.trigger( synthnm );
		newtriggered = true;
	    }
	    else
		synthRemoved.trigger( synthnm );
	}
    }

    if ( defsd && !newtriggered )
	synthAdded.trigger( BufferString(defsd->name()) );

    initGrp( nullptr );
}


void uiSynthParsGrp::openCB( CallBacker* )
{
    if ( !confirmSave() )
	return;

    IOPar par;
    const IOObjContext ctio( mIOObjContext(SyntheticDataPars) );
    uiIOObjSelDlg dlg( this, uiStrings::phrOpen( tr("synthetic parameters") ),
		       ctio );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return;

    if ( !par.read(dlg.ioObj()->mainFileName(),
		   mTranslGroupName(SyntheticDataPars)) )
	return;

    stratsynth_.clearSynthetics( true );
    if ( usePar(par) )
	lastsavedfnm_.set( dlg.ioObj()->mainFileName() );
}


void uiSynthParsGrp::saveCB( CallBacker* )
{
    if ( !lastsavedfnm_.isEmpty() && File::exists(lastsavedfnm_.str()) )
    {
	doSave( lastsavedfnm_.str() );
	return;
    }

    saveAsCB( nullptr );
}


void uiSynthParsGrp::saveAsCB( CallBacker* )
{
    IOObjContext ctio( mIOObjContext(SyntheticDataPars) );
    ctio.forread_ = false;
    uiIOObjSelDlg dlg( this, uiStrings::phrSave( tr("synthetic parameters") ),
		       ctio );
    if ( dlg.go() != uiDialog::Accepted || !dlg.ioObj() )
	return;

    doSave( dlg.ioObj()->mainFileName() );
}


bool uiSynthParsGrp::doSave( const char* fnm )
{
    IOPar par;
    fillPar( par );
    const bool res = par.write( fnm, mTranslGroupName(SyntheticDataPars) );
    lastsavedfnm_.set( fnm );
    return res;
}


void uiSynthParsGrp::typeChg( CallBacker* )
{
    const SynthGenParams::SynthType synthtype =
			SynthGenParams::parseEnumSynthType( typefld_->text() );
    SynthGenParams& sgp = stratsynth_.genParams();
    const BufferString wvltnm( sgp.getWaveletNm() );
    sgp = SynthGenParams( synthtype );
    if ( !wvltnm.isEmpty() )
    {
	sgp.setWavelet( wvltnm );
	sgp.createName( sgp.name_ );
    }

    putToScreen();
    updatefld_->setSensitive( !synthnmlb_->isEmpty() );
    addnewfld_->setSensitive( true );
}


void uiSynthParsGrp::putToScreen()
{
    NotifyStopper parschgstopper( synthseis_->parsChanged );
    NotifyStopper angparschgstopper( angleinpfld_->valuechanged );
    const SynthGenParams& genparams = stratsynth_.genParams();
    typefld_->setCurrentItem( SynthGenParams::toString(genparams.synthtype_) );
    synthseis_->setWavelet( genparams.getWaveletNm() );

    if ( genparams.isPSBased() )
    {
	BufferStringSet psnms;
	getPSNames( psnms );
	psselfld_->box()->setEmpty();
	if ( psnms.isPresent(genparams.inpsynthnm_) ||
	     genparams.inpsynthnm_.isEmpty() )
	{
	    psselfld_->box()->addItems( psnms );
	    psselfld_->box()->setCurrentItem( genparams.inpsynthnm_ );
	    psselfld_->box()->setSensitive( true );
	}
	else
	{
	    psselfld_->box()->addItem( toUiString(genparams.inpsynthnm_) );
	    psselfld_->box()->setSensitive( false );
	}

	angleinpfld_->setValue( genparams.anglerg_ );
    }
    else if ( genparams.isAttribute() )
    {
	NotifyStopper ns_inpsel( inpselfld_->box()->selectionChanged );
	NotifyStopper ns_instattrib( instattribfld_->box()->selectionChanged );
	BufferStringSet inpnms;
	getInpNames( inpnms );
	inpselfld_->box()->setEmpty();
	if ( inpnms.isPresent(genparams.inpsynthnm_) ||
	     genparams.inpsynthnm_.isEmpty() )
	{
	    inpselfld_->box()->addItems( inpnms );
	    inpselfld_->box()->setCurrentItem( genparams.inpsynthnm_ );
	    inpselfld_->box()->setSensitive( true );
	    if ( genparams.inpsynthnm_.isEmpty() )
	    {
		SynthGenParams& gp = stratsynth_.genParams();
		gp.inpsynthnm_.set(inpselfld_->box()->text());
		BufferString nm;
		gp.createName( nm );
		gp.name_.set( nm );
	    }
	}
	else
	{
	    inpselfld_->box()->addItem( toUiString(genparams.inpsynthnm_) );
	    inpselfld_->box()->setSensitive( false );
	}
	instattribfld_->box()->chooseAll( false );
	instattribfld_->box()->setChosen( genparams.attribtype_ );
    }
    else
	synthseis_->usePar( genparams.raypars_ );

    namefld_->setText( genparams.name_ );
    updateFieldDisplay();
}


bool uiSynthParsGrp::getFromScreen()
{
    const BufferString nm( namefld_->text() );
    if ( nm.isEmpty() )
	mErrRet(tr("Please specify a valid name"),return false);

    SynthGenParams& sgp = stratsynth_.genParams();
    const SynthGenParams::SynthType synthtype =
			SynthGenParams::parseEnumSynthType(typefld_->text() );
    const SynthGenParams sgptype( synthtype );

    if ( sgptype.isPSBased() )
    {
	if ( psselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot generate an angle stack synthetics without "
			"any NMO corrected Prestack."), return false );

	if ( !psselfld_->box()->sensitive() )
	    mErrRet( tr("Cannot change synthetic data as the dependent "
			"prestack synthetic data has already been removed"),
			return false );

	SyntheticData* inppssd = stratsynth_.getSynthetic(
		psselfld_->box()->textOfItem(psselfld_->box()->currentItem()));
	if ( !inppssd )
	    mErrRet( tr("Problem with Input Prestack synthetic data"),
		     return false);

	sgp = SynthGenParams( synthtype );
	sgp.name_.set( nm );
	sgp.inpsynthnm_.set( inppssd->name() );
	sgp.anglerg_ = angleinpfld_->getFInterval();
	return true;
    }
    else if ( sgptype.isAttribute() )
    {
	if ( inpselfld_->box()->isEmpty() )
	    mErrRet( tr("Cannot generate attributes without "
			"any post stack synthetics."), return false );

	if ( !inpselfld_->box()->sensitive() )
	    mErrRet( tr("Cannot change synthetic data as the dependent "
			"poststack synthetic data has already been removed"),
			return false );

	SyntheticData* inpsd = stratsynth_.getSynthetic(
	    inpselfld_->box()->textOfItem(inpselfld_->box()->currentItem()) );
	if ( !inpsd )
	    mErrRet( tr("Problem with Input synthetic data"), return false);

	sgp = SynthGenParams( synthtype );
	sgp.name_.set( nm );
	sgp.inpsynthnm_.set( inpsd->name() );
	sgp.attribtype_ =
	(Attrib::Instantaneous::OutType) instattribfld_->box()->firstChosen();
	return true;
    }

    IOPar iop;
    synthseis_->fillPar( iop );
    sgp.usePar( iop );
    sgp.setWavelet( synthseis_->getWaveletName() );
    sgp.name_.set( nm );

    return true;
}


void uiSynthParsGrp::updateWaveletName()
{
    synthseis_->setWavelet( stratsynth_.genParams().getWaveletNm() );
    BufferString nm;
    stratsynth_.genParams().createName( nm );
    namefld_->setText( nm );
}


//uiSynthGenDlg

uiSynthGenDlg::uiSynthGenDlg( uiParent* p, StratSynth& gp )
    : uiDialog(p,uiDialog::Setup(tr("Specify Synthetic Parameters"),
				mNoDlgTitle,
				 mODHelpKey(mRayTrcParamsDlgHelpID) )
				.modal(false))
{
    setForceFinalise( true );
    setCtrlStyle( CloseOnly );

    uisynthparsgrp_ = new uiSynthParsGrp( this, gp );
}


uiSynthGenDlg::~uiSynthGenDlg()
{
}
