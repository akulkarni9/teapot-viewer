## Teapot-Viewer 0.9a ##

The Teapot-Viewer is a fast and extendable 3D-Model Viewer for Linux and Windows.

The GUI base on wxWidgets.
The core/scenegraph is written in C++ using common design pattens (graph,visitor).
STL and boost are the only required dependencies. For rendering there s an graphics API independent renderer interface. Currently there is a basic Direct3D 9 and a OpenGL 1.4 renderer implementation.

  * The Direct3D 9 renderer runs a simple shader, supporting bumpmapping, reflectiontexture and alphatextures.
  * The OpenGL renderer uses the fixed pipeline, the textures are loaded with devIL (http://openil.sourceforge.net).

The SceneIO Module, provides an internal OBJ laoder/writer. Additionally there is a simple plugin interface. It is possible to load Models from zip files. Reading zip files base on the Minizip library (http://www.winimage.com/zLibDll/minizip.html).

Currently there are several loader plugins, all base on 3rd-party libraries:

  * FCollada (.dae) (http://www.feelingsoftware.com/)
  * lib3DS (.3ds) (http://code.google.com/p/lib3ds/)
  * OpenCascade (.stp, .iges, .brep) (http://www.opencascade.org/)
  * SketchUpSDK (.skp) (http://code.google.com/apis/sketchup/)
<a href='Hidden comment: 
* Coin3d (.iv, .wrl) (http://www.coin3d.org)
* libG3D (.q3d and more) (http://automagically.de/g3dviewer/)
'></a>

<a href='Hidden comment: 
=== Donation ===

If you feel this utility is useful to you and want to support it and it"s future development please consider donating...

<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=taibal%40gmx%2ede&lc=US&item_name=teapot%2dviewer&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donateCC_LG%2egif%3aNonHostedGuest" rel="nofollow"><img src="https://www.paypal.com/en_US/i/btn/btn_donateCC_LG.gif" />

Unknown end tag for &lt;/a&gt;


'></a>
## Screenshots ##

<img src='http://teapot-viewer.googlecode.com/svn/trunk/doc/screenshots/teapot.obj.png' width='320'>
<img src='http://teapot-viewer.googlecode.com/svn/trunk/doc/screenshots/F40.dae.png' width='320'>
<img src='http://teapot-viewer.googlecode.com/svn/trunk/doc/screenshots/tiefite.3ds.png' width='320'>
<img src='http://teapot-viewer.googlecode.com/svn/trunk/doc/screenshots/CUI-DEFAULT.wrl.png' width='320'>