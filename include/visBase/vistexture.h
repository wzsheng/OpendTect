#ifndef vistexture_h
#define vistexture_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vistexture.h,v 1.21 2007-10-10 03:59:24 cvsnanne Exp $
________________________________________________________________________


-*/

#include "visdata.h"
#include "scaler.h"

class DataClipper;
class BasicTask;
class Color;
class IOPar;
class SoComplexity;
class SoGroup;
class SoSwitch;
class visBaseTextureColorIndexMaker;
template <class T> class Interval;

namespace visBase
{

class VisColorTab;
class VisColTabMod;
class ThreadWorker;

/*!\brief
is a base class for Texture2 and Texture3 and should not be used directly.

A number of caches are implemented to minimize the calculation-time
when the colortable is updated.

If ThreadWorker is set, it utilizes mt processing.

*/

class Texture : public DataObject
{
public:
    enum		DataType { Color, Transparency,
				   Hue, Saturation, Brightness, 
				   Red, Green, Blue };
    virtual bool	turnOn(bool yn);
    virtual bool	isOn()	const;
    			
    void		setAutoScale(bool yn);
    bool		autoScale() const;

    void		setColorTab(VisColorTab&);
    VisColorTab&	getColorTab();

    void		setClipRate( float );
    			/*!< Relayed to colortable */
    float		clipRate() const;
    			/*!< Relayed to colortable */

    void		setResolution(int res)		{ resolution = res; }
    int			getResolution() const		{ return resolution; }

    const TypeSet<float>& getHistogram() const;

    void		setUseTransperancy(bool yn);
    bool		usesTransperancy() const;

    void		setThreadWorker(ThreadWorker*);
    ThreadWorker*	getThreadWorker();

    void		setTextureQuality(float);
    float		getTextureQuality() const;

    VisColTabMod&	getColTabMod()		{ return *coltabmod; }
    const VisColTabMod&	getColTabMod() const	{ return *coltabmod; }
    void		setColorPars(bool,bool,const Interval<float>&);
    const Interval<float>& getColorDataRange() const;

    SoNode*		getInventorNode();

    virtual void        fillPar(IOPar&,TypeSet<int>&) const;
    virtual int         usePar(const IOPar&);

protected:
    			Texture();
    			~Texture();
    void		setResizedData(float*,int sz,DataType);
    			/*!< Is taken over by me */
    int			nextPower2(int,int,int) const;

    virtual unsigned char* getTexturePtr()				= 0;
    virtual void	finishEditing()					= 0;

    SoSwitch*		onoff;
    SoGroup*		texturegrp;
    SoComplexity*	quality;

    int			resolution;

protected:
    void		colorTabChCB(CallBacker*);
    void		colorSeqChCB(CallBacker*);
    void		autoscaleChCB(CallBacker*);

    virtual void	clipData();
    virtual void	makeColorIndexes();
    virtual void	makeTexture();
    virtual void	makeColorTables();
    virtual void	clearDataCache(bool);

    float*		datacache;
    float*		colordatacache;
    unsigned char*	indexcache;
    int			cachesize;
    int			indexcachesize;

    TypeSet<LinScaler>	datascales;
			/*!<\note The first entry is not used since
				  the color range is in the coltab
			*/

    ::Color*		colortabcolors;

    unsigned char*	red;
    unsigned char*	green;
    unsigned char*	blue;
    unsigned char*	trans;

    TypeSet<float>	histogram;

    bool		usetrans;

    DataType		curtype;
    VisColTabMod*	coltabmod;
    VisColorTab*	colortab;
    ObjectSet<visBaseTextureColorIndexMaker> colorindexers;
    ObjectSet<BasicTask> texturemakers;
    ThreadWorker*	threadworker;

    static const char*	colortabstr;
    static const char*	texturequalitystr;
    static const char*	usestexturestr;
    static const char*	resolutionstr;
    static const char*	coltabmodstr;
};

};

#endif
