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

#include "AccentButton.h"
#include <map>
#include <tuple>
#include <iostream>

AccentButtonCache::AccentButtonCache()
{}

Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonCache::getIconSurface(Accent button_state,
                                   const Gdk::RGBA& color1,
                                   const Gdk::RGBA& color2)
{
  return icon_surface_map_[{button_state, hashColor(color1), hashColor(color2)}];
}

Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonCache::getTextSurface(const Glib::ustring& text,
                                   const Pango::FontDescription& font,
                                   const Gdk::RGBA& color)
{
  return text_surface_map_[{text.raw(), hashFont(font), hashColor(color)}];
}

Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonCache::getAnimationSurface(const Gdk::RGBA& color)
{
  return animation_surface_map_[{hashColor(color)}];
}

void AccentButtonCache::clearIconSurfaceCache()
{ icon_surface_map_.clear(); }

void AccentButtonCache::clearTextSurfaceCache()
{ text_surface_map_.clear(); }

void AccentButtonCache::clearAnimationSurfaceCache()
{ animation_surface_map_.clear(); }


//static
AccentButtonCache::ColorHash AccentButtonCache::hashColor(const Gdk::RGBA& color)
{
  return (((color.get_red_u()   >> 12) & 0x000f) << 12)
    |    (((color.get_green_u() >> 12) & 0x000f) <<  8)
    |    (((color.get_blue_u()  >> 12) & 0x000f) <<  4)
    |    (((color.get_alpha_u() >> 12) & 0x000f) <<  0);
};

//static
AccentButtonCache::FontHash AccentButtonCache::hashFont(const Pango::FontDescription& font)
{
  return font.hash();
}


namespace
{
  constexpr gint64 kAnimationDuration = 75000; // usecs
  constexpr gushort kAnimationAlphaPeak = 65535; //max: 65535
  constexpr gushort kAnimationMaxFrames = 3;
}

//
// AccentButtonDrawingArea implementation
//
AccentButtonDrawingArea::AccentButtonDrawingArea(Accent state,
                                                 const Glib::ustring& label)
  : button_state_(state),
    label_(label),
    icon_width_(kIconWidth),
    icon_height_(kIconHeight),
    text_width_(0),
    text_height_(0),
    icon_text_padding_(0),
    min_width_(-1),
    min_height_(-1),
    animation_running_(false),
    animation_tick_callback_id_(0),
    animation_alpha_(0)

{
  set_can_focus(false);

  //recalculateDimensions();

  Gtk::Settings::get_default()->property_gtk_font_name().signal_changed()
    .connect(sigc::mem_fun(*this, &AccentButtonDrawingArea::onFontChanged));

  Gtk::Settings::get_default()->property_gtk_theme_name().signal_changed()
    .connect(sigc::mem_fun(*this, &AccentButtonDrawingArea::onThemeChanged));

  signal_style_updated()
    .connect(sigc::mem_fun(*this, &AccentButtonDrawingArea::onStyleChanged));

  get_style_context()->add_class(GTK_STYLE_CLASS_BUTTON);
}


AccentButtonDrawingArea::AccentButtonDrawingArea(AccentButtonDrawingArea&& src)
  : button_state_(std::move(src.button_state_)),
    label_(std::move(src.label_)),
    icon_width_(std::move(src.icon_width_)),
    icon_height_(std::move(src.icon_height_)),
    text_width_(std::move(src.text_width_)),
    text_height_(std::move(src.text_width_)),
    icon_text_padding_(std::move(src.icon_text_padding_)),
    min_width_(std::move(src.min_width_)),
    min_height_(std::move(src.min_height_)),
    animation_running_(std::move(src.animation_running_)),
    animation_tick_callback_id_(std::move(src.animation_tick_callback_id_)),
    animation_alpha_(std::move(src.animation_alpha_))

{}

AccentButtonDrawingArea::~AccentButtonDrawingArea()
{
  if (animation_running_)
    stopAnimation();
}

void AccentButtonDrawingArea::setLabel(const Glib::ustring& label)
{
  if (label.raw() != label_.raw())
  {
    label_ = label;
    queue_resize();
  }
}

void AccentButtonDrawingArea::setAccentState(Accent state)
{
  if (state != button_state_)
  {
    button_state_ = state;
    queue_draw();
  }
}

