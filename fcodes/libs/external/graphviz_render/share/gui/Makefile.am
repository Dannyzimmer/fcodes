# ## Process this file with automake to produce Makefile.in
#

guidir = $(pkgdatadir)/smyrna
gui = smyrna.glade attrs.txt arial.tga template.dot attr_widgets.dot mouse_actions.txt
iconsdir = $(pkgdatadir)/smyrna
icons = cursor.png \
	fit_to_screen.png \
	move.png new_delete.png \
	pan.png selectx.png zoom_in.png \
	zoom_out.png zoom.png \
	fisheye.raw pan.raw zoom.raw \
	selectr.png single_select.png 2D.png 3D.png center.png \
	details.png fisheye.png mod_3D.png \
	mod_fisheye.png no_fisheye.png \
	zoomin.png zoomout.png

if WITH_SMYRNA
gui_DATA = $(gui)
icons_DATA = $(icons)
endif

EXTRA_DIST = $(gui) $(icons)
