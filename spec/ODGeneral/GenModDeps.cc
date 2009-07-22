/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2002
 * FUNCTION : Generate file to include in make.Vars
-*/

static const char* rcsID = "$Id: GenModDeps.cc,v 1.17 2009-07-22 16:01:29 cvsbert Exp $";

#include "prog.h"
#include "strmprov.h"
#include "bufstringset.h"
#include "filepath.h"
#include "timefun.h"
#include <stdlib.h>
#include <iostream>

class Dep
{
public:
    			Dep( const char* m )
			    : name(m)	{}
    bool		operator ==( const char* s ) const
			    { return name == s; }

    BufferString	name;
    BufferStringSet	mods;
};


int main( int argc, char** argv )
{
    int arg1 = 1; bool html = false;
    if ( argc >= 3 && !strcmp(argv[1],"--html") )
	{ html = true; arg1 = 2; }
    if ( argc-arg1+1 < 3 )
    {
	std::cerr << "Usage: " << argv[0]
	     << " [--html] input_ModDeps_file output_file" << std::endl;
	ExitProgram( 1 );
    }
    StreamProvider spin( argv[arg1] );
    StreamData sdin = spin.makeIStream();
    if ( !sdin.usable() )
    {
	std::cerr << argv[0] << ": Cannot open input stream" << std::endl;
	ExitProgram( 1 );
    }
    else if ( sdin.istrm == &std::cin )
	std::cout << "Using standard input." << std::endl;
    std::istream& instrm = *sdin.istrm;

    if ( !instrm )
    {
	std::cerr << "Bad ModDeps file" << std::endl;
	ExitProgram( 1 );
    }
    StreamProvider spout( argv[arg1+1] );
    StreamData sdout = spout.makeOStream();
    if ( !sdout.usable() )
    {
	std::cerr << argv[0] << ": Cannot open output stream" << std::endl;
	ExitProgram( 1 );
    }
    std::ostream& outstrm = *sdout.ostrm;

    char linebuf[1024];
    char wordbuf[256];
    ObjectSet<Dep> deps;
    while ( instrm )
    {
	instrm.getline( linebuf, 1024 );
	char* bufptr = linebuf; 
	mTrimBlanks(bufptr);
	if ( ! *bufptr || *bufptr == '#' )
	    continue;

	char* nextptr = (char*)getNextWord(bufptr,wordbuf);
	if ( ! wordbuf[0] ) continue;
	od_int64 l = strlen( wordbuf );
	if ( wordbuf[l-1] == ':' ) wordbuf[l-1] = '\0';
	if ( ! wordbuf[0] ) continue;

	*nextptr++ = '\0';
	mSkipBlanks(nextptr);

	Dep* newdep = new Dep( wordbuf ) ;
	BufferStringSet filedeps;
	while ( nextptr && *nextptr )
	{
	    mSkipBlanks(nextptr);
	    nextptr = (char*)getNextWord(nextptr,wordbuf);
	    if ( !wordbuf[0] ) break;

	    if ( wordbuf[1] != '.' || (wordbuf[0] != 'S' && wordbuf[0] != 'D') )
		{ std::cerr << "Cannot handle dep=" << wordbuf << std::endl;
		    ExitProgram(1); }

	    filedeps.add( wordbuf );
	}


	BufferStringSet depmods;
	for ( int idx=filedeps.size()-1; idx>=0; idx-- )
	{
	    const char* filedep = filedeps.get(idx).buf();
	    const char* modnm = filedep + 2;
	    if ( *filedep == 'S' )
	    {
		depmods.add( modnm );
	        continue;
	    }

	    Dep* depdep = find( deps, modnm );
	    if ( !depdep )
		{ std::cerr << "Cannot find dep=" << modnm << std::endl;
		    		ExitProgram(1); }

	    for ( int idep=depdep->mods.size()-1; idep>=0; idep-- )
	    {
		const char* depdepmod = depdep->mods.get(idep).buf();
		if ( depmods.indexOf(depdepmod) < 0 )
		    depmods.add( depdepmod );
	    }
	}
	if ( depmods.size() < 1 )
	    { delete newdep; continue; }
	deps += newdep;

	for ( int idx=depmods.size()-1; idx>=0; idx-- )
	    newdep->mods += depmods[idx];
    }
    sdin.close();

    if ( html )
    {
	outstrm << "<html>\n<!-- Generated by " << FilePath(argv[0]).fileName()
	        << " (" << Time_getFullDateString()
	        << ") -->\n\n\n<body bgcolor=\"#dddddd\">"
		    "<title>OpendTect source documentation</title>\n\n"
		    "<center><h3>Open"
		<< GetProjectVersionName()
		<< " modules</h3></center>\n\n"
		   "<TABLE align=\"center\" summary=\"Modules\""
			   "WIDTH=\"75%\" BORDER=\"1\">"
		<< std::endl;

	BufferStringSet allmods;
	for ( int idx=0; idx<deps.size(); idx++ )
	    allmods.add( deps[idx]->name );
	allmods.sort();

	for ( int idep=0; idep<allmods.size(); idep++ )
	{
	    const BufferString& nm = allmods.get(idep);
	    outstrm << "<TR align=center>\n";

	    outstrm << " <TD><b>" << nm << "</b></TD>\n <TD><a href=\""
	    << nm << "/index.html\">Main Page</a></TD>\n <TD><a href=\""
	    << nm << "/classes.html\">Class Index</a></TD>\n <TD><a href=\""
	    << nm << "/hierarchy.html\">Class Hierarchy</a></TD>\n <TD><a href=\""
	    << nm << "/annotated.html\">Compound List</a></TD>\n</TR>\n\n";
	}

	outstrm << "\n</TABLE>\n\n<body>\n<html>" << std::endl;
    }

    else
    {
	for ( int idx=0; idx<deps.size(); idx++ )
	{
	    Dep& dep = *deps[idx];

	    outstrm << 'L' << dep.name << " :=";
	    for ( int idep=0; idep<dep.mods.size(); idep++ )
		outstrm << " -l" << (const char*)(*dep.mods[idep]);
	    outstrm << std::endl;
	    outstrm << 'I' << dep.name << " :=";
	    for ( int idep=0; idep<dep.mods.size(); idep++ )
		outstrm << ' ' << (const char*)(*dep.mods[idep]);
	    outstrm << std::endl;

	}
    }


    sdout.close();
    ExitProgram( 0 ); return 0;
}
