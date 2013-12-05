/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2012
 * FUNCTION :
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "nrbytes2string.h"
#include "testprog.h"
#include "bufstringset.h"
#include "iopar.h"


#define mRunTest( desc, test ) \
{ \
    if ( (test) ) \
    { \
	if ( !quiet ) \
	{ \
	    od_cout() << desc << ":"; \
	    od_cout() << " OK\n"; \
	} \
    } \
    else \
    { \
	od_cout() << desc << ":"; \
	od_cout() << " Fail\n"; \
	return false; \
    } \
}


static bool testBytes2String()
{
    NrBytesToStringCreator b2s;
    b2s.setUnitFrom( 100000, true ); //hundred thoughand
    mRunTest( "kB unit (1)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "kB unit (2)",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    b2s.setUnitFrom( 2000000, true ); //Two millions
    mRunTest( "MB unit",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )

    b2s.setUnitFrom( 1000000, true ); //One million
    mRunTest( "Maximum flag turned on",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::MB) )

    b2s.setUnitFrom( 1000000, false ); //One million
    mRunTest( "Maximum flag turned off",
	     b2s.getUnitString()== b2s.toString(NrBytesToStringCreator::KB) )

    mRunTest( "Conversion test", b2s.getString( 100000 )=="97.66 kB" );

    return true;
}

#define mTestStringPrecision( val, strval, isfloat )\
{\
    char buff[255];\
    BufferString valfmtstr;\
    BufferString testname;\
    BufferString valbfstr;\
    if ( isfloat )\
    {\
	testname = "Float precision ";\
	testname += val;\
	valfmtstr = getStringFromFloat( val, buff, 7 );\
	valbfstr =  Conv::to<const char*>((float)val);\
    }\
    else\
    {\
	testname = "Double precision ";\
	testname += val;\
	valbfstr =  Conv::to<const char*>((double)val);\
	valfmtstr = getStringFromDouble( val, buff, 15 );\
    }\
    mRunTest( testname.buf(), valfmtstr ==valbfstr && valbfstr == strval );\
}

static bool testStringPrecisionInAscII()
{
    mTestStringPrecision( 0, "0", true );
    mTestStringPrecision( 0.1, "0.1", true );
    mTestStringPrecision( 0.05, "0.05", true );
    mTestStringPrecision( 0.001, "0.001", true );
    mTestStringPrecision( 0.023, "0.023", true );
    //mTestStringPrecision( 0.0001, "0.0001", true );
    mTestStringPrecision( 0.00001, "0", true );
    mTestStringPrecision( 12.3456785, "12.34568", true );
    mTestStringPrecision( 123.4567852, "123.4568", true );
    mTestStringPrecision( 1234.567352, "1234.567", true );
    mTestStringPrecision( 12345.67352, "12345.67", true );
    mTestStringPrecision( 123456.7552, "123456.8", true );
    mTestStringPrecision( 0.00001, "0.00001", false );
    mTestStringPrecision( 0.10010001002030415,"0.1001000100203", false );
    mTestStringPrecision( 12.32568023782987356,"12.3256802378299", false );
    mTestStringPrecision( 123.2568023782987353,"123.256802378299", false );
    mTestStringPrecision( 1232.568023782927357,"1232.56802378293", false );
    mTestStringPrecision( 12325.68023782981352,"12325.6802378298", false );
    mTestStringPrecision( 123256.8023782984888,"123256.802378298", false );
    return true;
}


static bool testBufferStringFns()
{
    BufferString bs( "Tok1 Tok1 Tok2 Tok3 Tok4" );
    bs.replace( '4', '5' );
    bs.replace( "Tok1", 0 );
    bs.replace( "Tok2", " " );
    bs.replace( "Tok3", "X" );
    mRunTest("BufferString replace",bs == "    X Tok5");
    bs.trimBlanks();
    mRunTest("BufferString trimBlanks 1",bs == "X Tok5");
    bs = "\nXX\tYY Z\t";
    bs.trimBlanks();
    mRunTest("BufferString trimBlanks 2",bs == "XX\tYY Z");
    mRunTest("BufferString count",bs.count('Y')==2);
    mRunTest("BufferString contains 1",bs.contains(" Z"));
    mRunTest("BufferString contains 2",!bs.contains("\n"));
    bs = "\nXX\tYY Z\t";
    bs.remove( '\t' );
    mRunTest("BufferString remove 1",bs == "\nXXYY Z");
    bs.remove( "XX" );
    mRunTest("BufferString remove 2",bs == "\nYY Z");
    bs = "XXYYZ";
    bs.embed( '^', '+' );
    mRunTest("BufferString embed",bs == "^XXYYZ+");
    bs.quote( '=' );
    mRunTest("BufferString quote",bs == "=^XXYYZ+=");
    bs.unQuote( '=' );
    mRunTest("BufferString unQuote",bs == "^XXYYZ+");
    bs.unEmbed( '^', '+' );
    mRunTest("BufferString unEmbed",bs == "XXYYZ");

    return true;
}


#define mRunFirstOccTest(tofind,offs) \
    mRunTest( BufferString("firstOcc ",tofind), \
	    firstOcc(str,tofind)-str == offs)

#define mRunLastOccTest(tofind,offs) \
    mRunTest( BufferString("lastOcc ",tofind), \
	    lastOcc(str,tofind)-str == offs)

static bool testOccFns()
{
    const char* str = "[BEGIN] Tok1 Tok2 Tok1 Tok3 [END]";
    const int len = FixedString(str).size();

    mRunFirstOccTest('N',5);
    mRunFirstOccTest("N",5);
    mRunFirstOccTest("Tok1",8);
    mRunFirstOccTest("",0);

    mRunLastOccTest('N',len-3);
    mRunLastOccTest("N",len-3);
    mRunLastOccTest("Tok1",len-15);
    mRunLastOccTest("",len);

    mRunTest( "firstOcc non-exist", firstOcc(str,"Tok4") == 0 )
    mRunTest( "firstOcc non-exist", firstOcc(str,'X') == 0 )
    mRunTest( "lastOcc non-exist", lastOcc(str,"Tok4") == 0 )
    mRunTest( "lastOcc non-exist", lastOcc(str,'X') == 0 )

    return true;
}



int main( int argc, char** argv )
{
    mInitTestProg();

    if ( !testBytes2String()
      || !testStringPrecisionInAscII()
      || !testBufferStringFns()
      || !testOccFns() )
	ExitProgram( 1 );

    BufferStringSet strs;
    strs.add( "Str pos 0" );
    strs.add( "" );
    strs.add( "Str pos 2" );
    IOPar iop; strs.fillPar( iop );
    strs.setEmpty(); strs.usePar( iop );
    mRunTest( "BufferString use/fillPar test", strs.get(2)=="Str pos 2" );

    if ( !quiet )
    {
	FixedString str( 0 );
	od_cout() << "Should be empty: '" << str << "'" << od_endl;
    }

    return ExitProgram( 0 );
}
