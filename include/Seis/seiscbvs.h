#ifndef seiscbvs_h
#define seiscbvs_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		April 2001
 RCS:		$Id: seiscbvs.h,v 1.6 2001-07-21 16:35:47 bert Exp $
________________________________________________________________________

CBVS-based seimic translator.

-*/

#include <seistrctr.h>
#include <tracedata.h>
#include <cbvsinfo.h>
class CBVSReadMgr;
class CBVSWriteMgr;


class CBVSSeisTrcTranslator : public SeisTrcTranslator
{			isTranslator(CBVS,SeisTrc)
public:

			CBVSSeisTrcTranslator(const char* nm=0);
			~CBVSSeisTrcTranslator();

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int nrtrcs=1);
    bool		write(const SeisTrc&);
    void		close();

    bool		supportsGoTo() const		{ return true; }
    bool		goTo(const BinID&);
    void		forceWriteIntegrity( bool yn )	{ wrintegr = yn; }

    const UserIDSet*	parSpec(Conn::State) const
			{ return &datatypeparspec; }
    virtual void	usePar(const IOPar*);

    const CBVSReadMgr*	readMgr() const		{ return rdmgr; }

protected:

    bool		forread;
    bool		headerdone;
    bool		donext;
    int			nrdone;
    bool		wrintegr;

    // Following variables are inited by commitSelections_
    TraceDataInterpreter** storinterps;
    StepInterval<int>*	samps;
    Interval<int>*	cbvssamps;
    bool*		comps;
    bool*		userawdata;
    bool*		samedatachar;
    int*		actualsz;
    unsigned char**	blockbufs;
    unsigned char**	stptrs;
    unsigned char**	tdptrs;
    int			preseldatatype;

    CBVSReadMgr*	rdmgr;
    CBVSWriteMgr*	wrmgr;
    CBVSInfo::ExplicitData expldat;

    virtual void	cleanUp();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	commitSelections_();
    bool		startWrite();
    bool		toNext();
    bool		getFileName(BufferString&);

private:

    static UserIDSet	datatypeparspec;

    void		calcSamps();
    void		destroyVars();

};


#endif
