# set the include path for X, qt and KDE
INCLUDES = $(all_includes)

# these are the headers for your project
noinst_HEADERS = terminoa.h WidgetZoneImage.h dialogetatscles.h pointscomm.h \
	ListesCommandesReponsesTerminOA.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/terminoa.pot

KDE_ICON = AUTO

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = terminoa

# the application source, library search path, and link libraries
terminoa_SOURCES = main.cpp terminoa.cpp WidgetZoneImage.cpp DialogCleUSB.ui \
	dialogetatscles.cpp pointscomm.cpp
terminoa_LDFLAGS = $(KDE_RPATH) $(all_libraries)
terminoa_LDADD = -lssl -lCCfits $(LIB_KDEUI)

# this is where the desktop file will go 
shelldesktopdir   = $(kde_appsdir)/Utilities
shelldesktop_DATA = terminoa.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir   = $(kde_datadir)/terminoa
shellrc_DATA = terminoaui.rc

