/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : July 2005 / Mar 2008
-*/


#include "posinfo.h"
#include "survinfo.h"
#include "od_iostream.h"
#include "math2.h"


int PosInfo::LineData::size() const
{
    int res = 0;
    for ( int idx=0; idx<segments_.size(); idx++ )
	res += segments_[idx].nrSteps() + 1;
    return res;
}


int PosInfo::LineData::nearestSegment( double x ) const
{
    if ( segments_.size() < 1 )
	return -1;

    int ret = 0; float mindist = mUdf(float);
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	const PosInfo::LineData::Segment& seg = segments_[iseg];

	const bool isrev = seg.step < 0;
	const float hstep = (float)seg.step * 0.5f;
	float dist;
	if ( (isrev && x > seg.start+hstep) || (!isrev && x < seg.start-hstep) )
	    dist = (float)( x - seg.start );
	else if ( (isrev && x<seg.stop-hstep) || (!isrev && x>seg.stop+hstep))
	    dist = (float)( x - seg.stop );
	else
	    { ret = iseg; break; }

	if ( dist < 0 ) dist = -dist;
	if ( dist < mindist )
	    { ret = iseg; mindist = dist; }
    }

    return ret;
}


int PosInfo::LineData::segmentOf( int nr ) const
{
    for ( int iseg=0; iseg<segments_.size(); iseg++ )
    {
	if ( segments_[iseg].includes(nr,false) )
	{
	    if ( segments_[iseg].step < 2 )
		return iseg;

	    const bool inbetween = (nr-segments_[iseg].start)
				   % segments_[iseg].step;
	    return inbetween ? -1 : iseg;
	}
    }

    return -1;
}


Interval<int> PosInfo::LineData::range() const
{
    if ( segments_.isEmpty() ) return Interval<int>( mUdf(int), mUdf(int) );

    Interval<int> ret( segments_[0].start, segments_[0].start );
    for ( int idx=0; idx<segments_.size(); idx++ )
    {
	const Segment& seg = segments_[idx];
	if ( seg.start < ret.start ) ret.start = seg.start;
	if ( seg.stop < ret.start ) ret.start = seg.stop;
	if ( seg.start > ret.stop ) ret.stop = seg.start;
	if ( seg.stop > ret.stop ) ret.stop = seg.stop;
    }

    return ret;
}


void PosInfo::LineData::merge( const PosInfo::LineData& ld1, bool inc )
{
    if ( segments_.isEmpty() )
    {
	if ( inc )
	    segments_ = ld1.segments_;
	return;
    }
    if ( ld1.segments_.isEmpty() )
    {
	if ( !inc )
	    segments_.erase();
	return;
    }

    const PosInfo::LineData ld2( *this );
    segments_.erase();

    Interval<int> rg( ld1.range() ); rg.include( ld2.range() );
    const int defstep = ld1.segments_.isEmpty() ? ld2.segments_[0].step
						: ld1.segments_[0].step;
    if ( rg.start == rg.stop )
    {
	segments_ += Segment( rg.start, rg.start, defstep );
	return;
    }
    else if ( ld1.segments_.size() == 1 && ld2.segments_.size() == 1 )
    {
	// Very common, can be done real fast
	if ( inc )
	    segments_ += Segment( rg.start, rg.stop, defstep );
	else
	{
	    Segment seg( ld1.segments_[0] );
	    const Segment& ld2seg = ld2.segments_[0];
	    if ( ld2seg.start > seg.start ) seg.start = ld2seg.start;
	    if ( ld2seg.stop < seg.stop ) seg.stop = ld2seg.stop;
	    if ( seg.stop >= seg.start )
		segments_ += seg;
	}
	return;
    }

    // slow but straightforward
    Segment curseg( mUdf(int), 0, mUdf(int) );
    for ( int nr=rg.start; nr<=rg.stop; nr++ )
    {
	const bool in1 = ld1.segmentOf(nr) >= 0;
	bool use = true;
	if ( (!in1 && !inc) || (in1 && inc ) )
	    use = inc;
	else
	    use = ld2.segmentOf(nr) >= 0;

	if ( use )
	{
	    if ( mIsUdf(curseg.start) )
		curseg.start = curseg.stop = nr;
	    else
	    {
		int curstep = nr - curseg.stop;
		if ( mIsUdf(curseg.step) )
		{
		    curseg.step = curstep;
		    curseg.stop = nr;
		}
		else if ( curstep == curseg.step )
		    curseg.stop = nr;
		else
		{
		    segments_ += curseg;
		    curseg.start = curseg.stop = nr;
		    curseg.step = mUdf(int);
		}
	    }
	}
    }

    if ( mIsUdf(curseg.start) )
	return;

    if ( mIsUdf(curseg.step) ) curseg.step = defstep;
    segments_ += curseg;
}


