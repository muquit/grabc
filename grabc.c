/*  A program to pick a color by clicking the mouse.
 *
 *  RCS:
 *      $Revision$
 *      $Date$
 *
 *  Description:
 *
 *  When this program is run, the mouse pointer is grabbed and changed to
 *  a cross hair and when the mouse is clicked, the color of the clicked
 *  pixel is written to stdout in hex prefixed with #
 *
 *  This program can be useful when you see a color and want to use the
 *  color in xterm or your window manager's border but no clue what the 
 *  name of the color is. It's silly to use a image processing software
 *  to find it out.
 *
 * Example: 
 *   xterm -bg `grabc` -fg `grabc` (silly but esoteric!) 
 *
 *  Development History:
 *      who                  when               why
 *      ma_muquit@fccc.edu   march-16-1997     first cut
 *      muquit@muquit.com    Apr-10-2018       Do not use default colormap,
 *      rather get it from window attributes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifndef True
#define True    1
#endif

#ifndef False
#define False   0
#endif

#define VERSION_S "1.0.2"

static int g_debug = False;
static int g_print_in_hex = True;
static int g_print_in_rgb = False;
static int g_print_all_16_bits = False;
static Window g_window_id = (Window) NULL;
static int g_loc_specified = False;
static int g_x = 1;
static int g_y = 1;
static unsigned int g_width = 0;
static unsigned int g_height = 0;
static Cursor g_cross_cursor=(Cursor) NULL;

/* private function prototypes */
static Window select_window (Display *,int *x,int *y);

static Window findSubWindow(Display *display,Window top_winodw,
    Window window_to_check,int *x,int *y);

static Window get_window_color(Display *display,XColor *color);
static int MXError(Display *display,XErrorEvent *error);

static void show_usage(void)
{
    char
        **p;

    static char *options[]=
    {
" -v      - show version info",
" -h      - show this usage",
" -hex    - print pixel value as Hex on stdout",
" -rgb    - print pixel value as RGB on stderr",
" -W      - print the Window id at mouse click",
" -w id   - window id in hex, use -l +x+y",
" -l +x+y - pixel co-ordinate. requires window id",
" -d      - show debug messages",
" -a      - Print all 16 bits RGB components of color",
"           Default is high order 8 bits of components",
"Example:",
"* Print pixel color in hex on stdout:",
"   $ grabc",
"* Show usage:",
"   $ grabc -h",
"* Print Window Id (Note the upper case W):",
"   $ grabc -W",    
"* Print pixel color of Window iwith id 0x13234 at location 10,20",
"   $ grabc -w 0x13234 -l +10+20",
(char *) NULL

    };

    (void) printf("\n");
    (void) printf("grabc v%s\n",VERSION_S);
    (void) printf("A program to identify a pixel color of an X Window\n");
    (void) printf("by muquit@muquit.com https://www.muquit.com/\n\n");
    (void) printf("Usage: grabc [options]\n");
    (void) printf("Where the options are:\n");
    for (p=options; *p != NULL; p++)
    {
        (void) fprintf(stdout,"%s\n",*p);
        (void) fflush(stdout);
    }
}


static void log_debug(const char *fmt,...)
{
    va_list
        args;
    if (!g_debug)
    {
        return;
    }
    va_start(args, fmt);
    (void) fprintf(stderr,"[Debug]: ");
    vfprintf(stderr,fmt,args);
    (void) fprintf(stderr,"\n");
    va_end(args);
}

static Cursor get_cross_cursor(Display *display)
{
    if (g_cross_cursor == (Cursor) NULL)
    {
        g_cross_cursor=XCreateFontCursor(display,XC_tcross);
        if (g_cross_cursor == (Cursor) NULL)
        {
            (void) fprintf (stderr,"ERROR: Failed to create Cross Cursor!\n");
            exit(1);
        }
    }
    return g_cross_cursor;
}
static Window grab_mouse(Display *display,Window root_window)
{
    int
        status;

    Window
        subwindow;

    XEvent
        event;

    Cursor
        target_cursor;


    if (g_window_id != (Window) NULL)
    {
        return g_window_id;
    }

    target_cursor = get_cross_cursor(display);
    status=XGrabPointer(display,root_window,False,
    (unsigned int) ButtonPressMask,GrabModeSync,
    GrabModeAsync,root_window,target_cursor,CurrentTime);
    if (status == GrabSuccess)
    {
        XAllowEvents(display,SyncPointer,CurrentTime);
        XWindowEvent(display,root_window,ButtonPressMask,&event);
        subwindow = event.xbutton.subwindow;
    }
    else
    {
        return root_window;
    }

    return subwindow;
}
static void upgrab_mouse(Display *display)
{
    if (g_window_id != (Window) NULL)
    {
        XUngrabPointer(display,CurrentTime);
    }
}





