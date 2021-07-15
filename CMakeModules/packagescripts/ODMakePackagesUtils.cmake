#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:	CMake script to build a release
# Author:	Nageswara
# Date:		August 2012		
#RCS:		$Id$

macro ( CREATE_PACKAGE PACKAGE_NAME )
    if( ${PACKAGE_NAME} STREQUAL "base" )
	if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/qt_menu.nib
			     ${COPYTODATADIR}/qt_menu.nib )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/od.icns
			     ${COPYTODATADIR}/. )
	elseif( NOT WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E create_symlink
			     ${CMAKE_BUILD_TYPE} lib
			     WORKING_DIRECTORY ${COPYTOLIBDIR}/..)
	endif()

	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb )

	if( NOT MATLAB_DIR STREQUAL "" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/MATLAB
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/MATLAB )
	endif()

	COPY_THIRDPARTYLIBS()
	set( LIBLIST ${LIBLIST};${PLUGINS} )
    endif()


    if( ${PACKAGE_NAME} STREQUAL "dgbpro" )
	COPY_THIRDPARTYLIBS()
    endif()

    #TODO Need to check whether we need to use this macro on MAC.
    if ( APPLE AND SYSTEMLIBS ) #TODO Need to check whether we  need to use this macro on MAC.
	    COPY_MAC_SYSTEMLIBS()
	    unset( SYSTEMLIBS )
    endif()

    foreach( LIB ${LIBLIST} )
	if( WIN32 )
	    set( LIBNM "${LIB}.dll" )
	    #Stripping not required on windows
	elseif( APPLE )
	    set( LIBNM "lib${LIB}.dylib" )
	    execute_process( COMMAND ${SOURCE_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${LIBNM} )
	    #Not using breakpad on MAC
	else()
	    set( LIBNM "lib${LIB}.so")
	    execute_process( COMMAND strip ${COPYFROMLIBDIR}/${LIBNM} )
	endif()
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMLIBDIR}/${LIBNM}
			 ${COPYTOLIBDIR}/${LIBNM} )