void AccentButtonDrawingArea::scheduleAnimation(gint64 frame_time)
{
  // we merge overlapping animations by erasing the previously
  // scheduled animations in question

  auto has_overlap = [&frame_time] (const auto& time) -> bool {
    return abs( time - frame_time ) < ( kAnimationDuration );
  };

  auto erase_begin = std::find_if(
    scheduled_animations_.begin(),
    scheduled_animations_.end(),
    has_overlap);

  if (erase_begin != scheduled_animations_.end())
  {
    auto erase_end = std::find_if_not(
      erase_begin,
      scheduled_animations_.end(),
      has_overlap);

    scheduled_animations_.erase(erase_begin, erase_end);
  }

  scheduled_animations_.insert( frame_time );

  if (!animation_running_)
    startAnimation();
}

void AccentButtonDrawingArea::cancelAnimation()
{
  scheduled_animations_.clear();
}

void AccentButtonDrawingArea::startAnimation()
{
  if (animation_running_)
    return;

  animation_tick_callback_id_ = add_tick_callback(
    sigc::mem_fun(*this, &AccentButtonDrawingArea::updateAnimation) );

  animation_running_ = true;
}

void AccentButtonDrawingArea::stopAnimation()
{
  if (!animation_running_)
    return;

  remove_tick_callback(animation_tick_callback_id_);
  animation_running_ = false;
}

bool AccentButtonDrawingArea::updateAnimation(const Glib::RefPtr<Gdk::FrameClock>& clock)
{
  bool need_redraw = false;

  if (clock && button_state_ != kAccentOff)
  {
    gint64 frame_time = 0;

    auto timings = clock->get_current_timings();
    if (timings)
    {
      frame_time = timings->get_predicted_presentation_time();

      if (frame_time == 0)
        frame_time = timings->get_presentation_time();
    }

    // no timings or (predicted) presentation time available
    if (frame_time == 0)
      frame_time = clock->get_frame_time();

    // Remember: scheduled_animations_ contains the start times in descending order,
    //           so the lower bound w.r.t. greater<T> predicate gives us the first
    //           element not greater than frame_time
    auto it = scheduled_animations_.lower_bound(frame_time);

    if (it != scheduled_animations_.end())
    {
      gint64 animation_start_time = *it;
      gint64 animation_end_time = animation_start_time + kAnimationDuration;

      if (frame_time < animation_end_time)
      {
        double alpha_slope = - (double) kAnimationAlphaPeak / kAnimationDuration;
        gint64 time_delta = frame_time - animation_start_time;
        double new_animation_alpha = alpha_slope * time_delta  + kAnimationAlphaPeak;

        if (std::abs(new_animation_alpha - animation_alpha_)
            > ( kAnimationAlphaPeak / kAnimationMaxFrames) )
        {
          animation_alpha_ = std::max(0.0, new_animation_alpha);
          need_redraw = true;
        }
        scheduled_animations_.erase(++it, scheduled_animations_.end());
      }
      else
      {
        if (animation_alpha_ != 0)
        {
          animation_alpha_ = 0;
          need_redraw = true;
        }
        scheduled_animations_.erase(it, scheduled_animations_.end());
      }
    }
    else if (scheduled_animations_.empty())
    {
      if (animation_alpha_ != 0)
      {
        animation_alpha_ = 0;
        need_redraw = true;
      }
      animation_running_ = false;
    }
  }
  else //!clock
  {
    animation_running_ = false;
  }

  if (need_redraw)
    queue_draw_area(0,
                    icon_height_ + icon_text_padding_,
                    get_allocated_width(),
                    get_allocated_height() - icon_height_ - icon_text_padding_);

  return animation_running_;
}

Gtk::SizeRequestMode AccentButtonDrawingArea::get_request_mode_vfunc() const
{
  return Gtk::SIZE_REQUEST_CONSTANT_SIZE;
}

void AccentButtonDrawingArea::get_preferred_width_vfunc(int& minimum_width,
                                                        int& natural_width) const
{
  recalculateDimensions();
  minimum_width = natural_width = min_width_;
}

void AccentButtonDrawingArea::get_preferred_height_for_width_vfunc(int width,
                                                                   int& minimum_height,
                                                                   int& natural_height) const
{
  recalculateDimensions();
  minimum_height = natural_height = min_height_;
}

