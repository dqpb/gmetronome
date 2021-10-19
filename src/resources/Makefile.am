### The desktop files
desktop_in_file = gmetronome.desktop.in
desktop_file = $(desktop_in_file:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_file)

$(desktop_file): $(desktop_in_file) $(top_srcdir)/po/*.po
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

# The application icon
appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = gmetronome.png

### Application resources
resource_files = \
	css/global.css \
	ui/SettingsDialog.glade \
	ui/MainWindow.glade \
	ui/ShortcutDialog.glade \
	gmetronome.png \
	gmetronome.svg

gresource_xml_file = gmetronome.gresource.xml

nodist_noinst_DATA = resources.cpp

resources.cpp: $(gresource_xml_file) $(resource_files)
	$(GLIB_COMPILE_RESOURCES) $(srcdir)/$(gresource_xml_file) \
		--target=$@ --sourcedir=$(srcdir) --generate-source

### GSettings
gsettings_SCHEMAS = org.gmetronome.gschema.xml
@GSETTINGS_RULES@

CLEANFILES = \
	resources.cpp \
	$(desktop_file)

EXTRA_DIST = \
	$(gresource_xml_file) \
	$(gsettings_SCHEMAS) \
	$(resource_files) \
	$(desktop_in_file)