/*
** function to select a window
** output parameters: x,y (coordinate of the point of click)
** reutrns Window
** exits if mouse can not be grabbed
*/
static Window select_window(Display *display,int *x,int *y)
{
    Cursor
        target_cursor;

    int
        status;

    Window
        target_window,
        root_window;

    XEvent
        event;

    /*
    ** If window id and location is specified return the window id as 
    ** target window. Also initilaize x, y those specified with -l
    */
    if ((g_window_id != (Window) NULL) && g_loc_specified)
    {
        log_debug("Returning passing window: %lx",g_window_id);
        (*x) = g_x;
        (*y) = g_y;
        return g_window_id;
    }
    target_window=(Window) NULL;
    target_cursor = get_cross_cursor(display);
    root_window=XRootWindow(display,XDefaultScreen(display));
//    log_debug("Root Window ID: 0x%08lx",root_window);

        status=XGrabPointer(display,root_window,False,
            (unsigned int) ButtonPressMask,GrabModeSync,
            GrabModeAsync,root_window,target_cursor,CurrentTime);
        if (status == GrabSuccess)
        {
            XAllowEvents(display,SyncPointer,CurrentTime);
            XWindowEvent(display,root_window,ButtonPressMask,&event);
            Window subwindow = event.xbutton.subwindow;

            if (event.type == ButtonPress)
            {
                target_window=findSubWindow(display,root_window,
                    subwindow,
                    &event.xbutton.x,
                    &event.xbutton.y );

                if (target_window == (Window) NULL)
                {
                    (void) fprintf (stderr,
                        "ERROR: Failed to get target window, getting root window!\n");
                    target_window=root_window;
                }
                if (!g_loc_specified)
                {
                    XUngrabPointer(display,CurrentTime);
                }
            }

        }
        else
        {
            (void) fprintf (stderr,"ERROR: Failed to grab mouse pointer!\n");
            exit(1);
        }

        /* free things we do not need, always a good practice */
        (*x)=event.xbutton.x;
        (*y)=event.xbutton.y;

    return (target_window);
}

/* find a window */
static Window findSubWindow(Display *display,Window top_window,
    Window window_to_check,int *x,int *y)
{
    int 
        newx,
        newy;

    Window
        window;

    if (top_window == (Window) NULL)
        return ((Window) NULL);

    if (window_to_check == (Window) NULL)
        return ((Window) NULL);

    /* initialize automatics */
    window=window_to_check;

    while ((XTranslateCoordinates(display,top_window,window_to_check,
        *x,*y,&newx,&newy,&window) != 0) &&
           (window != (Window) NULL))
    {
        if (window != (Window) NULL)
        {
            top_window=window_to_check;
            window_to_check=window;
            (*x)=newx;
            (*y)=newy;
        }
    }

    if (window == (Window) NULL)
        window=window_to_check;


    (*x)=newx;
    (*y)=newy;

    return (window);
}

/*
 * get the color of the pixel of the point of mouse click
 * output paramter: XColor *color
 *
 * returns 
 * target Window on success
 * NULL on failure
 *
 */

static Window get_window_color(Display *display,XColor *color)
{
    Window
        root_window,
        target_window;

    XImage
        *ximage;

    int
        x,
        y;

    Status
        status;

    root_window=XRootWindow(display,XDefaultScreen(display));
    target_window=select_window(display,&x,&y);

    log_debug("  Root Window Id: 0x%08lx",root_window);
    log_debug("Target Window Id: 0x%08lx  X,Y: +%d+%d",target_window,x,y);
    
    if (target_window == (Window) NULL)
        return (Window) NULL;

    ximage=XGetImage(display,target_window,x,y,1,1,AllPlanes,ZPixmap);
    if (ximage == (XImage *) NULL)
    {
        /* Try root window */
        log_debug("Could not get XImage from Window: 0x%08lx",target_window);
        log_debug("Trying to get XImage from root window: 0x%08lx",root_window);
        ximage=XGetImage(display,root_window,x,y,1,1,AllPlanes,ZPixmap);
        if (ximage == (XImage *) NULL)
        {
            log_debug("Could not get XImage from target or root window");
            return (Window) NULL;
        }
        else
        {
            log_debug("OK successfully got XImage from root window");
            target_window = root_window;
        }

    }

    color->pixel=XGetPixel(ximage,0,0);
    XDestroyImage(ximage);

    return (target_window);
}

/* forgiving X error handler */

static int MXError (Display *display, XErrorEvent *error)
{
    int
        xerrcode;
 
    xerrcode = error->error_code;
 
    if (xerrcode == BadAlloc || 
        (xerrcode == BadAccess && error->request_code==88)) 
    {
        return (False);
    }
    else
    {
        switch (error->request_code)
        {
            case X_GetGeometry:
            {
                if (error->error_code == BadDrawable)
                    return (False);
                break;
            }

            case X_GetWindowAttributes:
            case X_QueryTree:
            {
                if (error->error_code == BadWindow)
                    return (False);
                break;
            }

            case X_QueryColors:
            {
                if (error->error_code == BadValue)
                    return(False);
                break;
            }
        }
    }
    return (True);
}