void AccentButtonDrawingArea::get_preferred_width_for_height_vfunc(int height,
                                                                   int& minimum_width,
                                                                   int& natural_width) const
{
  recalculateDimensions();
  minimum_width = natural_width = min_width_;
}

void AccentButtonDrawingArea::get_preferred_height_vfunc(int& minimum_height,
                                                         int& natural_height) const
{
  recalculateDimensions();
  minimum_height = natural_height = min_height_;
}


void AccentButtonDrawingArea::recalculateDimensions() const
{
  icon_width_  = kIconWidth; //icon_surface->get_width();
  icon_height_ = kIconHeight; //icon_surface->get_height();
  text_width_  = 0;
  text_height_ = 0;
  icon_text_padding_ = 0;

  if (!label_.empty())
  {
    auto style_context = get_style_context();

    const auto widget_state = style_context->get_state();
    Pango::FontDescription font = style_context->get_font(widget_state);
    Gdk::RGBA color = getPrimaryColor(style_context);

    //auto& icon_surface = getIconSurface(button_state_, color);
    auto& text_surface = const_cast<AccentButtonDrawingArea*>(this)->getTextSurface(label_, font, color);

    text_width_  = text_surface->get_width();
    text_height_ = text_surface->get_height();
  }

  if (text_height_ > 0)
    icon_text_padding_ = kPadding;

  min_width_ = std::max( std::max(text_width_, text_height_), icon_width_);
  min_height_ = text_height_ + icon_height_ + icon_text_padding_;
}

void AccentButtonDrawingArea::onFontChanged()
{
  surface_cache_.clearTextSurfaceCache();
  surface_cache_.clearAnimationSurfaceCache();

  queue_resize();
}

void AccentButtonDrawingArea::onThemeChanged()
{
  surface_cache_.clearIconSurfaceCache();
  surface_cache_.clearTextSurfaceCache();
  surface_cache_.clearAnimationSurfaceCache();

  queue_resize();
}

guint AccentButtonDrawingArea::current_font_hash_ = 0;
AccentButtonCache AccentButtonDrawingArea::surface_cache_;


void AccentButtonDrawingArea::onStyleChanged()
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();
  Pango::FontDescription font = style_context->get_font(widget_state);

  guint font_hash = font.hash();

  if (current_font_hash_ != font_hash)
  {
    current_font_hash_ = font_hash;

    surface_cache_.clearTextSurfaceCache();
    surface_cache_.clearAnimationSurfaceCache();

    queue_resize();
  }
}

//static
void AccentButtonDrawingArea::drawIconSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                                              Accent button_state,
                                              const Gdk::RGBA& color1,
                                              const Gdk::RGBA& color2)
{
  auto cairo_context = Cairo::Context::create(surface);

  Gdk::Cairo::set_source_rgba(cairo_context, color1);

  double surface_width = surface->get_width();

  double L = 0;
  double R = surface_width;
  double M = ( L + R ) / 2.;

  switch ( button_state ) {
  case kAccentStrong:
  {
    cairo_context->move_to(M,1);
    cairo_context->line_to(L,6);
    cairo_context->line_to(L,10);
    cairo_context->line_to(R,10);
    cairo_context->line_to(R,6);
    cairo_context->line_to(M,1);

    Gdk::Cairo::set_source_rgba(cairo_context, color2);
    cairo_context->fill();

    Gdk::Cairo::set_source_rgba(cairo_context, color1);
    [[fallthrough]];
  }
  case kAccentMid:
  {
    cairo_context->rectangle( L, 11, surface_width, 4);
    cairo_context->fill();
    [[fallthrough]];
  }
  case kAccentWeak:
  {
    cairo_context->rectangle( L, 16, surface_width, 4);
    cairo_context->fill();
  }
  break;
  case kAccentOff:
  {
    Gdk::RGBA tr_color = color1;
    tr_color.set_alpha(color1.get_alpha() * .3);
    Gdk::Cairo::set_source_rgba(cairo_context, tr_color);

    cairo_context->rectangle( L, 19, surface_width, 1);
    cairo_context->fill();
  }
  break;

  default:
    break;
  };
}

