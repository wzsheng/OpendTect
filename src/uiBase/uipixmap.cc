/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          08/12/1999
________________________________________________________________________

-*/

#include "uipixmap.h"

#include "arraynd.h"
#include "coltabindex.h"
#include "coltabsequence.h"
#include "file.h"
#include "filepath.h"
#include "odiconfile.h"
#include "settings.h"

#include "uirgbarray.h"

#include <QBitmap>
#include <QColor>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QPixmap>

mUseQtnamespace


uiPixmap::uiPixmap()
    : qpixmap_(nullptr)
{}


uiPixmap::uiPixmap( const uiPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


uiPixmap::uiPixmap( const uiRGBArray& rgbarr )
    : qpixmap_(new QPixmap)
    , srcname_("[uiRGBArray]")
{
    convertFromRGBArray( rgbarr );
}


uiPixmap::uiPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


uiPixmap::uiPixmap( int w, int h )
    : qpixmap_(new QPixmap( w<2 ? 1 : w, h<2 ? 2 : h ))
    , srcname_("[created]")
{
    qpixmap_->fill( QColor(0,0,0,0) );
}


uiPixmap::uiPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


uiPixmap::uiPixmap( const char* icnm )
    : qpixmap_(nullptr)
    , srcname_(icnm)
{
    OD::IconFile icfile( icnm );
    if ( !icfile.haveData() )
	{ qpixmap_ = new QPixmap; return; }

    qpixmap_ = new QPixmap( icfile.fileNames().get(0).str(), nullptr );
}


uiPixmap::~uiPixmap()
{
    delete qpixmap_;
}


void uiPixmap::convertFromRGBArray( const uiRGBArray& rgbarr )
{
    if ( !qpixmap_ ) qpixmap_ = new QPixmap;
    *qpixmap_ = QPixmap::fromImage( rgbarr.qImage(),
				    Qt::OrderedAlphaDither );
}


int uiPixmap::width() const
{ return qpixmap_->width(); }

int uiPixmap::height() const
{ return qpixmap_->height(); }

bool uiPixmap::isEmpty() const
{ return !qpixmap_ || qpixmap_->isNull(); }


void uiPixmap::scale( int w, int h )
{
    if ( !qpixmap_ )
	return;

    QPixmap newpm = qpixmap_->scaled( w, h, Qt::KeepAspectRatio );
    if ( newpm.isNull() )
	return;

    *qpixmap_ = newpm;
}


void uiPixmap::scaleToHeight( int h )
{
    if ( !qpixmap_ )
	return;

    const QPixmap newpm = qpixmap_->scaledToHeight( h );
    if ( newpm.isNull() )
	return;

    *qpixmap_ = newpm;
}


void uiPixmap::scaleToWidth( int w )
{
    if ( !qpixmap_ )
	return;

    const QPixmap newpm = qpixmap_->scaledToWidth( w );
    if ( newpm.isNull() )
	return;

    *qpixmap_ = newpm;
}


void uiPixmap::fill( const OD::Color& col )
{
    qpixmap_->fill( QColor(col.r(),col.g(),col.b()) );
    QPainter painter( qpixmap_ );
    painter.drawRect( 0, 0, qpixmap_->width()-1, qpixmap_->height()-1 );
}


void uiPixmap::fill( const ColTab::Sequence& seq, bool hor )
{
    srcname_ = "[colortable]";

    const bool validsz = width()>=2 && height()>=2;
    if ( seq.isEmpty() || !validsz )
    {
	qpixmap_->fill( QColor(0,0,0,0) );
	return;
    }

    uiRGBArray rgbarr( false );
    rgbarr.setSize( width(), height() );
    if ( hor )
    {
	ColTab::IndexedLookUpTable table( seq, width() );
	for ( int idx1=0; idx1<rgbarr.getSize(true); idx1++ )
	{
	    const OD::Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getSize(false); idx2++ )
		rgbarr.set( idx1, idx2, color );
	}
    }
    else // vertical colorbar
    {
	ColTab::IndexedLookUpTable table( seq, height() );
	const int rgbheight = rgbarr.getHeight();
	for ( int idx1=0; idx1<rgbheight; idx1++ )
	{
	    const int arridx = rgbheight - idx1 - 1;
	    const OD::Color color = table.colorForIndex( idx1 );
	    for ( int idx2=0; idx2<rgbarr.getWidth(); idx2++ )
		rgbarr.set( idx2, arridx, color );
	}
    }

    convertFromRGBArray( rgbarr );

    QPainter painter( qpixmap_ );
    painter.setPen( QColor(100,100,100) );
    painter.drawRect( 0, 0, qpixmap_->width()-1, qpixmap_->height()-1 );
}


