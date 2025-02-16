/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Dec 2003
-*/


#include "rockphysics.h"

#include "ascstream.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "mathproperty.h"
#include "safefileio.h"
#include "separstr.h"
#include "unitofmeasure.h"

static const char* filenamebase = "RockPhysics";
static const char* sKeyDef = "Formula";
static const char* sKeyVar = "Variable";
static const char* sKeyConst = "Constant";
static const char* sKeyRg = "Typical Range";


RockPhysics::Formula::Formula( const Mnemonic& mn, const char* nm )
    : Math::Formula()
{
    setName( nm );
    setOutputMnemonic( &mn );
    typicalrgs_.setNullAllowed();
}


RockPhysics::Formula::Formula( const Formula& oth )
    : Math::Formula(oth)
{
    typicalrgs_.setNullAllowed();
    *this = oth;
}


RockPhysics::Formula::~Formula()
{
    deepErase( typicalrgs_ );
}


RockPhysics::Formula& RockPhysics::Formula::operator =( const Formula& oth )
{
    if ( this != &oth )
    {
	Math::Formula::operator=( oth );
	src_ = oth.src_;
	constantnms_ = oth.constantnms_;
	deepCopy( typicalrgs_, oth.typicalrgs_ );
    }

    return *this;
}


bool RockPhysics::Formula::operator ==( const Formula& oth ) const
{
    if ( Math::Formula::operator !=(oth) ||
	 constantnms_ != oth.constantnms_ ||
	 typicalrgs_.size() != oth.typicalrgs_.size() )
	return false;

    return true;
}


bool RockPhysics::Formula::operator !=( const Formula& oth ) const
{
    return !(*this == oth);
}


bool RockPhysics::Formula::hasSameForm( const Math::Formula& form ) const
{
    return Math::Formula::operator ==( form );
}


RockPhysics::Formula* RockPhysics::Formula::get( const IOPar& iop )
{
    auto* fm = new Formula( Mnemonic::undef() );
    if ( !fm->usePar(iop) )
	{ delete fm; return nullptr; }

    return fm;
}


static BufferString getStrFromFMS( const char* inp )
{
    SeparString fms( inp, '`' );
    fms.setSepChar( '\n' );
    return BufferString( fms );
}


bool RockPhysics::Formula::usePar( const IOPar& iop )
{
    if ( iop.isEmpty() )
	return false;

    BufferString nm( iop.getKey(0) );
    if ( nm.isEmpty() )
	return false;

    setName( nm );
    BufferString desc;
    iop.get( sKey::Desc(), desc );
    desc = getStrFromFMS( desc );
    setDescription( desc.buf() );

    BufferString expr, unit;
    if ( iop.get(sKeyDef,expr) && !expr.isEmpty() )
	setText( expr.buf() );
    else
	return false;

    PtrMan<IOPar> subpar;
    int iinp = 0;
    for ( int idx=0; ; idx++ )
    {
	subpar = iop.subselect( IOPar::compKey(sKeyVar,idx) );
	if ( !subpar || subpar->isEmpty() )
	    break;

	nm = subpar->find( sKey::Name() );
	if ( !nm.isEmpty() )
	{
	    while ( isConst(iinp) && iinp < nrInputs() )
		iinp++;

	    setInputDef( iinp, nm );

	    desc.setEmpty();
	    unit.setEmpty();
	    BufferString mninpnm;
	    if ( subpar->get(sKey::Desc(),desc) )
	    {
		desc = getStrFromFMS( desc );
		setInputDescription( iinp, desc );
	    }
	    if ( subpar->get(sKey::Unit(),unit) )
	    {
		const UnitOfMeasure* uom = UoMR().get( unit );
		setInputFormUnit( iinp, uom );
	    }

	    subpar->get( sKey::Type(), mninpnm );
	    const Mnemonic* mn = mninpnm.startsWith( "Distance" )
					    ? &Mnemonic::distance()
					    : MNC().getByName( mninpnm, false );
	    if ( !mn )
		{ pErrMsg("Incorrect RockPhysics input in repository"); }
	    setInputMnemonic( iinp, mn );
	    iinp++;
	}
    }

    iinp = 0;
    for ( int idx=0; ; idx++ )
    {
	subpar = iop.subselect( IOPar::compKey(sKeyConst,idx) );
	if ( !subpar || subpar->isEmpty() )
	    break;

	nm = subpar->find( sKey::Name() );
	if ( !nm.isEmpty() )
	{
	    while ( !isConst(iinp) && iinp < nrInputs() )
		iinp++;

	    constantnms_.get( iinp ).set( nm );
	    desc.setEmpty();
	    BufferString defval;
	    subpar->get( sKey::Default(), defval );
	    setInputDef( iinp, defval.buf() );
	    if ( subpar->get(sKey::Desc(),desc) )
	    {
		desc = getStrFromFMS( desc );
		setInputDescription( iinp, desc );
	    }

	    Interval<float> typicalrg;
	    if ( subpar->get(sKeyRg,typicalrg) )
		delete typicalrgs_.replace( iinp,
					  new Interval<float>(typicalrg) );
	    iinp++;
	}
    }

    unit.setEmpty();
    iop.get( sKey::Unit(), unit );
    const BufferString mnemonicnm( iop.getValue(0) );
    const Mnemonic* mn = mnemonicnm.startsWith( "Distance" )
		       ? &Mnemonic::distance()
		       : MNC().getByName( mnemonicnm, false );
    if ( !mn )
	{ pErrMsg("Incorrect RockPhysics input in repository"); }

    setOutputMnemonic( mn );
    setOutputFormUnit( UoMR().get(unit) );

    return true;
}


