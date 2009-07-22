#ifndef uistatsdisplaywin_h
#define uistatsdisplaywin_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uistatsdisplaywin.h,v 1.7 2009-07-22 16:01:23 cvsbert Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "uistatsdisplay.h"
class BufferStringSet;
class uiComboBox;
class uiStatsDisplay;
namespace Stats { template <class T> class RunCalc; }

/*!\brief Stats display main window. See uistatsdisplay.h for details. */

mClass uiStatsDisplayWin : public uiMainWin
{
public:
    				uiStatsDisplayWin(uiParent*,
					const uiStatsDisplay::Setup&,int nr=1,
					bool ismodal=true);
    
    uiStatsDisplay*		statsDisplay(int nr=0)	{ return disps_[nr]; }
    void                        setData(const Stats::RunCalc<float>&,int nr=0);
    void			addDataNames(const BufferStringSet&);
    void			setDataName(const char*,int nr=0);
    void			setMarkValue(float,bool forx,int nr=0);
    void			showStat(int);

protected:

    ObjectSet<uiStatsDisplay>	disps_;
    uiComboBox*			statnmcb_;

    void			dataChanged(CallBacker*);
};


#endif