void PosInfo::CubeData::generate( BinID start, BinID stop, BinID step,
				  bool allowreversed )
{
    erase();

    if ( !allowreversed )
    {
	if ( start.inl() > stop.inl() ) Swap( start.inl(), stop.inl() );
	if ( start.crl() > stop.crl() ) Swap( start.crl(), stop.crl() );
	if ( step.inl() < 0 ) step.inl() = -step.inl();
	if ( step.crl() < 0 ) step.crl() = -step.crl();
    }

    const bool isinlrev = step.inl()<0;
    for ( int iln=start.inl(); isinlrev ? iln>=stop.inl() : iln<=stop.inl();
	      iln+=step.inl() )
    {
	LineData* ld = new LineData( iln );
	ld->segments_ += LineData::Segment( start.crl(), stop.crl(),step.crl());
	*this += ld;
    }
}


void PosInfo::CubeData::copyContents( const PosInfo::CubeData& cd )
{
    if ( &cd != this )
    {
	erase();
	for ( int idx=0; idx<cd.size(); idx++ )
	    *this += new PosInfo::LineData( *cd[idx] );
    }
}


int PosInfo::CubeData::totalSize() const
{
    int totalsize = 0;
    for ( int idx=0; idx<size(); idx++ )
	totalsize += (*this)[idx]->size();

    return totalsize;
}


int PosInfo::CubeData::totalSizeInside( const TrcKeySampling& hrg ) const
{
    int totalsize = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const PosInfo::LineData* linedata = (*this)[idx];
	if ( !hrg.inlOK( linedata->linenr_ ) )
	    continue;

	for ( int idy=0; idy<linedata->segments_.size(); idy++ )
	{
	    const PosInfo::LineData::Segment& segment =
		linedata->segments_[idy];

	    for ( int crl=segment.start; crl<=segment.stop; crl+=segment.step )
	    {
		if ( hrg.crlOK(crl) )
		    totalsize ++;
	    }
	}
    }

    return totalsize;
}


int PosInfo::CubeData::indexOf( int lnr, int* newidx ) const
{
    for ( int idx=0; idx<size(); idx++ )
	if ( (*this)[idx]->linenr_ == lnr )
	    return idx;
    return -1;
}


int PosInfo::SortedCubeData::indexOf( int reqlnr, int* newidx ) const
{
    const int nrld = size();
    if ( nrld < 1 )
	{ if ( newidx ) *newidx = 0; return -1; }

    int loidx = 0;
    int lnr = (*this)[loidx]->linenr_;
    if ( reqlnr <= lnr )
    {
	if ( newidx ) *newidx = 0;
	return reqlnr == lnr ? loidx : -1;
    }
    else if ( nrld == 1 )
	{ if ( newidx ) *newidx = 1; return -1; }

    int hiidx = nrld - 1;
    lnr = (*this)[hiidx]->linenr_;
    if ( reqlnr >= lnr )
    {
	if ( newidx ) *newidx = hiidx+1;
	return reqlnr == lnr ? hiidx : -1;
    }
    else if ( nrld == 2 )
	{ if ( newidx ) *newidx = 1; return -1; }

    while ( hiidx - loidx > 1 )
    {
	const int mididx = (hiidx + loidx) / 2;
	lnr = (*this)[mididx]->linenr_;
	if ( lnr == reqlnr )
	    return mididx;
	else if ( reqlnr > lnr )
	    loidx = mididx;
	else
	    hiidx = mididx;
    }

    if ( newidx )
	*newidx = hiidx;
    return -1;
}


