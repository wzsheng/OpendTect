#ifndef wellmarker_h
#define wellmarker_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellmarker.h,v 1.9 2009-06-22 11:49:52 cvsbert Exp $
________________________________________________________________________


-*/

#include "namedobj.h"
#include "color.h"
#include "tableascio.h"
namespace Strat { class Level; }

namespace Well
{

class Track;

/*!\brief Marker, should be attached to Strat level

  Can be unattached, then uses the fallback name and color (and istop_). 

*/

mClass Marker : public ::NamedObject
{
public:

			Marker( const char* nm=0, float dh=0 )
			: ::NamedObject(nm)
			, dah_(dh)
			, levelid_(-1)		{}
			Marker(int lvlid,float dh);

    float		dah() const		{ return dah_; }
    void		setDah( float v )	{ dah_ = v; }
    int			levelID() const		{ return levelid_; }
    void		setLevelID( int v )	{ levelid_ = v; }
    const Strat::Level*	level() const;

    const BufferString&	name() const;
    Color		color() const;

    static const char*	sKeyDah();

    // setName() and setColor() only used as fallback, if not attached to level
    void		setColor( Color col )	{ color_ = col; }

protected:

    float		dah_;
    int			levelid_;
    Color		color_;
};


mClass MarkerSet : public ObjectSet<Marker>
{
public:
    			MarkerSet()		{}
};


mClass MarkerSetAscIO : public Table::AscIO
{
public:
    			MarkerSetAscIO( const Table::FormatDesc& fd )
			    : Table::AscIO(fd)		{}

    static Table::FormatDesc*	getDesc();

    bool		get(std::istream&,MarkerSet&,const Well::Track&) const;

};


} // namespace Well

#endif
