#ifndef maddefs_h
#define maddefs_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: madio.h,v 1.2 2007-11-13 16:21:27 cvsbert Exp $
-*/

#include "bufstring.h"
#include "strmdata.h"
class IOPar;


namespace ODMad
{

extern const char* sKeyMadagascar;

class FileSpec
{
public:

    			FileSpec(bool forread);
    void		set(const char* fnm,const char* datapath=0);
				//!< If not absolute: uses datapath
				//!<	then, if datapath==null -> $DATAPATH

    const char*		fileName() const	{ return fname_; }
    const char*		dataPath()		{ return datapath_; }

    StreamData		open() const;		//!< if !usable() -> errMsg()
    const char*		errMsg() const		{ return errmsg_; }

    static const char*	defDataPath();
    static const char*	sKeyDataPath;

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);	//!< sets errMsg() if failed

protected:

    bool		forread_;
    BufferString	fname_;
    BufferString	datapath_;
    mutable BufferString errmsg_;

};

} // namespace

#endif
