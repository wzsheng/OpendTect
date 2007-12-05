#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.25 2007-12-05 11:55:49 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "seisioobjinfo.h"
#include "seisselection.h"
class IOObj;
class Executor;
class uiGenInput;
class uiSeisSubSel;
class SeisResampler;
class uiSeis2DSubSel;
class uiSeis3DSubSel;
class uiSeisFmtScale;
namespace Seis { class SelData; }


class uiSeisTransfer : public uiGroup
{
public:

    class Setup : public Seis::SelSetup
    {
    public:
			Setup( Seis::GeomType gt )
			    : Seis::SelSetup(gt)
			    , withnullfill_(false)	{}
				    //!< 'add null traces' (3D)
			Setup( const Seis::SelSetup& sss )
			    : Seis::SelSetup(sss)
			    , withnullfill_(false)	{}
			Setup( bool is2d, bool isps )
			    : Seis::SelSetup(is2d,isps)
			    , withnullfill_(false)	{}

	mDefSetupMemb(bool,withnullfill)
    };

			uiSeisTransfer(uiParent*,const Setup&);
    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
	    			   const char* executor_txt,
				   const char* work_txt,
				   const char* attr2dnm,
				   const char* linenm2d_overrule=0) const;

    uiSeisSubSel*	selfld;
    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    uiSeis2DSubSel*	selFld2D(); //!< null when not 2D
    uiSeis3DSubSel*	selFld3D(); //!< null when not 3D

    void		setSteering(bool);
    void		setInput(const IOObj&);
    Seis::SelData*	getSelData() const;
    SeisResampler*	getResampler() const; //!< may return null

    int			maxBytesPerSample() const;
    SeisIOObjInfo::SpaceInfo spaceInfo() const;

    bool		removeNull() const;
    bool		fillNull() const;

protected:

    Setup		setup_;

};


#endif