void uiPixmap::fillGradient( const OD::Color& col1, const OD::Color& col2,
			      bool hor )
{
    srcname_ = "[Gradient]";

    const bool validsz = width()>=2 && height()>=2;
    if ( !validsz )
    {
	qpixmap_->fill( QColor(0,0,0,0) );
	return;
    }

    uiRGBArray rgbarr( false );
    rgbarr.setSize( width(), height() );
    if ( hor )
    {
	for ( int idx1=0; idx1<rgbarr.getSize(true); idx1++ )
	{
	    const float frac = idx1 / (float) width();
	    const OD::Color color = OD::Color::interpolate( col1, col2, frac );
	    for ( int idx2=0; idx2<rgbarr.getSize(false); idx2++ )
		rgbarr.set( idx1, idx2, color );
	}
    }
    else // vertical colorbar
    {
	for ( int idx1=0; idx1<rgbarr.getSize(false); idx1++ )
	{
	    const float frac = idx1 / (float) height();
	    const OD::Color color = OD::Color::interpolate( col1, col2, frac );
	    for ( int idx2=0; idx2<rgbarr.getSize(true); idx2++ )
		rgbarr.set( idx2, idx1, color );
	}
    }

    convertFromRGBArray( rgbarr );

    QPainter painter( qpixmap_ );
    painter.setPen( QColor(100,100,100) );
    painter.drawRect( 0, 0, qpixmap_->width()-1, qpixmap_->height()-1 );
}


bool uiPixmap::save( const char* fnm, const char* fmt, int quality ) const
{ return qpixmap_ ? qpixmap_->save( fnm, fmt, quality ) : false; }


bool uiPixmap::isPresent( const char* icnm )
{
    return OD::IconFile::isPresent( icnm );
}

static int sPDFfmtIdx = 6;

static const char* sImageFormats[] =
{ "jpg", "png", "tiff", "webp", "bmp", "xpm", "pdf", nullptr };

static const char* sImageFormatDescs[] =
{
    "JPEG (*.jpg *.jpeg)",
    "PNG (*.png)",
    "TIFF (*.tif *.tiff)",
    "WebP (*.webp)",
    "Windows Bitmap (*.bmp)",
    "XPM (*.xpm)",
    "Portable Doc Format (*.pdf)",
    nullptr
};


void supportedImageFormats( BufferStringSet& formats, bool forread,
			    bool withprintformats )
{
    QList<QByteArray> imgfrmts = forread
	? QImageReader::supportedImageFormats()
	: QImageWriter::supportedImageFormats();

    for ( int idx=0; idx<imgfrmts.size(); idx++ )
	formats.add( imgfrmts[idx].data() );

    if ( withprintformats )
	formats.add( sImageFormats[sPDFfmtIdx] );
}


void getImageFormatDescs( BufferStringSet& descs, bool forread,
			  bool withprintformats )
{
    BufferStringSet formats; supportedImageFormats( formats, forread );

    int idx = 0;
    while ( sImageFormats[idx] )
    {
	if ( formats.isPresent(sImageFormats[idx]) )
	    descs.add( sImageFormatDescs[idx] );
	idx++;
    }

    if ( withprintformats )
	descs.add( sImageFormatDescs[sPDFfmtIdx] );
}


void getImageFileFilter( BufferString& filter, bool forread,
			 bool withprintformats )
{
    BufferStringSet descs;
    getImageFormatDescs( descs, forread, withprintformats );
    filter = descs.cat( ";;" );
}
