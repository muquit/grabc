# grabc
A command line tool for X Window System to identify the color string in hex by clicking on a pixel.

  When this program is run, the mouse pointer is grabbed and changed to
  a cross hair and when the mouse is clicked, the color of the clicked
  pixel is written to stdout in hex prefixed with #

  This program can be useful when you see a color and want to use the
  color in xterm or your window manager's border but no clue what the
  name of the color is. It's silly to use a image processing software
  to find it out.

# Synopsis

```
grabc v1.0.2
A program to identify a pixel color of an X Window
by muquit@muquit.com https://www.muquit.com/

Usage: grabc [options]
Where the options are:
 -v         - show version info
 -h         - show this usage
 -hex       - print pixel value as Hex on stdout
 -rgb       - print pixel value as RGB on stderr
 -W         - print the Window id at mouse click
 -w id      - window id in hex, use before -l
 -l +x+y    - pixel co-ordinate. requires window id
 -d         - show debug messages
 -a         - Print all 16 bits of color. Default is high order 8 bits
Example:
* Print pixel color in hex on stdout:
   $ grabc
* Show usage:
   $ grabc -h
* Print Window Id (Note the upper case W):
   $ grabc -W
* Print pixel color of Window with id 0x13234 at location 10,20
   $ grabc -w 0x13234 -l +10+20
```

# How to compile
Older version of this program is available on Ubuntu. However, if you need to get the latest version, you have to compile it yourself.

* You will need ```libx11-dev``` package if you are on Ubuntu. 
```
    sudo apt-get -y install libx11-dev
```


* To compile, at the shell prompt, type:
```
    make
```    

# Know Issues

* If color grabbed from root window, it might always show #000000


# ChangeLog

# v1.0.2
 * Was not working properly on Ubuntu Terminal. It was using default Colormap. Do not use default colormap, rather get it from window attributes. 
 * If could not get XImage from target window, it is probably root window,
    so try to grab it from there.
* Added some options
* Color can be grabbed from a specific location

* Change Copyright to MIT from GNU GPL

(Apr-10-2018)

# v1.0.1
* first cut

(march-16-1997)


# License

MIT
