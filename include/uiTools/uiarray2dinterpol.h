#ifndef uiarray2dinterpol_h
#define uiarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          April 2009
 RCS:           $Id: uiarray2dinterpol.h,v 1.6 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"
#include "factory.h"

class Array2DInterpol;
class InverseDistanceArray2DInterpol;
class uiGenInput;
class uiArray2DInterpol;


mClass uiArray2DInterpolSel : public uiDlgGroup
{
public:
    mDefineFactory1ParamInClass(uiArray2DInterpol,uiParent*,factory);
    				uiArray2DInterpolSel(uiParent*,bool filltype,
				bool holesz, bool withclassification, 
				const Array2DInterpol* oldvals);

    bool			acceptOK();
    Array2DInterpol*		getResult();
    				//!<\note Becomes caller's
				
    void			setDistanceUnit(const char*);
    				//!<A unitstring in [] that tells what the
				//!<unit is for going from one cell to another
    
    const char*			helpID() const;

protected:
					~uiArray2DInterpolSel();
    uiParent*				getTopObject();
    void				selChangeCB(CallBacker*);

    Array2DInterpol*			result_;

    uiGenInput*				filltypefld_;
    uiGenInput*				maxholeszfld_;
    uiGenInput*				methodsel_;
    uiGenInput*				isclassificationfld_;

    ObjectSet<uiArray2DInterpol>	params_;
};


mClass uiArray2DInterpol : public uiDlgGroup
{
public:
    virtual void	setValuesFrom(const Array2DInterpol&)		{}
    			//*!Dose only work if provided object is of 'your' type.

    bool		acceptOK()					= 0;
    virtual void	setDistanceUnit(const char*)			{}

    Array2DInterpol*	getResult();
			//!<Becomes caller's


protected:
    			uiArray2DInterpol(uiParent*,const char* nm);
    			~uiArray2DInterpol();
    Array2DInterpol*	result_;
};


mClass uiInverseDistanceArray2DInterpol : public uiArray2DInterpol
{
public:
    static void			initClass();
    static uiArray2DInterpol*	create(uiParent*);

    				uiInverseDistanceArray2DInterpol(uiParent*);

    void			setValuesFrom(const Array2DInterpol&);
    bool			acceptOK();
    void			setDistanceUnit(const char*);

    const char*			helpID() const { return "od: 104.0.13"; }

protected:

    void			useRadiusCB(CallBacker*);

    uiGenInput*			searchradiusfld_;
    uiGenInput*			cornersfirstfld_;
    uiGenInput*			stepsizefld_;
    uiGenInput*			nrstepsfld_;
};

#endif
