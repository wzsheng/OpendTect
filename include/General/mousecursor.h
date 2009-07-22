#ifndef mousecursor_h
#define mousecursor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          March 2008
 RCS:           $Id: mousecursor.h,v 1.7 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "ptrman.h"

/*!Definition of a mouse cursor, can be either a predefined shape (from the
   enum, or a file. */

mClass MouseCursor
{
public:
    virtual		~MouseCursor()					{}
		    /*! This enum type defines the various cursors that can be
		        used.

			\value Arrow		standard arrow cursor
			\value UpArrow		upwards arrow
			\value Cross		crosshair
			\value Wait		hourglass/watch
			\value Ibeam		ibeam/text entry
			\value SizeVer		vertical resize
			\value SizeHor 		horizontal resize
			\value SizeFDiag	diagonal resize (\)
			\value SizeBDiag	diagonal resize (/)
			\value SizeAll		all directions resize
			\value Blank		blank/invisible cursor
			\value SplitV		vertical splitting
			\value SplitH		horizontal splitting
			\value PointingHand	a pointing hand
			\value Forbidden	a slashed circle
			\value WhatsThis	an arrow with a question mark
			\value Bitmap
			\value NotSet 

			Arrow is the default for widgets in a normal state.
		    */
    enum Shape		{ Arrow, UpArrow, Cross, Wait, Ibeam,
			  SizeVer, SizeHor, SizeBDiag, SizeFDiag, SizeAll,
			  Blank, SplitV, SplitH, PointingHand, Forbidden,
			  WhatsThis, Last = WhatsThis, Bitmap = 24, NotSet
			};

    			MouseCursor() : shape_(NotSet)			{}
			MouseCursor( Shape s ) : shape_(s)		{}
    bool		operator==(const MouseCursor&) const;
    bool		operator!=(const MouseCursor&) const;

    Shape		shape_;
    BufferString	filename_;
    int			hotx_;
    int			hoty_;
};


/*! \brief Sets another cursor for current application

    example:

    \code
	MouseCursorManager::setOverride( MouseCursor::Wait );
	calculateHugeMandelbrot();              // lunch time...
	MouseCursorManager::restoreOverride();
    \endcode    

    Application cursors are stored on an internal stack.
    setOverride() pushes the cursor onto the stack, and
    restoreOverride() pops the active cursor off the stack.
    Every setOverride() must eventually be followed by a
    corresponding restoreOverride(), otherwise the stack will
    never be emptied.  

    If replace is true, the new cursor will replace the last
    overridecw cursor (the stack keeps its depth). If replace is
    FALSE, the new cursor is pushed onto the top of the stack.
*/


mClass MouseCursorManager
{
public:

    virtual	~MouseCursorManager()					{}

    static void	setOverride(MouseCursor::Shape,bool replace=false);
    static void	setOverride(const MouseCursor&,bool replace=false);
    static void setOverride(const char* filenm,int hotx=-1, int hoty=-1,
	    	            bool replace=false);	
    static void	restoreOverride();


    static MouseCursorManager*		mgr();
    static void				setMgr(MouseCursorManager*);
    					//!<\note I will not manage manager

protected:
    virtual void setOverrideShape(MouseCursor::Shape,bool replace)	= 0;
    virtual void setOverrideCursor(const MouseCursor&,bool replace)	= 0;
    virtual void setOverrideFile(const char* filenm,
	    			 int hotx,int hoty, bool replace)	= 0;	
    virtual void restoreInternal()					= 0;


    static MouseCursorManager*		mgr_;
};


/*! Class to automaticly change cursor, and change it back automaticly when
    class is running out of scope.
*/

mClass MouseCursorChanger
{
public:
		MouseCursorChanger(const char* fnm, int hotx, int hoty);
		MouseCursorChanger(MouseCursor::Shape cs);
		~MouseCursorChanger();

    void	restore();

protected:
    bool	active_;
};


#endif
