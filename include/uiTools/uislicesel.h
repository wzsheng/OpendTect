#ifndef uislicesel_h
#define uislicesel_h
/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uislicesel.h,v 1.3 2002-08-19 07:30:00 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class uiLabeledSpinBox;
class uiGenInput;
class CubeSampling;
class uiPushButton;
class uiCheckBox;


class uiSliceSel : public uiDialog
{
public:
				uiSliceSel(uiParent*,const CubeSampling&,
					   const CallBack&,bool isvol=false);
				~uiSliceSel();
    const CubeSampling&		getCubeSampling()	{ return cs; }

protected:

    void			updateSel(CallBacker*);
    void			csChanged(CallBacker*);
    void			stepSel(CallBacker*);
    void			readInput();
    bool			acceptOK(CallBacker*);

    uiLabeledSpinBox*           inlfld;
    uiLabeledSpinBox*           crlfld;
    uiLabeledSpinBox*           zfld;
    uiGenInput*                 inlrgfld;
    uiGenInput*                 crlrgfld;
    uiGenInput*                 zrgfld;
    uiCheckBox*			doupdfld;
    uiLabeledSpinBox*		stepfld;

    CubeSampling&		cs;
    Notifier<uiSliceSel>	cschanged;
};

#endif
