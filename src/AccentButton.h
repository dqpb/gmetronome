/*
 * Copyright (C) 2020 The GMetronome Team
 *
 * This file is part of GMetronome.
 *
 * GMetronome is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GMetronome is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMetronome.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GMetronome_AccentButton_h
#define GMetronome_AccentButton_h

#include "Meter.h"
#include <gtkmm.h>
#include <set>

/**
 * @class AccentButtonCache
 */
class AccentButtonCache {
public:
  AccentButtonCache();

  Cairo::RefPtr<Cairo::ImageSurface>&
  getIconSurface(Accent button_state,
                 const Gdk::RGBA& color1,
                 const Gdk::RGBA& color2);

  Cairo::RefPtr<Cairo::ImageSurface>&
  getTextSurface(const Glib::ustring& text,
                 const Pango::FontDescription& font,
                 const Gdk::RGBA& color);

  Cairo::RefPtr<Cairo::ImageSurface>&
  getAnimationSurface(const Gdk::RGBA& color);

  void clearIconSurfaceCache();
  void clearTextSurfaceCache();
  void clearAnimationSurfaceCache();

private:
  using ColorHash = uint32_t;
  using FontHash = guint;

  static ColorHash hashColor(const Gdk::RGBA& color);
  static FontHash hashFont(const Pango::FontDescription& font);

  using IconSurfaceMap = std::map <
    std::tuple<Accent,ColorHash,ColorHash>,
    Cairo::RefPtr<Cairo::ImageSurface>
    >;

  using TextSurfaceMap =  std::map <
    std::tuple<std::string,FontHash,ColorHash>,
    Cairo::RefPtr<Cairo::ImageSurface>
    >;

  using AnimationSurfaceMap =  std::map <
    std::tuple<ColorHash>,
    Cairo::RefPtr<Cairo::ImageSurface>
    >;

  IconSurfaceMap icon_surface_map_;
  TextSurfaceMap text_surface_map_;
  AnimationSurfaceMap animation_surface_map_;
};

/**
 * @class AccentButtonDrawingArea
 */
class AccentButtonDrawingArea : public Gtk::DrawingArea {
public:
  AccentButtonDrawingArea(Accent state = kAccentMid,
                          const Glib::ustring& label = "");

  AccentButtonDrawingArea(const AccentButtonDrawingArea&) = delete;

  AccentButtonDrawingArea(AccentButtonDrawingArea&& src);

  virtual ~AccentButtonDrawingArea();

  void setAccentState(Accent state);

  Accent getAccentState() const
    { return button_state_; }

  void setLabel(const Glib::ustring& label);

  const Glib::ustring& getLabel() const
    { return label_; }

  void scheduleAnimation(gint64 frame_time);

  void cancelAnimation();

protected:
  Accent button_state_;
  Glib::ustring label_;

  static constexpr int kIconWidth = 16;
  static constexpr int kIconHeight = 20;
  static constexpr int kPadding = 1;

  // cache
  mutable int icon_width_;
  mutable int icon_height_;
  mutable int text_width_;
  mutable int text_height_;
  mutable int icon_text_padding_;
  mutable int min_width_;
  mutable int min_height_;

  static guint current_font_hash_;
  static AccentButtonCache surface_cache_;

  // animation start times in reverse order
  std::set<gint64, std::greater<gint64>> scheduled_animations_;

  bool animation_running_;
  guint animation_tick_callback_id_;
  gushort animation_alpha_;

  void startAnimation();

  void stopAnimation();

  bool updateAnimation(const Glib::RefPtr<Gdk::FrameClock>&);

  Gtk::SizeRequestMode get_request_mode_vfunc() const override;

  void get_preferred_width_vfunc(int& minimum_width,
                                 int& natural_width) const override;

  void get_preferred_height_for_width_vfunc(int width,
                                            int& minimum_height,
                                            int& natural_height) const  override;

  void get_preferred_height_vfunc(int& minimum_height,
                                  int& natural_height) const override;

  void get_preferred_width_for_height_vfunc(int height,
                                            int& minimum_width,
                                            int& natural_width) const override;

  void recalculateDimensions() const;

  void onFontChanged();
  void onThemeChanged();
  void onStyleChanged();

  Gdk::RGBA getPrimaryColor(Glib::RefPtr<Gtk::StyleContext>) const;
  Gdk::RGBA getSecondaryColor(Glib::RefPtr<Gtk::StyleContext>) const;

  //Override default signal handler:
  bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) override;
  void draw_icon(const Cairo::RefPtr<Cairo::Context>& cr);
  void draw_text(const Cairo::RefPtr<Cairo::Context>& cr);
  void draw_animation(const Cairo::RefPtr<Cairo::Context>& cr);

  const Cairo::RefPtr<Cairo::ImageSurface>&
  getIconSurface(Accent button_state,
                 const Gdk::RGBA& color1,
                 const Gdk::RGBA& color2);

  const Cairo::RefPtr<Cairo::ImageSurface>&
  getTextSurface(const Glib::ustring& text,
                 const Pango::FontDescription& font,
                 const Gdk::RGBA& color);

  const Cairo::RefPtr<Cairo::ImageSurface>&
  getAnimationSurface(const Gdk::RGBA& color);

public:
  static void drawIconSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                              Accent button_state,
                              const Gdk::RGBA& color1,
                              const Gdk::RGBA& color2);

  static void drawTextSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                              Glib::RefPtr<Pango::Layout>& layout,
                              const Gdk::RGBA& color);

  static void drawAnimationSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                                   const Gdk::RGBA& color);
};

/**
 * @class AccentButton
 */
class AccentButton : public Gtk::Button {
public:
  AccentButton(Accent state = kAccentMid,
               const Glib::ustring& label = "");

  AccentButton(const AccentButton&) = delete;

  AccentButton(AccentButton&& src);

  virtual ~AccentButton();

  void setAccentState(Accent state);

  Accent getAccentState() const
    { return drawing_area_.getAccentState(); }

  void setLabel(const Glib::ustring& label);

  const Glib::ustring& getLabel() const
    { return drawing_area_.getLabel(); }

  void scheduleAnimation(gint64 frame_time);

  void cancelAnimation();

  AccentButtonDrawingArea& getDrawingArea()
    { return drawing_area_; }

  const AccentButtonDrawingArea& getDrawingArea() const
    { return drawing_area_; }

  sigc::signal<void()> signal_accent_state_changed()
    { return signal_accent_state_changed_; }

protected:
  void on_clicked() override;
  bool on_scroll_event(GdkEventScroll *scroll_event) override;
  bool on_button_press_event(GdkEventButton * button_event) override;
  bool on_button_release_event(GdkEventButton * button_event) override;

  bool setNextAccentState(bool cycle = false);
  bool setPrevAccentState(bool cycle = false);

private:
  AccentButtonDrawingArea drawing_area_;
  sigc::signal<void()> signal_accent_state_changed_;
};

#endif//GMetronome_AccentButton_h
