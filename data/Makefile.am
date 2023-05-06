### Desktop
desktop_in_file = org.gnome.gitlab.dqpb.GMetronome.desktop.in
desktop_file = $(desktop_in_file:.desktop.in=.desktop)

desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_file)

$(desktop_file): $(desktop_in_file) $(top_srcdir)/po/*.po
	$(AM_V_GEN)$(MSGFMT) --desktop --template $(desktop_in_file) -d $(top_srcdir)/po -o $@

### Appstream
metainfo_in_file = org.gnome.gitlab.dqpb.GMetronome.metainfo.xml.in
metainfo_file = $(metainfo_in_file:.xml.in=.xml)

metainfodir = $(datadir)/metainfo
metainfo_DATA = $(metainfo_file)

$(metainfo_file): $(metainfo_in_file) $(top_srcdir)/po/*.po
	$(AM_V_GEN)$(MSGFMT) --xml --template $(metainfo_in_file) -d $(top_srcdir)/po -o $@

### Icons
icons16dir = $(datadir)/icons/hicolor/16x16/apps
icons16_DATA = icons/16x16/org.gnome.gitlab.dqpb.GMetronome.png

icons32dir = $(datadir)/icons/hicolor/32x32/apps
icons32_DATA = icons/32x32/org.gnome.gitlab.dqpb.GMetronome.png

icons48dir = $(datadir)/icons/hicolor/48x48/apps
icons48_DATA = icons/48x48/org.gnome.gitlab.dqpb.GMetronome.png

icons64dir = $(datadir)/icons/hicolor/64x64/apps
icons64_DATA = icons/64x64/org.gnome.gitlab.dqpb.GMetronome.png

icons128dir = $(datadir)/icons/hicolor/128x128/apps
icons128_DATA = icons/128x128/org.gnome.gitlab.dqpb.GMetronome.png

icons256dir = $(datadir)/icons/hicolor/256x256/apps
icons256_DATA = icons/256x256/org.gnome.gitlab.dqpb.GMetronome.png

iconsscalabledir = $(datadir)/icons/hicolor/scalable/apps
iconsscalable_DATA = icons/scalable/org.gnome.gitlab.dqpb.GMetronome.svg

iconssymbolicdir = $(datadir)/icons/hicolor/symbolic/apps
iconssymbolic_DATA = icons/scalable/org.gnome.gitlab.dqpb.GMetronome-symbolic.svg

### GSchema
gschema_in_file = org.gnome.gitlab.dqpb.GMetronome.gschema.xml.in
gschema_file = $(gschema_in_file:.xml.in=.xml)
gschema_valid_file = $(gschema_file:.xml=.valid)

#
# Since GSETTINGS_RULES uses GNU make specific syntax and is therefore unportable
# we use a slightly simplified and portable modification here
#
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

install-data-hook:
	@-if test -z "$(DESTDIR)"; then	\
		echo "Updating icon theme cache."; \
		gtk-update-icon-cache -f -t $(datadir)/icons/hicolor; \
	fi

CLEANFILES = \
	$(desktop_file) \
	$(metainfo_file) \
	$(gschema_file)

EXTRA_DIST = \
	$(gschema_in_file) \
	icons/16x16/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/32x32/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/48x48/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/64x64/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/128x128/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/256x256/org.gnome.gitlab.dqpb.GMetronome.png \
	icons/scalable/org.gnome.gitlab.dqpb.GMetronome.svg \
	icons/scalable/org.gnome.gitlab.dqpb.GMetronome-symbolic.svg
