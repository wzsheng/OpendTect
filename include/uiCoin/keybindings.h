#ifndef keybindings_h
#define keybindings_h

/*+
 ________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          Dec 2002
 RCS:           $Id: keybindings.h,v 1.6 2006-12-13 08:18:33 cvsnanne Exp $
________________________________________________________________________

*/

#include "bufstringset.h"
class SoMouseButtonEvent;
class SoEvent;

/*! \brief Class for setting keybindings.

Each binding is a BufferString. This string contains the several keys 
separated by a `.  e.g. zoom = "Left`Control"

*/


class KeyBindings
{
public:
    				KeyBindings(const char* nm=0)
				    : name(nm) {};

    BufferString		name;

    BufferString		zoom;
    BufferString		rotate;
    BufferString		pan;


    static const char*		sName;
    static const char*		sRotate;
    static const char*		sPan;
    static const char*		sZoom;

    static const char*		sControl;
    static const char*		sShift;
    static const char*		sRight;
    static const char*		sLeft;
    static const char*		sMiddle;
    static const char*		sNone;
};


class EventButton
{
public:
                                EventButton() {}

    BufferString                mousebut;
    BufferString                keybut;
};


class KeyBindMan
{
public:
                                KeyBindMan();
                                ~KeyBindMan();

    void                        setKeyBindings(const char*,bool saveinsett);
    void                        getAllKeyBindings(BufferStringSet&);
    const char*			getCurrentKeyBindings() const
				{ return curkeyb; }

    const SoEvent*		processSoEvent(const SoEvent* const,bool,bool);

protected:

    bool                        correctButtonsPushed(EventButton,const char*);
    void                        doZoom(SoMouseButtonEvent*);
    void                        doPan(SoMouseButtonEvent*);
    void                        doRotate(SoMouseButtonEvent*);

    ObjectSet<KeyBindings>      keyset;
    BufferString		curkeyb;

    EventButton                 zoom;
    EventButton                 pan;
    EventButton                 rotate;

    bool                        dozoom;
    bool                        shiftpress;
    bool                        ctrlpress;
    bool                        useownevent;
};

#endif
