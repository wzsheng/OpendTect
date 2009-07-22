#ifndef uimadiosel_h
#define uimadiosel_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: uimadiosel.h,v 1.9 2009-07-22 16:01:28 cvsbert Exp $
-*/

#include "uicompoundparsel.h"
#include "uidialog.h"
#include "madprocflow.h"
#include "seistype.h"
#include "iopar.h"

class uiCheckBox;
class uiGenInput;
class CtxtIOObj;
class uiSeisSel;
class uiIOObjSel;
class uiFileInput;
class uiSeisSubSel;
class uiSeis2DSubSel;
class uiSeis3DSubSel;


class uiMadIOSel : public uiCompoundParSel
{
public:
			uiMadIOSel(uiParent*,bool isinp);

    void		usePar(const IOPar&);
    void		useParIfNeeded(const IOPar&);
    bool		fillPar( IOPar& iop ) const
    			{ iop.merge(iop_); return iop_.size(); }

    Notifier<uiMadIOSel>	selectionMade;
protected:

    bool		isinp_;
    IOPar		iop_;

    virtual BufferString getSummary() const;
    void		doDlg(CallBacker*);

};


class uiMadIOSelDlg : public uiDialog
{
public:

			uiMadIOSelDlg(uiParent*,IOPar&,bool isinp);
			~uiMadIOSelDlg();

    inline bool		isInp() const
    			{ return isinp_; }
    bool		isNone() const;
    bool		isMad() const;
    bool		isSU() const;
    inline bool		isOD() const	{ return !isMad() && !isNone(); }
    ODMad::ProcFlow::IOType ioType() const
			{
			    return isNone() ? ODMad::ProcFlow::None
				 : (isMad() ? ODMad::ProcFlow::Madagascar
				 : (ODMad::ProcFlow::IOType)geomType());
			}

    			// Functions only valid if isOD()
    Seis::GeomType	geomType() const;
    CtxtIOObj&		ctxtIOObj(Seis::GeomType);
    uiSeisSel*		seisSel(Seis::GeomType);
    uiSeisSubSel*	seisSubSel(Seis::GeomType);

    void		usePar(const IOPar&);
    bool		fillPar(IOPar&);

protected:

    CtxtIOObj&		ctio3d_;
    CtxtIOObj&		ctio2d_;
    CtxtIOObj&		ctiops3d_;
    CtxtIOObj&		ctiops2d_;
    int			idx3d_, idx2d_, idxps3d_, idxps2d_, idxmad_,
			idxsu_, idxnone_;
    IOPar&		iop_;
    bool		isinp_;

    uiGenInput*		typfld_;
    uiSeisSel*		seis3dfld_;
    uiSeisSel*		seis2dfld_;
    uiSeisSel*		seisps3dfld_;
    uiSeisSel*		seisps2dfld_;
    uiFileInput*	madfld_;
    uiCheckBox*		sconsfld_;
    // Inp only:
    uiSeis3DSubSel*	subsel3dfld_;
    uiSeis2DSubSel*	subsel2dfld_;
    uiSeis2DSubSel*	subsel2dpsfld_;

    void		initWin(CallBacker*);
    void		typSel(CallBacker*);
    void		selChg(CallBacker*);
    void		sconsCB(CallBacker*);

    bool		getInp();
    bool		acceptOK(CallBacker*);

};


#endif