static void setIOPWithNLs( IOPar& iop, const char* ky, const char* val )
{
    SeparString fms( val, '\n' );
    fms.setSepChar( '`' );
    iop.set( ky, fms );
}


void RockPhysics::Formula::fillPar( IOPar& iop ) const
{
    iop.setEmpty();
    const Mnemonic* mn = outputMnemonic();
    iop.set( name(), mn ? mn->name() : Mnemonic::undef().name() );
    setIOPWithNLs( iop, sKey::Desc(), description() );
    iop.set( sKeyDef, text() );
    const UnitOfMeasure* uom = outputFormUnit();
    iop.set( sKey::Unit(), uom ? uom->name() : BufferString::empty() );
    int icst = 0, ivar = 0;
    IOPar cstpar, varpar;
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
    {
	const BufferString desc( inputDescription(iinp) );
	const BufferString inpdef( inputDef(iinp) );
	if ( isConst(iinp) )
	{
	    const BufferString keybase( IOPar::compKey(sKeyConst,icst++) );
	    cstpar.set( IOPar::compKey(keybase,sKey::Name()),
			constantnms_.get(iinp).buf() );
	    if ( !desc.isEmpty() )
		setIOPWithNLs( cstpar, IOPar::compKey(keybase,sKey::Desc()),
			       desc.str() );
	    cstpar.set( IOPar::compKey(keybase,sKey::Default()), inpdef );
	    const Interval<float>* typrg = typicalrgs_.get( iinp );
	    if ( typrg )
		cstpar.set( IOPar::compKey(keybase,sKeyRg), *typrg );
	}
	else if ( !isSpec(iinp) )
	{
	    const BufferString keybase( IOPar::compKey(sKeyVar,ivar++) );
	    mn = inputMnemonic( iinp );
	    if ( !mn )
		mn = &Mnemonic::undef();
	    uom = inputFormUnit( iinp );
	    varpar.set( IOPar::compKey(keybase,sKey::Type()), mn->name() );
	    varpar.set( IOPar::compKey(keybase,sKey::Name()), inpdef );
	    if ( !desc.isEmpty() )
		setIOPWithNLs( varpar, IOPar::compKey(keybase,sKey::Desc()),
			       desc.str() );
	    varpar.set( IOPar::compKey(keybase,sKey::Unit()),
				    uom ? uom->name() : BufferString::empty() );
	}
    }

    iop.merge( varpar );
    iop.merge( cstpar );
}


bool RockPhysics::Formula::setDef( const char* str )
{
    PtrMan<MathProperty> mp = getProperty();
    if ( !mp )
	return false;

    Math::Formula::operator =( mp->getForm() );
    return true;
}


MathProperty* RockPhysics::Formula::getProperty( const PropertyRef* pr ) const
{
    if ( !pr )
    {
	const Mnemonic* mn = outputMnemonic();
	const PropertyRefSelection prs( mn ? *mn : Mnemonic::undef() );
	if ( prs.isEmpty() )
	    return nullptr;
	pr = prs.first();
    }

    auto* ret = new MathProperty( *pr );
    ret->getForm() = *this;

    return ret;
}


void RockPhysics::Formula::setText( const char* desc )
{
    constantnms_.setEmpty();
    deepErase( typicalrgs_ );
    Math::Formula::setText( desc );
    for ( int iinp=0; iinp<nrInputs(); iinp++ )
    {
	constantnms_.add( new BufferString );
	typicalrgs_.add( nullptr );
    }
}


void RockPhysics::Formula::setConstantName( int iinp, const char* nm )
{
    if ( constantnms_.validIdx(iinp) && isConst(iinp) )
	constantnms_.get( iinp ).set( nm );
}


void RockPhysics::Formula::setInputTypicalRange( int iinp,
						 const Interval<float>& rg )
{
    if ( typicalrgs_.validIdx(iinp) && isConst(iinp) )
	delete typicalrgs_.replace( iinp, new Interval<float>( rg ) );
}


const char* RockPhysics::Formula::inputConstantName( int iinp ) const
{
    return constantnms_.validIdx(iinp) && isConst(iinp)
	? constantnms_.get( iinp ).buf() : nullptr;
}


Interval<float> RockPhysics::Formula::inputTypicalRange( int iinp ) const
{
    Interval<float> ret = Interval<float>::udf();
    if ( typicalrgs_.validIdx(iinp) && isConst(iinp) && typicalrgs_.get(iinp) )
	ret = *typicalrgs_.get( iinp );
    return ret;
}