void PosInfo::CubeData::limitTo( const TrcKeySampling& hsin )
{
    TrcKeySampling hs( hsin );
    const bool is2d = hs.is2D();
    hs.normalize();
    for ( int iidx=size()-1; iidx>=0; iidx-- )
    {
	PosInfo::LineData* ld = (*this)[iidx];
	if ( !hs.inlOK(ld->linenr_) )
	{ removeSingle( iidx ); continue; }

	int nrvalidsegs = 0;
	for ( int iseg=ld->segments_.size()-1; iseg>=0; iseg-- )
	{
	    StepInterval<int>& seg = ld->segments_[iseg];
	    const bool isrev = seg.start > seg.stop;
	    int segstart = is2d && isrev ? seg.stop : seg.start;
	    int segstop = is2d && isrev ? seg.start : seg.stop;
	    if ( segstart > hs.stop_.crl() || segstop < hs.start_.crl() )
	    { ld->segments_.removeSingle( iseg ); continue; }

	    if ( is2d && isrev )
		seg.step = -1*seg.step;

	    seg.step = Math::LCMOf( seg.step, hs.step_.crl() );
	    if ( !seg.step )
	    { ld->segments_.removeSingle( iseg ); continue; }

	    if ( segstart < hs.start_.crl() )
	    {
		int newstart = hs.start_.crl();
		int diff = newstart - segstart;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstart = segstart + diff;
		}

		if ( isrev )
		    seg.stop = newstart;
		else
		    seg.start = newstart;
	    }
	    if ( segstop > hs.stop_.crl() )
	    {
		int newstop = hs.stop_.crl();
		int diff = segstop - newstop;
		if ( diff % seg.step )
		{
		    diff += seg.step - diff % seg.step;
		    newstop = segstop - diff;
		}

		if ( isrev )
		    seg.start = newstop;
		else
		    seg.stop = newstop;
	    }
	    if ( segstart > segstop )
		ld->segments_.removeSingle( iseg );
	    else
	    {
		if ( is2d && isrev )
		    seg.step = -1*seg.step;

		nrvalidsegs++;
	    }
	}

	if ( !nrvalidsegs )
	    removeSingle( iidx );
    }
}


bool PosInfo::CubeData::includes( const BinID& bid ) const
{
    return includes( bid.inl(), bid.crl() );
}


bool PosInfo::CubeData::includes( int lnr, int crl ) const
{
    int ilnr = indexOf( lnr ); if ( ilnr < 0 ) return false;
    for ( int iseg=0; iseg<(*this)[ilnr]->segments_.size(); iseg++ )
	if ( (*this)[ilnr]->segments_[iseg].includes(crl,false) )
	    return true;
    return false;
}


void PosInfo::CubeData::getRanges( Interval<int>& inlrg,
				   Interval<int>& crlrg ) const
{
    inlrg.start = inlrg.stop = crlrg.start = crlrg.stop = 0;
    const int sz = size();
    if ( sz < 1 )
	return;

    bool isfirst = true;
    for ( int iln=0; iln<sz; iln++ )
    {
	const PosInfo::LineData& ld = *(*this)[iln];
	if ( ld.segments_.isEmpty() )
	    continue;

	if ( isfirst )
	{
	    isfirst = false;
	    inlrg.start = inlrg.stop = ld.linenr_;
	    crlrg = ld.segments_[0];
	}

	inlrg.include( ld.linenr_ );
	for ( int iseg=0; iseg<ld.segments_.size(); iseg++ )
	    crlrg.include( ld.segments_[iseg], false );
    }
}


bool PosInfo::CubeData::getInlRange( StepInterval<int>& rg,
				     bool wantsorted ) const
{
    const int sz = size();
    if ( sz < 1 ) return false;
    rg.start = rg.stop = (*this)[0]->linenr_;
    if ( sz == 1 )
	{ rg.step = 1; return true; }

    int prevlnr = rg.stop = (*this)[1]->linenr_;
    rg.step = rg.stop - rg.start;
    bool isreg = rg.step != 0;
    if ( !isreg ) rg.step = 1;

    for ( int idx=2; idx<sz; idx++ )
    {
	const int newlnr = (*this)[idx]->linenr_;
	int newstep =  newlnr - prevlnr;
	if ( newstep != rg.step )
	{
	    isreg = false;
	    if ( newstep && abs(newstep) < abs(rg.step) )
	    {
		rg.step = newstep;
		rg.sort( newstep > 0 );
	    }
	}
	rg.include( newlnr, true );
	prevlnr = newlnr;
    }

    rg.sort( wantsorted );
    return isreg;
}


