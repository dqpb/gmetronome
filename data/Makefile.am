### The desktop files
desktop_in_file = gmetronome.desktop.in
desktop_file = $(desktop_in_file:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_file)

$(desktop_file): $(desktop_in_file) $(top_srcdir)/po/*.po
	$(AM_V_GEN)$(MSGFMT) --desktop --template $(srcdir)/$(desktop_in_file) -d $(top_srcdir)/po -o $@

### Icons
icons16dir = $(datadir)/icons/hicolor/16x16/apps
icons16_DATA = icons/16x16/gmetronome.png

icons32dir = $(datadir)/icons/hicolor/32x32/apps
icons32_DATA = icons/32x32/gmetronome.png

icons48dir = $(datadir)/icons/hicolor/48x48/apps
icons48_DATA = icons/48x48/gmetronome.png

icons64dir = $(datadir)/icons/hicolor/64x64/apps
icons64_DATA = icons/64x64/gmetronome.png

icons256dir = $(datadir)/icons/hicolor/256x256/apps
icons256_DATA = icons/256x256/gmetronome.png

iconsscalabledir = $(datadir)/icons/hicolor/scalable/apps
iconsscalable_DATA = icons/scalable/gmetronome.svg

### Resources files
resource_files = \
	css/global.css \
	ui/SettingsDialog.glade \
	ui/MainWindow.glade \
	ui/ShortcutDialog.glade \
	icons/16x16/gmetronome.png \
	icons/32x32/gmetronome.png \
	icons/48x48/gmetronome.png \
	icons/64x64/gmetronome.png \
	icons/256x256/gmetronome.png \
	icons/scalable/gmetronome.svg 

gresource_xml_file = gmetronome.gresource.xml

### GSchema files
gschema_in_file = org.gmetronome.gschema.xml.in
gschema_file = $(gschema_in_file:.xml.in=.xml)

nodist_noinst_DATA = $(gschema_file)

gschema_sed_script=

if HAVE_ALSA
gschema_sed_script+=-e "/HAVE_ALSA/d"
else
gschema_sed_script+=-e "/HAVE_ALSA_BEGIN/,/HAVE_ALSA_END/d"
endif
if HAVE_OSS
gschema_sed_script+=-e "/HAVE_OSS/d"
else
gschema_sed_script+=-e "/HAVE_OSS_BEGIN/,/HAVE_OSS_END/d"
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
	$(AM_V_GEN)$(SED) $(gschema_sed_script) $(srcdir)/$(gschema_in_file) > $@

gschema_valid_file = $(gschema_file:.xml=.valid)

$(gschema_valid_file): $(gschema_file)
	$(AM_V_GEN) $(GLIB_COMPILE_SCHEMAS) --strict --dry-run --schema-file=$(gschema_file) && touch $@

install-gsettings-schemas: $(gschema_file) $(gschema_valid_file)
	@$(NORMAL_INSTALL)
	if test -n "$(gschema_file)"; then \
		test -z "$(gsettingsschemadir)" || $(MKDIR_P) "$(DESTDIR)$(gsettingsschemadir)"; \
		$(INSTALL_DATA) $(gschema_file) "$(DESTDIR)$(gsettingsschemadir)"; \
		test -n "$(GSETTINGS_DISABLE_SCHEMAS_COMPILE)$(DESTDIR)" || $(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir); \
	fi

uninstall-gsettings-schemas:
	@$(NORMAL_UNINSTALL)
	file=`echo "$(gschema_file)" | sed -e "s|^.*/||"`; \
	test -n "$$file" || exit 0; \
	echo " ( cd '\''$(DESTDIR)$(gsettingsschemadir)'\'' && rm -f" $$file ")"; \
	cd "$(DESTDIR)$(gsettingsschemadir)" && rm -f "$$file"
	test -n "$(GSETTINGS_DISABLE_SCHEMAS_COMPILE)$(DESTDIR)" || $(GLIB_COMPILE_SCHEMAS) $(gsettingsschemadir)

clean-gsettings-schemas:
	rm -f $(gschema_valid_file)

.PHONY : uninstall-gsettings-schemas install-gsettings-schemas clean-gsettings-schemas

all-local: $(gschema_valid_file)

uninstall-local: uninstall-gsettings-schemas

install-data-local: install-gsettings-schemas

mostlyclean-local: clean-gsettings-schemas

CLEANFILES = \
	$(gschema_file) \
	$(desktop_file)

EXTRA_DIST = \
	$(gresource_xml_file) \
	$(gschema_in_file) \
	$(resource_files) \
	$(desktop_in_file)

