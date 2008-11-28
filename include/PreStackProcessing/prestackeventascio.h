#ifndef prestackeventascio_h
#define prestackeventascio_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	K. Tingdahl
 Date:		November 2008
 RCS:		$Id: prestackeventascio.h,v 1.2 2008-11-28 19:56:38 cvskris Exp $
________________________________________________________________________

-*/

#include "binidvalset.h"
#include "task.h"
#include "horsampling.h"
#include "prestackevents.h"

namespace PreStack
{
class EventManager;

/*!Outputs an ascii string with all prestack event, each pick on one row. The
columns are as follows:

     Inline
     Crossline
     Event index (0-N).
     Dip, going to increasing inlines, If not dip is available, 0 is written
     Dip, going to increasing crosslines, If not dip is available, 0 is written
     Event quality, 0-255
     Azimuth (0-2PI)
     Offset
     Depth
     Pick quality, 0-255
*/


class EventExporter : public SequentialTask
{
public:
    			EventExporter(std::ostream& strm,EventManager&);
    			~EventExporter();
    void		setHRange(const HorSampling& hrg);

    od_int64		nrDone() const { return nrdone_; }
    od_int64		totalNr() const { return locations_.totalSize(); }
    int			nextStep();
    const char*		nrDoneText() const;

protected:

    std::ostream&		strm_;
    EventManager&		events_;
    HorSampling			hrg_;

    BinIDValueSet		locations_;
    BinIDValueSet::Pos		pos_;

    int				nrdone_;
    int				fileidx_;
};


} // namespace PreStack

#endif