bool PosInfo::CubeData::getCrlRange( StepInterval<int>& rg,
				     bool wantsorted ) const
{
    const int sz = size();
    if ( sz < 1 ) return false;

    const PosInfo::LineData* ld = (*this)[0];
    rg = ld->segments_.size() ? ld->segments_[0] : StepInterval<int>(0,0,1);
    bool foundrealstep = rg.start != rg.stop;
    bool isreg = true;

    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	ld = (*this)[ilnr];
	for ( int icrl=0; icrl<ld->segments_.size(); icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = ld->segments_[icrl];
	    rg.include( seg.start ); rg.include( seg.stop );

	    if ( seg.step && seg.start != seg.stop )
	    {
		if ( !foundrealstep )
		{
		    rg.step = seg.step;
		    foundrealstep = true;
		}
		else if ( rg.step != seg.step )
		{
		    isreg = false;
		    const int segstep = abs(seg.step);
		    const int rgstep = abs(rg.step);
		    if ( segstep < rgstep )
		    {
			rg.step = seg.step;
			rg.sort( seg.step > 0 );
		    }
		}
	    }
	}
    }

    rg.sort( wantsorted );
    return isreg;
}


bool PosInfo::CubeData::isValid( const PosInfo::CubeDataPos& cdp ) const
{
    if ( cdp.lidx_ < 0 || cdp.lidx_ >= size() )
	return false;
    const TypeSet<LineData::Segment>& segs( (*this)[cdp.lidx_]->segments_ );
    if ( cdp.segnr_ < 0 || cdp.segnr_ >= segs.size() )
	return false;
    return cdp.sidx_ >= 0 && cdp.sidx_ <= segs[cdp.segnr_].nrSteps();
}


bool PosInfo::CubeData::isValid( const BinID& bid ) const
{
    const PosInfo::CubeDataPos cdatapos( cubeDataPos(bid) );
    return isValid( cdatapos );
}


bool PosInfo::CubeData::isValid( od_int64 globalidx,
				 const TrcKeySampling& tks ) const
{
    const BinID bid( tks.atIndex(globalidx) );
    return isValid( bid );
}


bool PosInfo::CubeData::toNext( PosInfo::CubeDataPos& cdp ) const
{
    if ( !isValid(cdp) )
    {
	cdp.toStart();
	return isValid(cdp);
    }
    else
    {
	cdp.sidx_++;
	if ( cdp.sidx_ > (*this)[cdp.lidx_]->segments_[cdp.segnr_].nrSteps() )
	{
	    cdp.segnr_++; cdp.sidx_ = 0;
	    if ( cdp.segnr_ >= (*this)[cdp.lidx_]->segments_.size() )
	    {
		cdp.lidx_++; cdp.segnr_ = 0;
		if ( cdp.lidx_ >= size() )
		    return false;
	    }
	}
	return true;
    }
}


BinID PosInfo::CubeData::binID( const PosInfo::CubeDataPos& cdp ) const
{
    return !isValid(cdp) ? BinID(0,0)
	: BinID( (*this)[cdp.lidx_]->linenr_,
		 (*this)[cdp.lidx_]->segments_[cdp.segnr_].atIndex(cdp.sidx_) );
}


PosInfo::CubeDataPos PosInfo::CubeData::cubeDataPos( const BinID& bid ) const
{
    PosInfo::CubeDataPos cdp;
    cdp.lidx_ = indexOf( bid.inl() );
    if ( cdp.lidx_ < 0 )
	return cdp;
    const TypeSet<LineData::Segment>& segs( (*this)[cdp.lidx_]->segments_ );
    for ( int iseg=0; iseg<segs.size(); iseg++ )
    {
	const StepInterval<int>& seg( segs[iseg] );
	if ( seg.includes(bid.crl(),true) )
	{
	    if ( !seg.step || !((bid.crl()-seg.start) % seg.step) )
	    {
		cdp.segnr_ = iseg;
		cdp.sidx_ = seg.getIndex( bid.crl() );
	    }
	    break;
	}
    }
    return cdp;
}


bool PosInfo::CubeData::isFullyRectAndReg() const
{
    const int sz = size();
    if ( sz < 1 ) return true;

    const PosInfo::LineData* ld = (*this)[0];
    if ( ld->segments_.isEmpty() )
	return sz == 1;
    const PosInfo::LineData::Segment seg = ld->segments_[0];

    int lnrstep = mUdf(int);
    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	ld = (*this)[ilnr];
	if ( ld->segments_.isEmpty() )
	    return false;
	if ( ld->segments_.size() > 1 || ld->segments_[0] != seg )
	    return false;
	if ( ilnr > 0 )
	{
	    if ( ilnr == 1 )
		lnrstep = ld->linenr_ - (*this)[ilnr-1]->linenr_;
	    else if ( ld->linenr_ - (*this)[ilnr-1]->linenr_ != lnrstep )
		return false;
	}
    }

    return true;
}


