#ifndef attribfactory_h
#define attribfactory_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
 RCS:           $Id: attribfactory.h,v 1.8 2010-10-07 17:29:19 cvshelene Exp $
________________________________________________________________________

-*/

#include "sets.h"

namespace Attrib
{

class Desc;
class Provider;

typedef Provider* (*ProviderCreater)(Desc&);

mClass ProviderFactory
{
public:
			ProviderFactory();
			~ProviderFactory();
			
			/*Interface from attribs' initClass() */
    void		addDesc( Desc*, ProviderCreater );

    Provider*		create( Desc& ) const;
    Desc*		createDescCopy( const char* nm ) const;
    void		updateAllDescsDefaults();

protected:
    int				indexOf( const char* ) const;
    ObjectSet<Desc>		descs_;
    TypeSet<ProviderCreater>	creaters_;
};

mGlobal extern ProviderFactory& PF();

}; //Namespace

#endif

