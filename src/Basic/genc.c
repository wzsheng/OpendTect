/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.37 2004-01-21 12:47:41 dgb Exp $";

#include "genc.h"
#include "filegen.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
#include <unistd.h>
#else
#include <process.h>
#include <float.h>
#include "windows.h"
#include "getspec.h"	// GetSpecialFolderLocation()
#endif

#include "debugmasks.h"

static FileNameString filenamebuf;
static FileNameString surveyname;
static int surveynamedirty = YES;


int SurveyNameDirty()
{
    return surveynamedirty;
}


void SetSurveyNameDirty()
{
    surveynamedirty = 1;
}


const char* GetSoftwareDir()
{
    const char* dir = 0;
#ifdef __win__
    dir = getenv( "DTECT_WINAPPL" );
    if ( !dir ) dir = getenv( "dGB_WINAPPL" );
#else
    dir = getenv( "DTECT_APPL" );
    if ( !dir ) dir = getenv( "dGB_APPL" );
#endif


    if ( !dir )
    {
	if ( !getenv("dGB_ARGV0") ) return 0;

	static FileNameString progname;
	strcpy( progname, getenv("dGB_ARGV0") );

	if( !*progname ) return 0;


	char* chptr1 = progname;
	char* chptr2 = chptr1;
	while ( chptr2 = strstr( chptr1 + 3 , "bin" ) )
	    chptr1 = chptr2;

	if ( !chptr1 ) return 0;

	*chptr1-- = '\0';

	/* Remove trailing dirseps from pathbuf */
	const char* dirsep = sDirSep;
	while ( chptr1 != progname-1 && *chptr1 == *dirsep ) *chptr1-- = '\0';

	dir = progname;
    }

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareDir: %s\n", dir );
	dgb_debug_message( buf );
    }

    return dir;
}


const char* GetExecScript( int remote )
{
    static FileNameString progname;

    strcpy( progname, "'" );
    strcat( progname, GetSoftwareDir() );
    strcpy( progname, File_getFullPath(progname, "bin") );
#ifdef __win__
    strcpy( progname, File_getFullPath(progname, "win") );
#endif
    strcpy( progname, File_getFullPath(progname, "od_exec") );

    if( remote )
	strcat( progname, "_rmt" );

#ifdef __win__
    strcat( progname, ".csh" );
#endif

    strcat( progname, "' " );
    return progname;
}


const char* GetDataFileName( const char* fname )
{
    strcpy( filenamebuf, File_getFullPath( GetSoftwareDir(), "data" ) );
    if ( fname && *fname )
	strcpy( filenamebuf, File_getFullPath( filenamebuf, fname ) );

    if ( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataFileName for %s: %s\n", fname ? fname : "(null)",
			filenamebuf );
	dgb_debug_message( buf );
    }

    return filenamebuf;
}


const char* GetSoftwareUser()
{
    const char* ptr = 0;
#ifdef __win__

    ptr = getenv( "DTECT_WINUSER" );
    if ( !ptr ) ptr = getenv( "dGB_WINUSER" );

#endif

    if ( !ptr ) ptr = getenv( "DTECT_USER" );
    if ( !ptr ) ptr = getenv( "dGB_USER" );

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSoftwareUser: %s\n", ptr ? ptr : "Not set" );
	dgb_debug_message( buf );
    }

    return ptr;
}

const char* _GetHomeDir()
{
#ifdef __win__

    static FileNameString home = "";

    const char* ptr = getenv( "DTECT_WINHOME" );
    if ( !ptr ) ptr = getenv( "dGB_WINHOME" );


    if ( ptr && *ptr ) return ptr;

    strcpy( home, getenv("HOMEDRIVE") );
    strcat( home, getenv("HOMEPATH") );

    if( strcmp( home, "" ) && strcmp( home, "c:\\" ) && strcmp( home, "C:\\" ) 
	&& File_isDirectory( home ) ) // Apparantly, home has been set...
	return home;

    return 0;

#else

    const char* ptr = getenv( "DTECT_HOME" );
    if ( !ptr ) ptr = getenv( "dGB_HOME" );
    if ( !ptr ) ptr = getenv( "HOME" );
    return ptr;

#endif
}

const char* GetSettingsDir(void)
{
    const char* ptr = _GetHomeDir();

#ifdef __win__
    if ( !ptr )
	ptr = getenv( "APPDATA" ); // should be set by OS

    if ( !ptr )
	ptr = getenv( "DTECT_APPLICATION_DATA" ); // set by init script

    if ( !ptr ) // Last resort. Is known to cause problems when used 
                // during initialisation of statics. (0xc0000005)
	ptr = GetSpecialFolderLocation( CSIDL_APPDATA ); // "Application Data"

    if ( !ptr )
	return 0;

    char* chptr = ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    ptr = File_getFullPath( ptr, ".od" );

    if ( !File_isDirectory(ptr) )
    {
	if ( File_exists(ptr) ) ptr = 0;
	else if ( !File_createDir(ptr,0) ) ptr = 0;
    }

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSettingsDir: %s\n", ptr );
	dgb_debug_message( buf );
    }

    return ptr;
}


