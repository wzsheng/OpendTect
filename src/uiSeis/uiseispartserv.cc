/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseispartserv.cc,v 1.111 2009-07-22 16:01:42 cvsbert Exp $";

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioobj.h"
#include "ioman.h"
#include "keystrs.h"
#include "ptrman.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2dline.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "posinfo.h"
#include "survinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisioobjinfo.h"

#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uiseiscbvsimp.h"
#include "uiseisiosimple.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseisrandto2dline.h"
#include "uisegyread.h"
#include "uisegyexp.h"
#include "uisegysip.h"
#include "uisurvinfoed.h"
#include "uiseissel.h"
#include "uiseiswvltimp.h"
#include "uiseiswvltman.h"
#include "uiseispreloadmgr.h"
#include "uiselsimple.h"
#include "uisurvey.h"
#include "uitaskrunner.h"
#include "uibatchtime2depthsetup.h"


uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    uiSEGYSurvInfoProvider* sip = new uiSEGYSurvInfoProvider();
    uiSurveyInfoEditor::addInfoProvider( sip );
    SeisIOObjInfo::initDefault( sKey::Steering );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    PtrMan<uiDialog> dlg = 0;
    if ( opt == 6 )
	dlg = new uiSeisImpCBVS( appserv().parent() );
    else if ( opt < 2 || opt == 7 )
    {
	if ( !forread )
	{
	    const Seis::GeomType gt = opt == 0 ? Seis::Vol : Seis::Line;
	    dlg = new uiSEGYExp( appserv().parent(), gt );
	}
	else
	{
	    uiSEGYRead::Setup su( opt == 7 ? uiSEGYRead::DirectDef
		    			   : uiSEGYRead::Import );
	    if ( opt == 7 )
		{ su.geoms_ -= Seis::Line; su.geoms_ -= Seis::Vol; }
	    new uiSEGYRead( appserv().parent(), su );
	}
    }
    else
    {
	const Seis::GeomType gt =  opt == 2 ? Seis::Vol
				: (opt == 3 ? Seis::Line
				: (opt == 4 ? Seis::VolPS : Seis::LinePS));
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true;
	dlg = new uiSeisIOSimple( appserv().parent(), gt, forread );
    }

    return dlg ? dlg->go() : true;
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


void uiSeisPartServer::manageSeismics()
{
    uiSeisFileMan dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::managePreLoad()
{
    uiSeisPreLoadMgr dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    uiSeisWvltImp dlg( appserv().parent() );
    dlg.go();
}


void uiSeisPartServer::manageWavelets()
{
    uiSeisWvltMan dlg( appserv().parent() );
    dlg.go();
}


bool uiSeisPartServer::select2DSeis( MultiID& mid, bool with_attr )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    uiSeisSel::Setup setup(Seis::Line); setup.selattr( with_attr );
    uiSeisSelDlg dlg( appserv().parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DLineSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    BufferString fnm = ioobj->fullUserExpr(true); \
    Seis2DLineSet lineset( fnm );


void uiSeisPartServer::get2DLineSetName( const MultiID& mid, 
					 BufferString& setname )
{
    mGet2DLineSet()
    setname = lineset.name();
}


bool uiSeisPartServer::select2DLines( const MultiID& mid, BufferStringSet& res )
{
    BufferStringSet linenames;
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getLineNames( linenames );

    uiSelectFromList::Setup setup( "Select 2D Lines", linenames );
    uiSelectFromList dlg( appserv().parent(), setup );
    if ( dlg.selFld() )
    {
	dlg.selFld()->setMultiSelect();
	dlg.selFld()->selectAll( true );
    }
    if ( !dlg.go() )
	return false;

    if ( dlg.selFld() )
	dlg.selFld()->getSelectedItems( res );
    return res.size();
}


void uiSeisPartServer::get2DLineInfo( BufferStringSet& linesets,
				      TypeSet<MultiID>& setids,
				      TypeSet<BufferStringSet>& linenames )
{
    SeisIOObjInfo::get2DLineInfo( linesets, &setids, &linenames );
}


bool uiSeisPartServer::get2DLineGeometry( const MultiID& mid,
					  const char* linenm,
					  PosInfo::Line2DData& geom )
{
    mGet2DLineSet(false)
    int lineidx = lineset.indexOf( linenm );
    if ( lineidx < 0 )
    {
	BufferStringSet attribs;
	get2DStoredAttribs( mid, linenm, attribs );
	if ( attribs.isEmpty() ) return false;

	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	int maxnrtrcs = 0;
	for ( int idx=0; idx<attribs.size(); idx++ )
	{
	    int indx = lineset.indexOf( LineKey(linenm,attribs.get(idx)) );
	    if ( indx<0 || !lineset.getRanges(indx,trcrg,zrg) || !trcrg.step )
		continue;

	    const int nrtrcs = trcrg.nrSteps() + 1;
	    if ( nrtrcs > maxnrtrcs )
	    {
		maxnrtrcs = nrtrcs;
		lineidx = indx;
	    }
	}

	if ( lineidx < 0 ) return false;
    }

    return lineset.getGeometry( lineidx, geom );
}


void uiSeisPartServer::get2DStoredAttribs( const MultiID& mid, 
					   const char* linenm,
					   BufferStringSet& attribs )
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs );
}


void uiSeisPartServer::get2DStoredAttribsPartingDataType( const MultiID& mid,
						          const char* linenm,
					             BufferStringSet& attribs,
						     const char* datatypenm,
       						     bool isincl )
{
    uiSeisIOObjInfo objinfo( mid );
    objinfo.getAttribNamesForLine( linenm, attribs, true, datatypenm,
	   			   !isincl, isincl );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid, const char* linekey,
				       CubeSampling& cs, SeisTrcBuf& buf )
{
    mGet2DLineSet(false)

    const int lidx = lineset.indexOf( linekey );
    if ( lidx < 0 ) return false;

    lineset.getCubeSampling( cs, lidx );
    PtrMan<Executor> exec = lineset.lineFetcher( lidx, buf );
    uiTaskRunner dlg( appserv().parent() );
    return dlg.execute( *exec );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id) );
    const ObjectSet<IOObj>& ioobjs = IOM().dirPtr()->getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj)
	  && SeisTrcTranslator::is2D(ioobj) != for3d )
	    nms.add( (const char*)ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( appserv().parent(), rln );
    dlg.go();
}


void uiSeisPartServer::create2DGridFromRln(
				const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DGridDlg dlg( appserv().parent(), rln );
    dlg.go();
}


void uiSeisPartServer::processTime2Depth() const
{
    uiBatchTime2DepthSetup dlg( appserv().parent() );
    dlg.go();
}
