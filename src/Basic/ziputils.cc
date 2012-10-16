/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Ranojay Sen
Date:          December 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "ziputils.h"

#include "bufstring.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "strmprov.h"
#include "winutils.h"
#include "ziparchiveinfo.h"

#include <iostream>

#define mDirCheck( dir ) \
    if ( !File::exists(dir) ) \
{ \
    errmsg_ = dir; \
    errmsg_ += " does not exist"; \
    return false; \
} \



ZipUtils::ZipUtils( const char* filelistnm )
    : filelistname_( filelistnm )
    , needfilelist_( !filelistname_.isEmpty() )
{}

ZipUtils::~ZipUtils()
{ }

bool ZipUtils::Zip( const char* src, const char* dest )
{
    mDirCheck( src );
    return doZip( src, dest );
}


bool ZipUtils::UnZip( const char* src, const char* dest )
{
    mDirCheck( src );
    mDirCheck( dest );
    return doUnZip( src, dest );
}

bool ZipUtils::doZip( const char* src, const char* dest )
{
    FilePath srcfp( src );
    BufferString newsrc = srcfp.fileName();
    FilePath zipcomfp( GetBinPlfDir(), "zip" );
    BufferString cmd( zipcomfp.fullPath() );
    cmd += " -r \""; 
    cmd += dest;
    cmd += "\"";
    cmd += " ";
    cmd += newsrc;
#ifndef __win__
    File::changeDir( srcfp.pathOnly() );
    const bool ret = !system( cmd );
    File::changeDir( GetSoftwareDir(0) );
    return ret;
#else
    const bool ret = execProc( cmd, true, false, srcfp.pathOnly() );
    return ret;
#endif
}

bool ZipUtils::doUnZip( const char* src, const char* dest )
{
    bool tempfile = false;
    FilePath orgfnm( filelistname_ );
    if ( needfilelist_ )
    {
	if ( !File::exists( orgfnm.pathOnly() ) ) 
	{
	    tempfile = true;
	    FilePath listfp( src );
	    if (  listfp.nrLevels() <= 1 )
		filelistname_ = orgfnm.fileName();
	    else
	    {
		listfp = listfp.pathOnly();
		listfp.add( orgfnm.fileName() );
		filelistname_ = listfp.fullPath();
	    }
	}    
    }

    bool res = false;
#ifdef __win__
    BufferString cmd( "cmd /c unzip -o \"", src );
    cmd.add( "\" -d \"" ).add( dest ).add( "\"");
    if ( needfilelist_ )
	cmd.add( " > " ).add( "\"" ).add( filelistname_ ).add( "\"" );
    res = ExecOSCmd( cmd );
#else
    BufferString cmd( "unzip -o ", src );
    cmd.add( " -d " ).add( dest ).add( " > " )
	.add( needfilelist_ ? filelistname_ : "/dev/null" );
    res = !system( cmd );
#endif

    if ( res && tempfile )
    {
	File::copy( filelistname_, orgfnm.fullPath() );
	File::remove( filelistname_ );
	return true;
    }

    if ( !res )
    {
        errmsg_ = "Unzip failed in the command: ";
        errmsg_ += cmd;
    }
    else
    {
        errmsg_.setEmpty();
    }

    return res;
}


bool ZipUtils::makeZip( const char* zipfnm, const BufferString& src, 
    TaskRunner* tr, ZipHandler::CompLevel cl )
{
    BufferStringSet src2;
    src2.add( src );
    return makeZip( zipfnm, src2, tr, cl );
}



bool ZipUtils::makeZip( const char* zipfnm, const BufferStringSet& src, 
    TaskRunner* tr, ZipHandler::CompLevel cl )
{
    ZipHandler ziphdler_;
    ziphdler_.setCompLevel( cl );
    if ( !ziphdler_.initMakeZip(zipfnm,src) )
    {
	return false;
    }

    Zipper exec( ziphdler_ );
    if ( !(tr ? tr->execute(exec) : exec.execute()) )
    {
	return false;
    }

    return true;
}


bool ZipUtils::appendToArchive( const char* srcfnm, const char* fnm,
    TaskRunner* tr, ZipHandler::CompLevel cl )
{
    ZipHandler ziphdler_;
    ziphdler_.setCompLevel( cl );
    if ( !ziphdler_.initAppend(srcfnm,fnm) )
    {
	return false;
    }

    Zipper exec( ziphdler_ );
    if ( !(tr ? tr->execute( exec ) : exec.execute()) )
    {
	return false;
    }

    return true;
}


int Zipper::nextStep()
{
    if ( ziphd_.getTotalFileCount(nrdir_) == nrdone_ )
    {
	nrdir_++;
	FilePath fp( ziphd_.getAllFileNames().get(nrdone_) );
	ziphd_.setNrLevel( fp.nrLevels() );
    }

    int ret;
    ret = ziphd_.openStrmToRead(ziphd_.getAllFileNames().get(nrdone_));
    if ( ret == 0 )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    if ( ret == 1 )
    {
	ret = ziphd_.doZCompress();
	ziphd_.closeSrcStream();
	if ( !ret )
	{
	    ziphd_.closeDestStream();
	    return ErrorOccurred();
	}

    }

    nrdone_++;
    if ( nrdone_ < totalNr() )
	return MoreToDo();

    int ptrlctn = ziphd_.getDestStream().tellp();
    ret = ziphd_.setCentralDirHeader();
    if ( !ret )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    ret = ziphd_.setEndOfCentralDirHeader ( ptrlctn );
    if ( !ret )
    {
	ziphd_.closeDestStream();
	return ErrorOccurred();
    }

    ziphd_.closeDestStream();
    return Finished();
}

od_int64 Zipper::nrDone() const
{ return nrdone_; }

od_int64 Zipper::totalNr() const
{ return ziphd_.getAllFileNames().size(); }

const char* Zipper::nrDoneText() const
{ return ( "Files" ); }

const char* Zipper::message() const
{ return ziphd_.errorMsg(); }

bool ZipUtils::unZipArchive( const char* srcfnm,const char* basepath,
    TaskRunner* tr )
{
    ZipHandler ziphdler_;
    if ( !ziphdler_.initUnZipArchive(srcfnm,basepath) )
    {
	return false;
    }

    UnZipper exec( ziphdler_ );
    if ( !(tr ? tr->execute( exec ) : exec.execute()) )
    {
	return false;
    }

    return true;
}

bool ZipUtils::unZipFile( const char* srcfnm, const char* fnm, 
    const char* path )
{
    ZipHandler ziphdler_;
    if ( !ziphdler_.unZipFile(srcfnm,fnm,path) )
    {
	return false;
    }

    return true;
}


int UnZipper::nextStep()
{
    bool ret = ziphd_.readFileHeader(); 
    if ( ret == 0)
	return ErrorOccurred();

    nrdone_++;
    return nrDone() < totalNr() ? MoreToDo() : Finished();
}

od_int64 UnZipper::nrDone() const
{ return nrdone_; }

od_int64 UnZipper::totalNr() const
{ return ziphd_.getTotalFileCount(); }

const char* UnZipper::nrDoneText() const
{ return ( "Files" ); }

const char* UnZipper::message() const
{ return ziphd_.errorMsg(); }
