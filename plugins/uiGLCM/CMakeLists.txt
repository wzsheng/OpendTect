#_______________________Pmake___________________________________________________
#
#	Makefile : 	GLCM plugin
# 	Jan 2012	C.G. Eichkitz
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiODMain GLCM)
set(OD_MODULE_NAME uiGLCM)
set(OD_IS_PLUGIN yes )
set(OD_MODULE_SOURCES
	uiGLCM_attrib.cc
	uiGLCM_pi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