//static
void AccentButtonDrawingArea::drawTextSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                                              Glib::RefPtr<Pango::Layout>& layout,
                                              const Gdk::RGBA& color)
{
  auto cairo_context = Cairo::Context::create(surface);

  Gdk::Cairo::set_source_rgba(cairo_context, color);

  layout->show_in_cairo_context(cairo_context);
}

//static
void AccentButtonDrawingArea::drawAnimationSurface(Cairo::RefPtr<Cairo::ImageSurface>& surface,
                                                   const Gdk::RGBA& color)
{
  auto cairo_context = Cairo::Context::create(surface);

  Gdk::Cairo::set_source_rgba(cairo_context, color);

  cairo_context->translate( (double) surface->get_width() / 2.,
                            (double) surface->get_height() / 2.);

  int scale = surface->get_height() / 2;

  cairo_context->scale(scale, scale);
  cairo_context->arc(0.0, 0.0, 1.0, 0.0, 2 * M_PI);
  cairo_context->fill();
}


const Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonDrawingArea::getIconSurface(Accent button_state,
                                        const Gdk::RGBA& color1,
                                        const Gdk::RGBA& color2)
{
  auto& surface = surface_cache_.getIconSurface(button_state, color1, color2);

  if (!surface)
  {
    int surface_width = icon_width_;
    int surface_height = icon_height_;

    if (surface_height > 0 && surface_width > 0)
    {
      surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                            surface_width,
                                            surface_height);

      drawIconSurface(surface, button_state, color1, color2);
    }
  }
  return surface;
}

const Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonDrawingArea::getTextSurface(const Glib::ustring& text,
                                        const Pango::FontDescription& font,
                                        const Gdk::RGBA& color)
{
  auto& surface = surface_cache_.getTextSurface(text, font, color);

  if (!surface)
  {
    auto pango_context = create_pango_context();

    Glib::RefPtr<Pango::Layout> layout = Pango::Layout::create(pango_context);
    layout->set_font_description(font);
    layout->set_text(text);

    // measure text dimensions
    int surface_width, surface_height;
    layout->get_pixel_size(surface_width, surface_height);

    if (surface_height > 0 && surface_width > 0)
    {
      surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                            surface_width,
                                            surface_height);

      drawTextSurface(surface, layout, color);
    }
  }
  return surface;
}

const Cairo::RefPtr<Cairo::ImageSurface>&
AccentButtonDrawingArea::getAnimationSurface(const Gdk::RGBA& color)
{
  auto& surface = surface_cache_.getAnimationSurface(color);

  if (!surface)
  {
    int surface_height = get_allocated_height() - icon_height_ - kPadding;
    int surface_width = surface_height;

    if (surface_height > 0 /* && surface_width > 0 */)
    {
      surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32,
                                            surface_width,
                                            surface_height);
      drawAnimationSurface(surface, color);
    }
  }
  return surface;
}


Gdk::RGBA AccentButtonDrawingArea::getPrimaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state);

  return color;
}

Gdk::RGBA AccentButtonDrawingArea::getSecondaryColor(Glib::RefPtr<Gtk::StyleContext> context) const
{
  Gtk::StateFlags widget_state = context->get_state();
  Gdk::RGBA color = context->get_color(widget_state | Gtk::STATE_FLAG_LINK);
  return color;
}

bool AccentButtonDrawingArea::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  draw_animation(cr);
  draw_icon(cr);
  draw_text(cr);

  return false;
}

void AccentButtonDrawingArea::draw_icon(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();
  Gdk::RGBA color1 = getPrimaryColor(style_context);
  Gdk::RGBA color2 = getSecondaryColor(style_context);

  auto& surface = getIconSurface(button_state_, color1, color2);

  if (surface)
  {
    double L = (get_allocated_width() - icon_width_) / 2.;

    L = round (L);

    cr->set_source( surface , L, 0 );
    cr->rectangle( L, 0, icon_width_, icon_height_);
    cr->fill();
  }
}

void AccentButtonDrawingArea::draw_text(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();
  Gtk::StateFlags widget_state = style_context->get_state();

  Pango::FontDescription font = style_context->get_font(widget_state);
  Gdk::RGBA color = getPrimaryColor(style_context);

  // apply animation alpha value
  //color2.set_alpha_u(animation_alpha_);

  auto& surface = getTextSurface(label_, font, color);

  if (surface)
  {
    int surface_width = surface->get_width();
    int surface_height = surface->get_height();

    double L = (get_allocated_width() - surface_width) / 2.;
    double T = get_allocated_height() - surface_height;

    L = round (L);
    T = round (T);

    cr->set_source( surface, L, T );
    cr->rectangle( L, T, surface_width, surface_height);
    cr->fill();
  }
}

