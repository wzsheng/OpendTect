#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiEMAttrib
# 	October 2004	N. Hemstra
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiAttributes uiEarthModel EMAttrib )
set(OD_MODULE_SOURCES
	uiattremout.cc
	uiattrsurfout.cc
	uiattrtrcselout.cc
	uicreate2dgrid.cc
	uiemattribpartserv.cc
	uiflattenedcube.cc
	uihor2dfrom3ddlg.cc
	uihorizonattrib.cc
	uihorizonshiftdlg.cc
	uiimphorizon2d.cc
	uiseiseventsnapper.cc
	uistratamp.cc
	horizon2dscanner.cc
	inituiemattrib.cc
)
OD_INIT_MODULE()
