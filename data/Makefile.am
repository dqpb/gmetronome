### The desktop files
desktop_in_file = gmetronome.desktop.in
desktop_file = $(desktop_in_file:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_file)

$(desktop_file): $(desktop_in_file) $(top_srcdir)/po/*.po
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

# The application icon
appicondir = $(datadir)/icons/hicolor/48x48/apps
appicon_DATA = icons/gmetronome.png

### Resources files
resource_files = \
	css/global.css \
	ui/SettingsDialog.glade \
	ui/MainWindow.glade \
	ui/ShortcutDialog.glade \
	icons/gmetronome.png \
	icons/gmetronome.svg

gresource_xml_file = gmetronome.gresource.xml
resources_file = resources.cpp

### GSchema files
gschema_in_file = org.gmetronome.gschema.xml.in
gschema_file = $(gschema_in_file:.xml.in=.xml)

nodist_noinst_DATA = \
	$(resources_file) \
	$(gschema_file)

$(resources_file): $(gresource_xml_file) $(resource_files)
	$(AM_V_GEN)$(GLIB_COMPILE_RESOURCES) $(srcdir)/$(gresource_xml_file) \
		--target=$@ --sourcedir=$(srcdir) --generate-source

gschema_sed_script=

if HAVE_ALSA
gschema_sed_script+=-e "/HAVE_ALSA/d"
else
gschema_sed_script+=-e "/HAVE_ALSA_BEGIN/,/HAVE_ALSA_END/d"
endif
if HAVE_PULSEAUDIO
gschema_sed_script+=-e "/HAVE_PULSEAUDIO/d"
else
gschema_sed_script+=-e "/HAVE_PULSEAUDIO_BEGIN/,/HAVE_PULSEAUDIO_END/d"
endif

gschema_sed_script+= -e "/DEFAULT_AUDIO_BACKEND_BEGIN/,/DEFAULT_AUDIO_BACKEND_END/!b" \
                     -e "s/.*DEFAULT_AUDIO_BACKEND_END.*/'@DEFAULT_AUDIO_BACKEND@'/;t" \
                     -e "d"

$(gschema_file): $(gschema_in_file)
	$(AM_V_GEN)$(SED) $(gschema_sed_script) $< > $@

### GSettings
gsettings_SCHEMAS = $(gschema_file)
@GSETTINGS_RULES@

CLEANFILES = \
	$(resources_file) \
	$(gschema_file) \
	$(desktop_file)

EXTRA_DIST = \
	$(gresource_xml_file) \
	$(gschema_in_file) \
	$(resource_files) \
	$(desktop_in_file)

