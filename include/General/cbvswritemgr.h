#ifndef cbvswritemgr_h
#define cbvswritemgr_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format writer
 RCS:		$Id: cbvswritemgr.h,v 1.9 2002-11-27 17:05:23 bert Exp $
________________________________________________________________________

-*/

#include <cbvsio.h>
#include <cbvsinfo.h>
#include <iosfwd>

class CBVSWriter;

/*!\brief Vertical bricking specification */

struct VBrickSpec
{
		VBrickSpec()		{ setStd(false); }

    void	setStd(bool yn_bricking=false);

    int		nrsamplesperslab;	//!< -1 means no bricking
    int		maxnrslabs;
};


/*!\brief Writer for CBVS file packs */

class CBVSWriteMgr : public CBVSIOMgr
{
public:

			CBVSWriteMgr(const char* basefname,const CBVSInfo&,
					const PosAuxInfo* =0,
					VBrickSpec* =0);
			//!< See CBVSWriter for parameters 2 and 3
			~CBVSWriteMgr();

    unsigned long	bytesPerFile() const;
			//!< After this amount of bytes, a new file will
			//!< be created for the next inline.
			//!< The default is 1.8 GB, 0 = unlimited
    			//!< Only active without vertical bricking
    void		setBytesPerFile(unsigned long);
    			//!< Only works without vertical bricking

    bool		put(void**);
			//!< See CBVSWriter::put, only now succeeds or fails
    void		close();
			//!< See CBVSWriter::close

    int			nrComponents() const;
    const BinID&	binID() const;

protected:

    ObjectSet<CBVSWriter> writers_;
    TypeSet<int>	endsamps_;
    CBVSInfo		info_;

    const char*		errMsg_() const;

    ostream*		mkStrm();
    void		cleanup();

};


#endif
