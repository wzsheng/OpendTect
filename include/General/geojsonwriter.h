#pragma once
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		March 2021
________________________________________________________________________

*/

#include "generalmod.h"
#include "uistring.h"
#include "enums.h"
#include "geojson.h"
#include "giswriter.h"
#include "factory.h"


class LatLong;
class SurveyInfo;
class od_ostream;


class XMLItem;

/*!
\brief XML Writer.
*/

mExpClass(General) GeoJSONWriter : public GISWriter
{
mODTextTranslationClass(GeoJSONWriter);
public:
mDefaultFactoryInstantiation( GISWriter, GeoJSONWriter, "GeoJSON",
			      toUiString("GeoJSON") );

			GeoJSONWriter();
			~GeoJSONWriter();


    uiString		errMsg() const override	{ return errmsg_; }
    void		setStream(const BufferString&) override;

    void		setElemName(const char* nm) //!< before open()
			{ elemnm_ = nm; }
    void		setSurveyName(const char* nm) //!< before open()
			{ survnm_ = nm; }

    bool		close() override;

    bool		writePoints(const TypeSet<Coord>&,
			    const BufferStringSet& nms) override;
    bool		writePoint(
			    const RefObjectSet<const Pick::Set>&) override;
    bool		writePoint(const Coord&, const char* nm=0) override;
    bool		writeLine(const TypeSet<Coord>&,
				  const char* nm=0) override;
    bool		writeLine(
			    const RefObjectSet<const Pick::Set>&) override;
    bool		writePolygon(
			    const TypeSet<Coord>&,const char*nm=0) override;
    bool		writePolygon(
			    const TypeSet<Coord3>&,const char*nm=0) override;
    bool		writePolygon(
			    const RefObjectSet<const Pick::Set>&) override;
    BufferString	getExtension() override
			{ return BufferString("geojson"); }


protected:

    BufferString	elemnm_;
    BufferString	survnm_;
    uiString		errmsg_;
    OD::GeoJsonTree*	geojsontree_;
    bool		open(const char* fnm);
    bool		writeGeometry(BufferString,const TypeSet<Coord>&,
					const BufferStringSet&);
    bool		writeGeometry(BufferString,const TypeSet<Coord3>&,
					const BufferStringSet&);
    bool		writeGeometry(BufferString,
					const RefObjectSet<const Pick::Set>&);
};