void AccentButtonDrawingArea::draw_animation(const Cairo::RefPtr<Cairo::Context>& cr)
{
  auto style_context = get_style_context();
  Gdk::RGBA color;

  switch (button_state_)
  {
  case kAccentStrong:
    color = getSecondaryColor(style_context);
    color.set_alpha_u(0.9 * animation_alpha_);
    break;
  case kAccentMid:
    color = getPrimaryColor(style_context);
    color.set_alpha_u(0.6 * animation_alpha_);
    break;
  case kAccentWeak:
    color = getPrimaryColor(style_context);
    color.set_alpha_u(0.2 * animation_alpha_);
    break;

  default:
  case kAccentOff:
    color = getPrimaryColor(style_context);
    color.set_alpha_u(0.0);
    break;
  };

  auto& surface = getAnimationSurface(color);
  if (surface)
  {
    int surface_width = surface->get_width();
    int surface_height = surface->get_height();

    double L = (get_allocated_width() - surface_width) / 2.;
    double T = get_allocated_height() - surface_height;

    L = round (L);
    T = round (T);

    cr->set_source( surface, L, T );
    cr->rectangle( L, T, surface_width, surface_height);
    cr->fill();
  }
}

//
// AccentButton implementation
//
AccentButton::AccentButton(Accent state, const Glib::ustring& label)
  : drawing_area_(state, label)
{
  set_can_focus(true);
  set_focus_on_click(false);

  set_relief(Gtk::RELIEF_NONE);

  add(drawing_area_);
  drawing_area_.show();

  add_events(Gdk::SCROLL_MASK);
}

AccentButton::AccentButton(AccentButton&& src)
  : drawing_area_(std::move(src.drawing_area_)),
    signal_accent_state_changed_(std::move(src.signal_accent_state_changed_))
{}

AccentButton::~AccentButton()
{}

void AccentButton::setAccentState(Accent state)
{
  drawing_area_.setAccentState(state);
}

void AccentButton::setLabel(const Glib::ustring& label)
{
  drawing_area_.setLabel(label);
}

void AccentButton::scheduleAnimation(gint64 frame_time)
{
  drawing_area_.scheduleAnimation(frame_time);
}

void AccentButton::cancelAnimation()
{
  drawing_area_.cancelAnimation();
}

void AccentButton::on_clicked()
{
  switch (getAccentState()) {
  case kAccentStrong: setAccentState(kAccentOff); break;
  case kAccentMid: setAccentState(kAccentStrong); break;
  case kAccentWeak: setAccentState(kAccentMid); break;
  case kAccentOff: setAccentState(kAccentWeak); break;
  default: break;
  }
  signal_accent_state_changed_.emit();
  Button::on_clicked();
}

bool AccentButton::on_scroll_event(GdkEventScroll *scroll_event)
{
  bool state_changed = false;

  switch (scroll_event->direction) {

  case GDK_SCROLL_UP:
  case GDK_SCROLL_RIGHT:

    switch (getAccentState()) {
    case kAccentOff:
      setAccentState(kAccentWeak);
      state_changed = true;
      break;
    case kAccentWeak:
      setAccentState(kAccentMid);
      state_changed = true;
      break;
    case kAccentMid:
      setAccentState(kAccentStrong);
      state_changed = true;
      break;
    default:
      break;
    };

    break;

  case GDK_SCROLL_DOWN:
  case GDK_SCROLL_LEFT:

    switch (getAccentState()) {
    case kAccentStrong:
      setAccentState(kAccentMid);
      state_changed = true;
      break;
    case kAccentMid:
      setAccentState(kAccentWeak);
      state_changed = true;
      break;
    case kAccentWeak:
      setAccentState(kAccentOff);
      state_changed = true;
      break;
    default:
      break;
    };

    break;

  default:
    break;
  };

  if (state_changed)
    signal_accent_state_changed_.emit();

  return Gtk::Button::on_scroll_event(scroll_event);
}