PosInfo::SortedCubeData& PosInfo::SortedCubeData::add( PosInfo::LineData* ld )
{
    return (PosInfo::SortedCubeData&)doAdd( ld );
}


PosInfo::CubeData& PosInfo::SortedCubeData::doAdd( PosInfo::LineData* ld )
{
    if ( !ld ) return *this;

    int newidx;
    const int curidx = indexOf( ld->linenr_, &newidx );
    if ( curidx < 0 )
    {
	if ( newidx >= size() )
	    ManagedObjectSet<LineData>::doAdd( ld );
	else
	    insertAt( ld, newidx );
	return *this;
    }
    LineData* curld = (*this)[curidx];
    if ( ld == curld )
	return *this;

    curld->merge( *ld, true );
    delete ld;
    return *this;
}


bool PosInfo::CubeData::isCrlReversed() const
{
    const int sz = size();
    if ( sz < 1 )
	return false;
    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	const PosInfo::LineData& ld = *(*this)[ilnr];
	if ( ld.segments_.isEmpty() )
	    continue;
	if ( ld.segments_.size() >= 2 )
	{
	    if ( ld.segments_[0].start==ld.segments_[1].start )
	    {
		BufferString msg( "Two segemnts in line nr " );
		msg += ld.linenr_; msg += " have same start";
		pErrMsg( msg );
		continue;
	    }
	    return ld.segments_[0].start > ld.segments_[1].start;
	}
	else
	{
	    if ( ld.segments_[0].start==ld.segments_[0].stop )
		continue;
	    return ld.segments_[0].start > ld.segments_[0].stop;
	}
    }

    return false;
}


bool PosInfo::CubeData::haveCrlStepInfo() const
{
    const int sz = size();
    if ( sz < 1 )
	return false;

    for ( int ilnr=0; ilnr<sz; ilnr++ )
    {
	const PosInfo::LineData& ld = *(*this)[ilnr];
	for ( int icrl=0; icrl<ld.segments_.size(); icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = ld.segments_[icrl];
	    if ( seg.start != seg.stop )
		return true;
	}
    }

    return false;
}


void PosInfo::CubeData::merge( const PosInfo::CubeData& pd1, bool inc )
{
    PosInfo::CubeData pd2( *this );
    deepErase( *this );

    for ( int iln1=0; iln1<pd1.size(); iln1++ )
    {
	const PosInfo::LineData& ld1 = *pd1[iln1];
	const int iln2 = pd2.indexOf( ld1.linenr_ );
	if ( iln2 < 0 )
	{
	    if ( inc ) *this += new PosInfo::LineData(ld1);
	    continue;
	}

	PosInfo::LineData* ld = new PosInfo::LineData( *pd2[iln2] );
	ld->merge( ld1, inc );
	*this += ld;
    }
    if ( !inc ) return;

    for ( int iln2=0; iln2<pd2.size(); iln2++ )
    {
	const PosInfo::LineData& ld2 = *pd2[iln2];
	const int iln = indexOf( ld2.linenr_ );
	if ( iln < 0 )
	    *this += new PosInfo::LineData(ld2);
    }
}




bool PosInfo::CubeData::read( od_istream& strm, bool asc )
{
    const int intsz = sizeof(int);
    int buf[4]; int itmp = 0;
    if ( asc )
	strm >> itmp;
    else
    {
	strm.getBin( buf, intsz );
	itmp = buf[0];
    }
    const int nrinl = itmp;
    if ( nrinl < 0 )
	return false;

    const Interval<int> reasonableinls = SI().reasonableRange( true );
    const Interval<int> reasonablecrls = SI().reasonableRange( false );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	int linenr = 0, nrseg = 0;
	if ( asc )
	    strm >> linenr >> nrseg;
	else
	{
	    strm.getBin( buf, 2 * intsz );
	    linenr = buf[0];
	    nrseg = buf[1];
	}
	if ( linenr == 0 ) continue;

	if ( !reasonableinls.includes( linenr, false ) )
	    return false;


	PosInfo::LineData* iinf = new PosInfo::LineData( linenr );

	PosInfo::LineData::Segment crls;
	for ( int iseg=0; iseg<nrseg; iseg++ )
	{
	    if ( asc )
		strm >> crls.start >> crls.stop >> crls.step;
	    else
	    {
		strm.getBin( buf, 3 * intsz );
		crls.start = buf[0]; crls.stop = buf[1]; crls.step = buf[2];
	    }

	    if ( !reasonablecrls.includes( crls.start,false ) ||
		 !reasonablecrls.includes( crls.stop, false ) )
		    return false;

	    if ( crls.step<1 )
	    {
		if ( crls.step<0 || crls.start!=crls.stop )
		    return false;
	    }

	    iinf->segments_ += crls;
	}

	*this += iinf;
    }

    return true;
}