#copying breakpad symbols
	if ( OD_ENABLE_BREAKPAD )
	    if( WIN32 )
		#TODO
	    elseif( APPLE )
		#TODO
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/symbols/${LIBNM}
				 ${COPYTOLIBDIR}/symbols/${LIBNM} )
	    endif()
	endif()

	file( GLOB ALOFILES ${COPYFROMDATADIR}/plugins/${OD_PLFSUBDIR}/*.${LIB}.alo )
	foreach( ALOFILE ${ALOFILES} )
	    get_filename_component( ALOFILENAME ${ALOFILE} NAME )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ALOFILE}
				     ${COPYTODATADIR}/plugins/${OD_PLFSUBDIR}/${ALOFILENAME} )
	endforeach()
    endforeach()

    foreach( PYDIR ${PYTHONDIR} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${COPYFROMDATADIR}/bin/python/${PYDIR}
			 ${COPYTODATADIR}/bin/python/${PYDIR} )
    endforeach()
    foreach( PYFILE ${PYTHONFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/bin/python/${PYFILE}
			 ${COPYTODATADIR}/bin/python/${PYFILE} )
    endforeach()
    unset( PYTHONDIR )
    unset( PYTHONFILES )


    if( ${PACKAGE_NAME} STREQUAL "dgbbase" )
#Install lm 
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${COPYFROMDATADIR}/${SPECFILE}
			      ${COPYTODATADIR}/. )
	endforeach()

	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	if ( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/odExternal
			     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/odExternal )
	elseif( NOT APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/libexec
			     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/libexec )
	endif()
    endif()

    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "base" )
	    set( EXECLIST "${EXECLIST};${WINEXECLIST}" )
	endif()
    endif()

    foreach( EXE ${EXECLIST} )
	if( WIN32 )
	    #Stripping not required on windows
	elseif( APPLE )
	    #Not using breakpad on MAC , Stripping not required
	else()
	    execute_process( COMMAND strip ${COPYFROMLIBDIR}/${EXE} )
	endif()

	if( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/${EXE}.exe
			     ${COPYTOLIBDIR}/${EXE}.exe )
	elseif( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMEXEDIR}/${EXE}
			     ${COPYTOEXEDIR}/${EXE} )

	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/${EXE}
			     ${COPYTOLIBDIR}/${EXE} )
	endif()
#copying breakpad symbols
	if ( OD_ENABLE_BREAKPAD )
	    if ( WIN32 )
		#TODO
	    elseif( APPLE )
		#TODO
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/symbols/${EXE}
				 ${COPYTOLIBDIR}/symbols/${EXE} )
	    endif()
	endif()
    endforeach()

    if( ${PACKAGE_NAME} STREQUAL "base" )
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${COPYFROMDATADIR}/${SPECFILE}
			      ${COPYTODATADIR}/. )
	endforeach()
	foreach( FILES ${ODSCRIPTS} )
	     file( GLOB SCRIPTS ${COPYFROMDATADIR}/bin/${FILES} )
	     foreach( SCRIPT ${SCRIPTS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					 ${COPYTODATADIR}/bin/. )
	     endforeach()
	endforeach()

	if( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${COPYFROMDATADIR}/rsm
				     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/rsm )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/od_main_debug.bat
			     ${COPYTOLIBDIR}/od_main_debug.bat )
	endif()
    endif()
    foreach( EXTERNALLIB ${EXTERNALLIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMLIBDIR}/${EXTERNALLIB}
			 ${COPYTOLIBDIR}/${EXTERNALLIB} )
    endforeach()
    set( EXTERNALLIBS "")

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_PACKAGE )


macro( COPY_THIRDPARTYLIBS )
    message( STATUS "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    list( APPEND SYSLIBS ${SYSTEMLIBS} )
    list( APPEND SSLLIBS ${OPENSSLLIBS} )
    foreach( LIB ${OD_THIRD_PARTY_FILES} )
	string( FIND ${LIB} "Qt" ISQTLIB )
	if (  APPLE AND NOT ${ISQTLIB} EQUAL -1 )
	    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/${LIB}.framework
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${LIB}
			     ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	else()
	    if ( WIN32 )
		get_filename_component( PDBFILE ${LIB} LAST_EXT )
		if( ${PDBFILE} STREQUAL ".pdb" )
		    continue()
		endif()
	    else()
		if ( APPLE )
		    execute_process( COMMAND ${SOURCE_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${LIB} )
		endif()
		if ( NOT APPLE )
		    list( FIND SYSLIBS "${LIB}" ITEMIDX )
		    if ( NOT ${ITEMIDX} EQUAL -1 )
			if( NOT EXISTS ${COPYTOLIBDIR}/systemlibs )
			    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/systemlibs )
			endif()

			execute_process( COMMAND ${CMAKE_COMMAND} -E copy
					 ${COPYFROMLIBDIR}/${LIB} ${COPYTOLIBDIR}/systemlibs/${LIB} )
			continue()
		    endif()
		endif()

		list( FIND SSLLIBS "${LIB}" ITEMIDX )
		if ( NOT ${ITEMIDX} EQUAL -1 )
		    if ( APPLE )
			if( NOT EXISTS ${COPYTODATADIR}/OpenSSL )
			    file( MAKE_DIRECTORY ${COPYTODATADIR}/OpenSSL )
			endif()

			execute_process( COMMAND ${CMAKE_COMMAND} -E copy
					 ${COPYFROMLIBDIR}/${LIB} ${COPYTODATADIR}/OpenSSL/${LIB} )
		    else()
			if( NOT EXISTS ${COPYTOLIBDIR}/OpenSSL )
			    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/OpenSSL )
			endif()

			execute_process( COMMAND ${CMAKE_COMMAND} -E copy
					 ${COPYFROMLIBDIR}/${LIB} ${COPYTOLIBDIR}/OpenSSL/${LIB} )
		    endif()
		    continue()
		endif()
	    endif()

	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${LIB} ${COPYTOLIBDIR} )
	endif()

    endforeach()

    foreach( ODPLUGIN ${OD_QTPLUGINS} )
	if ( "${ODPLUGIN}" STREQUAL "resources" )
	    if ( APPLE )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/../PlugIns/resources
				 ${COPYTOLIBDIR}/../PlugIns/resources )
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/../resources
				 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/resources )
	    endif()
	else()
	    if ( APPLE )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/../PlugIns/${ODPLUGIN}
				 ${COPYTOLIBDIR}/../PlugIns/${ODPLUGIN} )
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/../plugins/${ODPLUGIN}
				 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/plugins/${ODPLUGIN} )
	    endif()
	endif()
    endforeach()

    foreach( TRANSLATION_FILE ${OD_QT_TRANSLATION_FILES} )
	get_filename_component( QTWEB_LOCALS_FILE ${TRANSLATION_FILE} EXT )
	if( ${QTWEB_LOCALS_FILE} STREQUAL ".pak" )
	    if ( APPLE )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				 ${COPYFROMLIBDIR}/../PlugIns/translations/qtwebengine_locales/${TRANSLATION_FILE}
				 ${COPYTOLIBDIR}/../PlugIns/translations/qtwebengine_locales/${TRANSLATION_FILE} )
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				 ${COPYFROMLIBDIR}/../translations/qtwebengine_locales/${TRANSLATION_FILE}
				 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/translations/qtwebengine_locales/${TRANSLATION_FILE} )
	    endif()
	else()
	    if ( APPLE )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				 ${COPYFROMLIBDIR}/../PlugIns/translations/${TRANSLATION_FILE}
				 ${COPYTOLIBDIR}/../PlugIns/translations/${TRANSLATION_FILE} )
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				 ${COPYFROMLIBDIR}/../translations/${TRANSLATION_FILE}
				 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/translations/${TRANSLATION_FILE} )
	    endif()
	endif()
    endforeach()

endmacro( COPY_THIRDPARTYLIBS )

macro( COPY_MAC_SYSTEMLIBS )
    message( STATUS "Copying ${OD_PLFSUBDIR} system libraries" )
    if( APPLE )
	foreach( SYSLIB ${SYSTEMLIBS} )
	    execute_process( COMMAND ${SOURCE_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${SYSLIB} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${SYSLIB} ${COPYTOLIBDIR} )
	endforeach()
    endif()
endmacro( COPY_MAC_SYSTEMLIBS )


macro( CREATE_BASEPACKAGES PACKAGE_NAME )
    set( RELFILENAM RELEASE )
    string( FIND ${PACKAGE_NAME} "dgb" STATUS )
    if( ${STATUS} EQUAL "0" )
	set( ODDGBSTR "dgb" )
	set( RELFILENAM ${RELFILENAM}.${ODDGBSTR}.txt )
        file( GLOB QMFILES ${COPYFROMDATADIR}/data/localizations/*.qm )
	foreach( QMFILE ${QMFILES} )
	    get_filename_component( QMFILENM ${QMFILE} NAME )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/data/localizations/${QMFILENM}
			     ${COPYTODATADIR}/data/localizations/${QMFILENM} )
	 endforeach()
    else()
	set( ODDGBSTR "od" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/relinfo/README.txt
			 ${COPYTODATADIR}/relinfo/README.txt )
	set( RELFILENAM ${RELFILENAM}.txt )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${COPYFROMDATADIR}/relinfo/RELEASEINFO.txt
				${COPYTODATADIR}/doc/ReleaseInfo/RELEASEINFO.txt )
    endif()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			    ${COPYFROMDATADIR}/relinfo/${RELFILENAM}
			    ${COPYTODATADIR}/doc/ReleaseInfo/${RELFILENAM} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			    ${COPYFROMDATADIR}/doc/Videos.${ODDGBSTR}
			    ${COPYTODATADIR}/doc/Videos.${ODDGBSTR} )

    foreach( LIBS ${LIBLIST} )
	file( GLOB DATAFILES ${COPYFROMDATADIR}/data/${LIBS} )
	foreach( DATA ${DATAFILES} )
	    get_filename_component( DATALIBNM ${DATA} NAME )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/data/${DATALIBNM}
			     ${COPYTODATADIR}/data/${DATALIBNM})
	 endforeach()
    endforeach()

    foreach( DATADIR ${DATADIRLIST} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${COPYFROMDATADIR}/data/${DATADIR}
			 ${COPYTODATADIR}/data/${DATADIR} )
    endforeach()

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_BASEPACKAGES )


macro( INIT_DESTINATIONDIR  PACKAGE_NAME )
    set ( PACKAGE_FILENAME ${PACKAGE_NAME} )
    set( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    set( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )
    if( ${PACKAGE_NAME} STREQUAL "basedata" )
	set( VER_FILENAME "basedata" )
	set( PACKAGE_FILENAME "basedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "basedata_mac.zip" )
	    set( VER_FILENAME "basedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
	set( VER_FILENAME "dgbbasedata" )
	set( PACKAGE_FILENAME "dgbbasedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbbasedata_mac.zip" )
	    set( VER_FILENAME "dgbbasedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "doc" )
	set( VER_FILENAME "doc" )
	set( PACKAGE_FILENAME "doc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "doc_mac.zip" )
	    set( VER_FILENAME "doc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	set( VER_FILENAME "dgbdoc" )
	set( PACKAGE_FILENAME "dgbdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbdoc_mac.zip" )
	    set( VER_FILENAME "dgbdoc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "classdoc" )
	set( VER_FILENAME "classdoc" )
	set( PACKAGE_FILENAME "classdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "classdoc_mac.zip" )
	    set( VER_FILENAME "classdoc_mac" )
	endif()
    endif()

    if( NOT EXISTS ${PACKAGE_DIR} )
	    file( MAKE_DIRECTORY ${PACKAGE_DIR} )
    endif()

    if( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	file( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
    endif()
    set( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )
    set( FULLVER_NAME "${OpendTect_FULL_VERSION}" )
    if( APPLE )
	set( REL_DIR "OpendTect\ ${REL_DIR}.app/Contents" )
    endif()

    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( EXISTS ${DESTINATION_DIR} )
	file( REMOVE_RECURSE ${DESTINATION_DIR} )
    endif()

    if ( NOT APPLE )
	if( NOT ${PACKAGE_NAME} STREQUAL "basedata" AND
	    NOT ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
	    file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	    set( COPYFROMLIBDIR ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release )
	    set( COPYTOLIBDIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	endif()

	set( COPYFROMDATADIR ${CMAKE_INSTALL_PREFIX} )
	set( COPYTODATADIR ${DESTINATION_DIR} )
    else()
	file( MAKE_DIRECTORY ${DESTINATION_DIR}
			     ${DESTINATION_DIR}/MacOS ${DESTINATION_DIR}/Frameworks )
	set( COPYFROMLIBDIR ${CMAKE_INSTALL_PREFIX}/Contents/Frameworks )
	set( COPYTOLIBDIR ${DESTINATION_DIR}/Frameworks )
	set( COPYFROMEXEDIR ${CMAKE_INSTALL_PREFIX}/Contents/MacOS )
	set( COPYTOEXEDIR ${DESTINATION_DIR}/MacOS )
	set( COPYFROMDATADIR ${CMAKE_INSTALL_PREFIX}/Contents/Resources )
	set( COPYTODATADIR ${DESTINATION_DIR}/Resources )
    endif()

    file( WRITE ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
    file( APPEND ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )
    if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/Contents/Info.plist
			     ${DESTINATION_DIR}/. )
    endif()

    message( STATUS "Preparing package ${VER_FILENAME}.zip ......" )
endmacro( INIT_DESTINATIONDIR )


macro( CREATE_DEVELPACKAGES )
    file( MAKE_DIRECTORY ${COPYFROMDATADIR}/doc
			 ${COPYTODATADIR}/doc/Programmer)
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     ${SOURCE_DIR}/CMakeLists.txt ${COPYTODATADIR} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/doc/Programmer/batchprogexample
		     ${COPYTODATADIR}/doc/Programmer/batchprogexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/doc/Programmer/pluginexample
		     ${COPYTODATADIR}/doc/Programmer/pluginexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/dtect
		     ${COPYTODATADIR}/dtect )

    foreach( SPECFILE ${SPECFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/doc/Programmer/${SPECFILE}
			 ${COPYTODATADIR}/doc/Programmer )
    endforeach()

    file( GLOB HTMLFILES ${BINARY_DIR}/doc/Programmer/*.html )
    foreach( HTMLFILE ${HTMLFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${HTMLFILE} ${COPYTODATADIR}/doc/Programmer )
    endforeach()
    file( GLOB PNGFILES ${SOURCE_DIR}/doc/Programmer/*.png )
    foreach( PNGFILE ${PNGFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${PNGFILE} ${COPYTODATADIR}/doc/Programmer )
    endforeach()

    foreach( DIR CMakeModules include src plugins spec )
	message( STATUS "Copying ${DIR} files" )
	if( "${DIR}" STREQUAL "plugins" )
	    foreach( ODPLUGIN ${ODPLUGINS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMDATADIR}/plugins/${ODPLUGIN}
				 ${COPYTODATADIR}/plugins/${ODPLUGIN} )
	    endforeach()
	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/${DIR}
			     ${COPYTODATADIR}/${DIR} )
	endif()
    endforeach()

    if ( APPLE )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/bin/od_cr_dev_env.csh
			 ${COPYTODATADIR}/od_cr_dev_env.csh )
    endif()

    if( WIN32 )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${SOURCE_DIR}/bin/od_cr_dev_env.bat
			 ${DESTINATION_DIR}/bin )

	set( DEVELLIBS ${ODLIBLIST} ${ODPLUGINS} ${SPECSOURCES} )
	#Copying dll, pdb and lib files.
	foreach( DLIB ${DEVELLIBS} )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${DLIB}.* )
	    string( TOLOWER ${DLIB} PDBLIB )
	    file( GLOB PDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${PDBLIB}.* )
	    set( FILES ${FILES} ${PDBFILES} )
	    foreach( FIL ${FILES} )
		if ( EXISTS "${FIL}" ) #Some modules have no library
		    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			    "${FIL}" ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
		endif()
	    endforeach()

	    if ( EXISTS "${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${DLIB}.lib" )
		#Some modules have no library
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		    ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${DLIB}.lib
		    ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	    endif()
	endforeach()
	#Copying executables and pdb files
	foreach( EXELIB ${EXECLIST} )
	    file( GLOB EXEFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXELIB}.* )
	    string( TOLOWER ${EXELIB} EXEPDBLIB )
	    file( GLOB EXEPDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXEPDBLIB}.* )
	    set( EXEILES ${EXEFILES} ${EXEPDBFILES} )
	    foreach( ELIB ${EXEILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${ELIB} ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	    endforeach()
	endforeach()

	#Copying third party debug libraries
	foreach( THIRDPARTY_DEBUGLIB ${OD_THIRD_PARTY_FILES_DEBUG} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${THIRDPARTY_DEBUGLIB}
				${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	endforeach()
    endif()

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_DEVELPACKAGES )

macro( CREATE_DOCPACKAGES PACKAGE_NAME )
    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "doc" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/od_userdoc
			     ${DESTINATION_DIR}/doc/od_userdoc )
	elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb_userdoc
			     ${DESTINATION_DIR}/doc/dgb_userdoc )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/doc/flexnet* )
	    foreach( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FIL} ${DESTINATION_DIR}/doc )
	    endforeach()
	endif()
    else()
	if( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated
				 ${DESTINATION_DIR}/doc/Programmer/Generated )
	    else()
		message( FATAL_ERROR "Class doc not installed correctly. ${PACKAGE_FILENAME} is not self contained." )
	    endif()
	endif()
    endif()
    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_DOCPACKAGES )

macro( ZIPPACKAGE PACKAGE_FILENAME REL_DIR PACKAGE_DIR )
    if( WIN32 )
	execute_process( COMMAND ${SOURCE_DIR}/bin/win64/zip -r -q
				 "${PACKAGE_FILENAME}" ${REL_DIR}
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    else()
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    endif()

    if( NOT ${STATUS} EQUAL "0" )
	message( FATAL_ERROR "Failed to create zip file ${PACKAGE_FILENAME}" )
   endif()
endmacro( ZIPPACKAGE )

#Genarate Symbols and then Strip the binaries
macro ( OD_GENERATE_BREAKPAD_SYMBOLS ALLLIBS EXECS)
    if ( NOT DEFINED BREAKPAD_DIR )
	message ( FATAL_ERROR "BREAKPAD_DIR not defined" )
    endif()
    if ( NOT EXISTS ${BREAKPAD_DIR} )
	message ( FATAL_ERROR "BREAKPAD_DIR: ${BREAKPAD_DIR} not found" )
    endif()

    set( SYMBOLDIRNM symbols_${OD_PLFSUBDIR}_${FULLVER_NAME} )
    if( NOT EXISTS ${PACKAGE_DIR}/${SYMBOLDIRNM} )
	file( MAKE_DIRECTORY ${PACKAGE_DIR}/symbols/${SYMBOLDIRNM} )
    endif()
    set( SYMBOLDIR ${PACKAGE_DIR}/symbols/${SYMBOLDIRNM} )

if( UNIX )
    set( LIBNAMES "" )
    foreach( LIB ${ALLLIBS} )
	set(SOLIB "lib${LIB}.so")
	set( LIBNAMES ${LIBNAMES} ${SOLIB} )
    endforeach()
    set( ALLLIBSBINS ${LIBNAMES} ${EXECS} )
    set( SYMGENCMD ${BREAKPAD_DIR}/bin/dump_syms )
elseif( WIN32 )
    set( LIBSBINS ${ALLLIBS} ${EXECS} )
    set( ALLLIBSBINS "" )
    foreach( LIBNM ${LIBSBINS} )
	if ( ${LIBNM} STREQUAL "lmutil" )
	    set( LIBNM "" )
	endif()
	set( ALLLIBSBINS ${ALLLIBSBINS} ${LIBNM} )
    endforeach()
    set( SYMGENCMD ${BREAKPAD_DIR}/bin/dump_syms.exe )
endif()

    foreach( FILENAME ${ALLLIBSBINS} )
	if ( UNIX )
	    execute_process( COMMAND "${SYMGENCMD}" ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
	elseif( WIN32 )
	    execute_process( COMMAND "${SYMGENCMD}" ${BINARY_DIR}/bin/${OD_PLFSUBDIR}/Release/${FILENAME}.pdb OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
	endif()

	file( STRINGS ${SYMBOLDIR}/${FILENAME}.sym STUFF LIMIT_COUNT 1 )
	string( REGEX REPLACE " " ";" NEWSTUFF ${STUFF} )
	set( WORDS ${NEWSTUFF} )
	list( GET WORDS 3 SYMADDRESS ) #assuming 4th word is symbol address
	if( UNIX )
	    file( MAKE_DIRECTORY ${SYMBOLDIR}/${FILENAME}/${SYMADDRESS} )
	elseif( WIN32 )
	    file( MAKE_DIRECTORY ${SYMBOLDIR}/${FILENAME}.pdb/${SYMADDRESS} )
	endif()

	if ( UNIX )
	    file( RENAME ${SYMBOLDIR}/${FILENAME}.sym ${SYMBOLDIR}/${FILENAME}/${SYMADDRESS}/${FILENAME}.sym )
	elseif( WIN32 )
	    file( RENAME ${SYMBOLDIR}/${FILENAME}.sym ${SYMBOLDIR}/${FILENAME}.pdb/${SYMADDRESS}/${FILENAME}.sym )
	endif()
	if( UNIX )
	    execute_process( COMMAND strip ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} )
	endif()
    endforeach()
endmacro( OD_GENERATE_BREAKPAD_SYMBOLS )

