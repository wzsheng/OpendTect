#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        H. Huck
 Date:          July 2006
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiattrdesced.h"

#include "attribdescid.h"
#include "position.h"
#include "ranges.h"

class uiAttrSel;
class uiGenInput;
class uiLabeledSpinBox;
class uiPushButton;
class uiGDPositionDlg;
class GapDeconACorrView;


/*! \brief GapDecon Attribute description editor */

mClass(uiAttributes) uiGapDeconAttrib : public uiAttrDescEd
{ mODTextTranslationClass(uiGapDeconAttrib);
public:

			uiGapDeconAttrib(uiParent*,bool);
			~uiGapDeconAttrib();

    void		getEvalParams(TypeSet<EvalParam>&) const;
    static const char*	sKeyOnInlineYN();
    static const char*	sKeyLineName();

protected:

    uiAttrSel*		inpfld_;
    uiGenInput*		gatefld_;
    uiGenInput*		lagfld_;
    uiGenInput*		gapfld_;
    uiGenInput*		noiselvlfld_;
    uiGenInput*		isinpzerophasefld_;
    uiGenInput*		isoutzerophasefld_;
    uiGenInput*		wantmixfld_;
    uiLabeledSpinBox*	stepoutfld_;
    uiPushButton*	exambut_;
    uiPushButton*	qcbut_;

    IOPar		par_;
    uiGDPositionDlg*	positiondlg_;
    GapDeconACorrView*	acorrview_;
    bool		setParameters(const Attrib::Desc&);
    bool		setInput(const Attrib::Desc&);

    bool		getParameters(Attrib::Desc&);
    bool		getInput(Attrib::Desc&);

    void		finalizeCB(CallBacker*);
    void                examPush(CallBacker*);
    void                qCPush(CallBacker*);
    void                mixSel(CallBacker*);
    bool		passStdCheck(const Attrib::Desc*,const char*,int,int,
				     Attrib::DescID);
    bool		passVolStatsCheck(const Attrib::Desc*,BinID,
					  Interval<float>);
    Attrib::Desc*	createNewDesc(Attrib::DescSet*,Attrib::DescID,
				      const char*,int,int,BufferString);
    Attrib::DescID	createVolStatsDesc(Attrib::Desc&,int);
    void		createHilbertDesc(Attrib::Desc&,Attrib::DescID&);
    Attrib::DescID	createGapDeconDesc(Attrib::DescID&,Attrib::DescID,
					   DescSet*,bool);
    void		prepareInputDescs(Attrib::DescID&,Attrib::DescID&,
					  Attrib::DescSet*);
    void		fillInGDDescParams(Attrib::Desc*);
    void		getInputMID(MultiID&) const;

			mDeclReqAttribUIFns
};


