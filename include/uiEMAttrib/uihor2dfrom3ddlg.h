#pragma once
/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Satyaki Maitra
Date:          July 2008
________________________________________________________________________

-*/

#include "uiemattribmod.h"
#include "uidialog.h"
#include "emposid.h"

namespace EM { class Horizon2D; }
class uiSeis2DMultiLineSel;
class uiHorizonParSel;
class uiGenInput;
class uiCheckBox;

mExpClass(uiEMAttrib) uiHor2DFrom3DDlg : public uiDialog
{ mODTextTranslationClass(uiHor2DFrom3DDlg);
public:
    				uiHor2DFrom3DDlg(uiParent*);

    bool			doDisplay() const;
    const TypeSet<EM::ObjectID>& getEMObjIDs() const	  { return emobjids_; }

protected:
    uiSeis2DMultiLineSel*	linesetinpsel_;
    uiHorizonParSel*		hor3dselfld_;
    uiGenInput*			hor2dnmfld_;
    uiCheckBox*			displayfld_;
    TypeSet<EM::ObjectID>	emobjids_;

    bool			checkOutNames(BufferStringSet&) const;
    bool			checkFlds();
    bool			acceptOK( CallBacker* );

};