bool PosInfo::CubeData::write( od_ostream& strm, bool asc ) const
{
    const int intsz = sizeof( int );
    const int nrinl = this->size();
    if ( asc )
	strm << nrinl << '\n';
    else
	strm.addBin( &nrinl, intsz );

    for ( int iinl=0; iinl<nrinl; iinl++ )
    {
	const PosInfo::LineData& inlinf = *(*this)[iinl];
	const int nrcrl = inlinf.segments_.size();
	if ( asc )
	    strm << inlinf.linenr_ << ' ' << nrcrl;
	else
	{
	    strm.addBin( &inlinf.linenr_, intsz );
	    strm.addBin( &nrcrl, intsz );
	}

	for ( int icrl=0; icrl<nrcrl; icrl++ )
	{
	    const PosInfo::LineData::Segment& seg = inlinf.segments_[icrl];
	    if ( asc )
		strm << ' ' << seg.start << ' ' << seg.stop << ' ' << seg.step;
	    else
	    {
		strm.addBin( &seg.start, intsz );
		strm.addBin( &seg.stop, intsz );
		strm.addBin( &seg.step, intsz );
	    }
	    if ( asc )
		strm << '\n';
	}

	if ( !strm.isOK() ) return false;
    }

    return true;
}


PosInfo::CubeDataFiller::CubeDataFiller( CubeData& cd )
    : cd_(cd)
    , ld_(0)
{
    initLine();
}


PosInfo::CubeDataFiller::~CubeDataFiller()
{
    finish();
}


PosInfo::LineData* PosInfo::CubeDataFiller::findLine( int lnr )
{
    const int idxof = cd_.indexOf( lnr );
    return idxof < 0 ? 0 : cd_[idxof];
}


void PosInfo::CubeDataFiller::add( const BinID& bid )
{
    if ( !ld_ || ld_->linenr_ != bid.inl() )
    {
	if ( ld_ )
	    finishLine();
	ld_ = findLine( bid.inl() );
	if ( !ld_ )
	    ld_ = new LineData( bid.inl() );
	else
	{
	    if ( ld_->segmentOf(bid.crl()) >= 0 )
		return;
	    mSetUdf(prevcrl); mSetUdf(seg_.step);
	}
    }

    if ( mIsUdf(prevcrl) )
	prevcrl = seg_.start = seg_.stop = bid.crl();
    else
    {
	const int curstep = bid.crl() - prevcrl;
	if ( curstep != 0 )
	{
	    if ( mIsUdf(seg_.step) )
		seg_.step = curstep;
	    else if ( seg_.step != curstep )
	    {
		ld_->segments_ += seg_;
		seg_.start = bid.crl();
		mSetUdf(seg_.step);
	    }
	    prevcrl = seg_.stop = bid.crl();
	}
    }
}


void PosInfo::CubeDataFiller::finish()
{
    if ( ld_ )
	finishLine();
}


void PosInfo::CubeDataFiller::initLine()
{
    ld_ = 0;
    prevcrl = seg_.start = seg_.stop = seg_.step = mUdf(int);
}


void PosInfo::CubeDataFiller::finishLine()
{
    if ( mIsUdf(seg_.start) )
	delete ld_;
    else
    {
	if ( mIsUdf(seg_.step) )
	{
	    if ( ld_->segments_.isEmpty() )
		seg_.step = SI().crlStep();
	    else
		seg_.step = ld_->segments_[0].step;
	}

	ld_->segments_ += seg_;
	cd_ += ld_;
    }

    initLine();
}