class RockPhysicsFormulaMgr : public CallBacker
{
public:

RockPhysicsFormulaMgr()
{
    mAttachCB( IOM().surveyToBeChanged, RockPhysicsFormulaMgr::doNull );
    mAttachCB( IOM().applicationClosing, RockPhysicsFormulaMgr::doNull );
}

~RockPhysicsFormulaMgr()
{
    detachAllNotifiers();
    delete fms_;
}

void doNull( CallBacker* )
{
    deleteAndZeroPtr( fms_ );
}

void createSet()
{
    Repos::FileProvider rfp( filenamebase, true );
    rfp.setSource( Repos::Source::ApplSetup );
    while ( rfp.next() )
    {
	const BufferString fnm( rfp.fileName() );
	SafeFileIO sfio( fnm );
	if ( !sfio.open(true) )
	    continue;

	ascistream astrm( sfio.istrm(), true );
	auto* tmp = new RockPhysics::FormulaSet;
	tmp->readFrom( astrm );
	if ( tmp->isEmpty() )
	    delete tmp;
	else
	{
	    delete fms_;
	    for ( const auto* fm : *tmp )
		const_cast<RockPhysics::Formula*>(fm)->setSource(rfp.source());
	    fms_ = tmp;
	    sfio.closeSuccess();
	    break;
	}

	sfio.closeSuccess();
    }

    if ( !fms_ )
	fms_ = new RockPhysics::FormulaSet;
}

    RockPhysics::FormulaSet*	fms_ = nullptr;

};

const RockPhysics::FormulaSet& ROCKPHYSFORMS()
{
    mDefineStaticLocalObject( RockPhysicsFormulaMgr, rfm, );
    if ( !rfm.fms_ )
	rfm.createSet();
    return *rfm.fms_;
}


RockPhysics::FormulaSet::~FormulaSet()
{
    ObjectSet<const RockPhysics::Formula>& fms = *this;
    deepErase( fms );
}


int RockPhysics::FormulaSet::getIndexOf( const char* nm ) const
{
    for ( int idx=0; idx<size(); idx++ )
    {
	const Formula& fm = *(*this)[idx];
	if ( fm.name() == nm )
	    return idx;
    }
    return -1;
}


const RockPhysics::Formula* RockPhysics::FormulaSet::getByName(
				const Mnemonic& mn, const char* nm ) const
{
    ObjectSet<const Math::Formula> rpforms;
    if ( !getRelevant(mn,rpforms) )
	return nullptr;

    for ( const auto* fm : rpforms )
    {
	if ( fm->name() == nm )
	    return sCast(const Formula*,fm);
    }

    return nullptr;
}


bool RockPhysics::FormulaSet::getRelevant( const Mnemonic& mn,
				   ObjectSet<const Math::Formula>& forms ) const
{
    ObjectSet<const Math::Formula> rpforms;
    for ( const auto* fm : *this )
	rpforms.add( fm );

    Math::getRelevant( mn, rpforms, forms );
    return !forms.isEmpty();
}


void RockPhysics::FormulaSet::getRelevant( const Mnemonic& mn,
					   BufferStringSet& nms ) const
{
    ObjectSet<const Math::Formula> forms;
    getRelevant( mn, forms );
    for ( const auto* fm : forms )
	nms.add( fm->name() );
}


bool RockPhysics::FormulaSet::hasType( PropType tp ) const
{
    for ( const auto* fm : *this )
	if ( fm->outputMnemonic() && fm->outputMnemonic()->stdType() == tp )
	    return true;

    return false;
}


void RockPhysics::FormulaSet::getRelevant( PropType tp,
					   MnemonicSelection& mnsel ) const
{
    for ( const auto* fm : *this )
    {
	const Mnemonic* mn = fm->outputMnemonic();
	if ( mn && mn->stdType() == tp )
	    mnsel.addIfNew( mn );
    }
}


bool RockPhysics::FormulaSet::save( Repos::Source src ) const
{
    Repos::FileProvider rfp( filenamebase );
    BufferString fnm = rfp.fileName( src );

    SafeFileIO sfio( fnm, true );
    if ( !sfio.open(false) )
    {
	BufferString msg( "Cannot write to " ); msg += fnm;
	ErrMsg( sfio.errMsg() );
	return false;
    }

    ascostream astrm( sfio.ostrm() );
    if ( !writeTo(astrm) )
	{ sfio.closeFail(); return false; }

    sfio.closeSuccess();
    return true;
}


void RockPhysics::FormulaSet::readFrom( ascistream& astrm )
{
    deepErase( *this );

    while ( !atEndOfSection(astrm.next()) )
    {
	IOPar iop; iop.getFrom(astrm);

	RockPhysics::Formula* fm = RockPhysics::Formula::get( iop );
	if ( fm )
	    *this += fm;
    }
}


bool RockPhysics::FormulaSet::writeTo( ascostream& astrm ) const
{
    astrm.putHeader( "Rock Physics" );
    for ( int idx=0; idx<size(); idx++ )
    {
	const RockPhysics::Formula& fm = *(*this)[idx];
	IOPar iop; fm.fillPar( iop );
	iop.putTo( astrm );
    }
    return astrm.isOK();
}