const char* GetPersonalDir(void)
{
    const char* ptr = _GetHomeDir();

#ifdef __win__

    if ( !ptr ) 
	ptr = getenv( "USERPROFILE" ); // should be set by OS

    if ( !ptr ) 
	ptr = getenv( "DTECT_USERPROFILE_DIR" );

    if ( !ptr ) 
	ptr = getenv( "DTECT_MYDOCUMENTS_DIR" );

    if ( !ptr ) // Last resort. Is known to cause problems when used 
                // during initialisation of statics. (0xc0000005)
	ptr = GetSpecialFolderLocation( CSIDL_PROFILE ); // "User Profile"

    char* chptr = ptr;
    while ( chptr && *chptr++ ) { if ( *chptr == '\r' ) *chptr='\0'; }

#endif

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetPersonalDir: %s\n", ptr );
	dgb_debug_message( buf );
    }

    return ptr;
}


const char* GetSurveyFileName()
{
    static FileNameString sfname;
    static int inited = NO;
    const char* ptr;

    if ( !inited )
    {
	ptr = GetSettingsDir();
	if ( !ptr ) return 0;
	strcpy( sfname, File_getFullPath(ptr,"survey") );
	ptr = GetSoftwareUser();
	if ( ptr )
	{
	    strcat( sfname, "." );
	    strcat( sfname, ptr );
	}
	inited = YES;
    }

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyFileName: %s\n", sfname );
	dgb_debug_message( buf );
    }

    return sfname;
}


void SetSurveyName( const char* newnm )
{
    strcpy( surveyname, newnm );
    surveynamedirty = 0;
}


const char* GetSurveyName()
{
    int len;
    FILE* fp;
    const char* fnm;

    if ( surveynamedirty )
    {
	fnm = GetSurveyFileName();
	if ( !fnm ) return 0;

	fp = fopen( fnm, "r" );
	if ( !fp ) return 0;

	surveyname[0] = '\0';
	fgets( surveyname, PATH_LENGTH, fp );
	len = strlen( surveyname );
	if ( len == 0 ) return 0;
	if ( surveyname[len-1] == '\n' ) surveyname[len-1] = '\0';

	fclose( fp );
	surveynamedirty = 0;
    }

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetSurveyName: %s\n", surveyname );
	dgb_debug_message( buf );
    }

    return surveyname;
}


extern const char* GetSettingsDataDir();

const char* GetBaseDataDir()
{
    const char* dir = 0;

#ifdef __win__
    dir = getenv( "DTECT_WINDATA" );
    if ( !dir ) dir = getenv( "dGB_WINDATA" );
#endif
    if ( !dir ) dir = getenv( "DTECT_DATA" );
    if ( !dir ) dir = getenv( "dGB_DATA" );

    if ( !dir ) dir = GetSettingsDataDir();

    return dir;
}


const char* GetDataDir()
{
    const char* survnm;
    const char* basedir = GetBaseDataDir();
    if ( !basedir || !*basedir ) return 0;

    survnm = GetSurveyName();
    if ( !survnm || !*survnm ) return basedir;

    strcpy( filenamebuf, File_getFullPath(basedir,survnm) );

    if( dgb_debug_isOn(DBG_SETTINGS) )
    {
	char buf[255];
	sprintf(buf, "GetDataDir: %s\n", filenamebuf );
	dgb_debug_message( buf );
    }

    return filenamebuf;
}


void swap_bytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    { 
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void put_platform( unsigned char* ptr )
{
#if defined(lux) || defined(__win__)
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#define getpid	_getpid
#endif

int getPID()
{
    return getpid();
}


int setEnvVar( const char* env, const char* val )
{
    char* buf;
    if ( !env || !*env ) return NO;
    if ( !val ) val = "";

    buf = mMALLOC( strlen(env)+strlen(val) + 2, char );
    strcpy( buf, env );
    if ( *val ) strcat( buf, "=" );
    strcat( buf, val );

    putenv( buf );
    return YES;
}

void exitProgram( int ret )
{

#ifdef __win__

#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

    // open process
    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, getPID() );
    if ( isBadHandle( hProcess ) )
	printf( "OpenProcess() failed, err = %lu\n", GetLastError() );
    else
    {
	// kill process
	if ( ! TerminateProcess( hProcess, (DWORD) -1 ) )
	    printf( "TerminateProcess() failed, err = %lu\n", GetLastError() );

	// close handle
	CloseHandle( hProcess );
    }
#endif

    exit(ret);
}


double IntPowerOf( double x, int y )
{
    double ret = 1;
    if ( x == 0 ) return y ? 0 : 1;

    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}


double PowerOf( double x, double y )
{
    int isneg = x < 0 ? 1 : 0;
    double ret;
 
    if ( x == 0 ) return y ? 0 : 1;
    if ( isneg ) x = -x;
 
    ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


#ifdef sun5
# include <ieeefp.h>
#endif

#ifdef __msvc__
# define finite	_finite
#endif

int isFinite( double v )
{
    return finite(v);
}
