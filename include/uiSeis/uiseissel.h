#ifndef uiseissel_h
#define uiseissel_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          July 2001
 RCS:           $Id: uiseissel.h,v 1.46 2010-08-11 14:50:45 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiioobjsel.h"
#include "uicompoundparsel.h"
#include "seistype.h"

class uiSeisIOObjInfo;
class uiListBox;
class uiCheckBox;

mClass uiSeisSel : public uiIOObjSel
{
public:

    struct Setup : public uiIOObjSel::Setup
    {
			Setup( Seis::GeomType gt )
			    : geom_(gt)
			    , selattr_(gt==Seis::Line)
			    , allowsetdefault_(true)
			    , steerpol_(-1)
			    , enabotherdomain_(false)	{}
			Setup( bool is2d, bool isps )
			    : geom_(Seis::geomTypeOf(is2d,isps))
			    , selattr_(is2d && !isps)
			    , allowsetdefault_(true)
			    , steerpol_(-1)
			    , enabotherdomain_(false)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(bool,selattr)		//!< 2D: can user select attrib?
	mDefSetupMemb(bool,allowsetdefault)	//!< Fill with def cube/line?
	mDefSetupMemb(bool,enabotherdomain)	//!< write only: T vs Depth
	mDefSetupMemb(int,steerpol)		//!< -1=no, 0=maybe, 1=only
	mDefSetupMemb(BufferString,defaultkey)
    };

			uiSeisSel(uiParent*,CtxtIOObj&,const Setup&);
			uiSeisSel(uiParent*,const IOObjContext&,const Setup&);
			~uiSeisSel();

    virtual bool	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    inline Seis::GeomType geomType() const { return seissetup_.geom_; }
    inline bool		is2D() const	{ return Seis::is2D(seissetup_.geom_); }
    inline bool		isPS() const	{ return Seis::isPS(seissetup_.geom_); }

    void		setAttrNm(const char*);
    const char*		attrNm() const	{ return attrnm_.buf(); }
    virtual void	processInput();
    virtual bool	existingTyped() const;
    virtual void	updateInput();

    static CtxtIOObj*	mkCtxtIOObj(Seis::GeomType,bool forread);
    				//!< returns new default CtxtIOObj
    static IOObjContext	ioContext(Seis::GeomType,bool forread);
    static void		fillContext(Seis::GeomType,bool forread,IOObjContext&);

protected:

    Setup		seissetup_;
    BufferString	attrnm_;
    mutable BufferString curusrnm_;
    IOPar		dlgiopar_;
    uiCheckBox*		othdombox_;

    Setup		mkSetup(const Setup&,bool);
    virtual void	newSelection(uiIOObjRetDlg*);
    virtual void	commitSucceeded();
    virtual IOObj*	createEntry(const char*);
    virtual const char*	userNameFromKey(const char*) const;
    virtual uiIOObjRetDlg* mkDlg();
    void		mkOthDomBox();
};


mClass uiSeisSelDlg : public uiIOObjSelDlg
{
public:

			uiSeisSelDlg(uiParent*,const CtxtIOObj&,
				     const uiSeisSel::Setup&);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    static const char*	standardTranslSel(Seis::GeomType,bool);

protected:

    uiListBox*		attrlistfld_;
    uiGenInput*		attrfld_;
    int			steerpol_;
    BufferString	notalloweddatatype_;	// 2D only

    void		entrySel(CallBacker*);
    void 		attrNmSel(CallBacker*);
    const char*		getDataType();
};


#endif