int main(int argc,char **argv)
{
    Display
        *display;

    int
        x,
        y,
        status;

    XColor  
        color;

    int
        rc,
        i,
        r,
        g,
        b;

    Window
        window_id,
        target_window;

    XWindowAttributes
        window_attributes;

    char
        *option;

    for (i=1; i < argc; i++)
    {
        option = argv[i];
        switch(*(option+1))
        {
            case 'a':
            {
                g_print_all_16_bits = True;
                break;
            }
            case 'd':
            {
                g_debug = True;
                break;
            }

            case 'h':
            {
                if (strncmp("hex",option+1,3) == 0)
                {
                    g_print_in_hex = True;
                }
                else
                {
                    show_usage();
                    return(1);
                }
                break;
            }

            case 'r':
            {
                if (strncmp("rgb",option+1,3) == 0)
                {
                    g_print_in_rgb = True;
                }
                break;
            }

            case 'w':
            {
                if (*option == '-')
                {
                    i++;
                    if (i == argc)
                    {
                        (void) fprintf(stderr,"ERROR: Missing Window id\n");
                        return(1);
                    }
                }
                g_window_id = (Window) strtol(argv[i],NULL, 16);
                break;
            }
            case 'W':
            {
                display=XOpenDisplay((char *) NULL);
                if (display == NULL)
                {
                    (void) fprintf(stderr,"ERROR: Could not open Display\n");
                    return(1);
                }
                Window window = select_window(display, &x, &y);
                if (window != (Window) NULL)
                {
                    log_debug("Window ID: 0x%08lx",window);
                    (void) fprintf(stdout,"0x%lx\n",window);
                }
                return(1);
                break;
            }

            case 'l':
            {
                if (*option == '-')
                {
                    i++;
                    if (i == argc)
                    {
                        (void) fprintf(stderr,"ERROR: Missing location +x+y\n");
                        return(1);
                    }
                }
                rc = XParseGeometry(argv[i], &g_x,&g_y,&g_width,&g_height);
                if (rc == 0)
                {
                    (void) fprintf(stderr,"ERROR: Could not parse location: %s\n",argv[i]);
                    (void) fprintf(stderr,"Example: -l +10+20\n");
                    return(1);
                }
                g_loc_specified = True;

                break;
            }

            case 'v':
            {
                (void) fprintf(stderr,"grabc v%s\n",VERSION_S);
                return(1);
                break;
            }

            default:
            {
                break;
            }
        }
    }

    if (g_loc_specified && (g_window_id == (Window) NULL))
    {
        (void) fprintf(stderr,"ERROR: Please specify window id with -w in hex to use this option\n");
        (void) fprintf(stderr,"Use -W option to find the Window Id\n");
        return(1);
    }

    display=XOpenDisplay((char *) NULL);
    XSetErrorHandler(MXError);

    if (display == (Display *) NULL)
    {
        (void) fprintf (stderr,"ERROR: Failed to open DISPLAY!\n");
        exit(1);
    }

    target_window = get_window_color(display,&color);
    if (target_window != (Window) NULL)
    {
        status = XGetWindowAttributes(display, target_window,
                &window_attributes);
        if (status == False || window_attributes.map_state != IsViewable)
        {
            (void) fprintf(stderr,"ERROR: Could not get Window Attributes\n");
            return(1);
        }
        XQueryColor(display, window_attributes.colormap, &color);
        if (g_print_all_16_bits)
        {
            (void) fprintf(stdout,"#%04x%04x%04x\n",
                (unsigned int)color.red,
                (unsigned int) color.green,
                (unsigned int) color.blue);
            (void) fflush(stdout);
            if (g_print_in_rgb)
            {
                (void) fprintf(stderr,"%d,%d,%d\n",
                    (unsigned int)color.red,
                    (unsigned int) color.green,
                    (unsigned int) color.blue);
            }

        }
        else
        {
            r=(color.red >> 8);
            g=(color.green >> 8);
            b=(color.blue >> 8);
            log_debug("Color: #%02x%02x%02x",r,g,b);
            (void) fprintf (stdout,"#%02x%02x%02x\n",r,g,b);
            (void) fflush(stdout);
            /*
            ** write the values in decimal on stderr
            */
            if (g_print_in_rgb)
            {
                (void) fprintf(stderr,"%d,%d,%d\n",r,g,b);
            }
        }
    }
    else
    {
        (void) fprintf (stderr,"ERROR: Failed to grab color!\n");
    }
    return (0);
}
