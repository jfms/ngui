/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, xuewen.chu
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of xuewen.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL xuewen.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */

#include "event.h"
#include "app-1.h"
#include "root.h"
#include "button.h"
#include "css.h"

XX_NS(ngui)

#define XX_FUN(NAME, S, CATEGORY, BUBBLE) \
const GUIEventName GUI_EVENT_##NAME(#S, GUI_EVENT_CATEGORY_##CATEGORY, BUBBLE);
XX_GUI_EVENT_TABLE(XX_FUN)
#undef XX_FUN

const Map<String, GUIEventName> GUI_EVENT_TABLE([]() -> Map<String, GUIEventName> {
  Map<String, GUIEventName> r;
#define XX_FUN(NAME, S, CATEGORY, BUBBLE) \
r.set(GUI_EVENT_##NAME.to_string(), GUI_EVENT_##NAME);
XX_GUI_EVENT_TABLE(XX_FUN)
#undef XX_FUN
  return r;
}());

static inline HighlightedStatus HOVER_or_NORMAL(View* view) {
  return view->is_focus() ? HIGHLIGHTED_HOVER : HIGHLIGHTED_NORMAL;
}

template<class T, typename... Args>
inline static Handle<T> NewEvent(Args... args) { return new T(args...); }

XX_DEFINE_INLINE_MEMBERS(View, EventInl) {
public:
#define _inl_view(self) static_cast<View::EventInl*>(static_cast<View*>(self))
  
  /**
   * @func trigger_highlightted
   */
  ReturnValue& trigger_highlightted(GUIHighlightedEvent& evt) {
    View* view = this;
    if ( view ) {
      if ( view->m_receive ) {
        view->Notification::trigger(GUI_EVENT_HIGHLIGHTED, evt);
        if ( evt.is_default() ) {
          CSSViewClasss* classs = view->classs();
          if ( classs ) { // 切换样式表状态
            classs->set_style_pseudo_status( CSSPseudoClass(evt.status()) );
          }
        }
      }
    }
    return evt.return_value;
  }
  
  /**
   * @func bubble_trigger
   */
  int& bubble_trigger(const NameType& name, GUIEvent& evt) {
    View* view = this;
    while( view ) {
      if ( view->m_receive ) {
        view->Notification::trigger(name, evt);
        if ( !evt.is_bubble() ) {
          break; // Stop bubble
        }
      }
      view = view->parent();
    }
    return evt.return_value;
  }
};

/**
 * @func trigger
 */
int& View::trigger(const NameType& name, GUIEvent& evt, bool need_send) {
  if ( m_receive || need_send ) {
    return Notification::trigger(name, evt);
  }
  return evt.return_value;
}

int View::trigger(const NameType& name, bool need_send) {
  if ( m_receive || need_send ) {
    auto del = noticer(name);
    if ( del ) {
      return del->trigger( **NewEvent<GUIEvent>(this) );
    }
  }
  return 0;
}

void View::trigger_listener_change(const NameType& name, int count, int change) {
  if ( change > 0 ) {
    m_receive = true; // bind event auto open option
  }
}

/**
 * @func focus
 */
bool View::focus() {
  if ( is_focus() ) return true;
  
  View* old = app()->focus_view();
  Handle<GUIFocusMoveEvent> evt;
  
  SelectPanel* panel = reinterpret_cast<Button*>(this)->panel();
  if ( panel ) {
    evt = NewEvent<GUIFocusMoveEvent>(panel, old, this);
    panel->trigger(GUI_EVENT_FOCUS_MOVE, **evt );
  }
  
  if ( (*evt && !evt->is_default()) || !_inl_app(app())->set_focus_view(this) ) {
    return false;
  }
  
  if ( old ) {
    _inl_view(old)->bubble_trigger(GUI_EVENT_BLUR, **NewEvent<GUIEvent>(old));
    _inl_view(old)->trigger_highlightted(
      **NewEvent<GUIHighlightedEvent>(old, HIGHLIGHTED_NORMAL)
    );
  }
  _inl_view(this)->bubble_trigger(GUI_EVENT_FOCUS, **NewEvent<GUIEvent>(this));
  _inl_view(this)->trigger_highlightted(
     **NewEvent<GUIHighlightedEvent>(this, HIGHLIGHTED_HOVER)
  );
  return true;
}

/**
 * @func blur
 */
bool View::blur() {
  if ( is_focus() ) {
    View* v = root();
    if ( v ) {
      if ( v != this ) {
        return v->focus();
      }
    }
    return false;
  }
  return true;
}

/**
 * @func is_focus
 */
bool View::is_focus() const {
  auto app_ = _inl_app(app());
  return app_ && this == app_->focus_view();
}

void View::set_is_focus(bool value) {
  if ( value ) {
    focus();
  } else {
    blur();
  }
}

/**
 * @func can_become_focus() 可以成为焦点
 */
bool View::can_become_focus() {
  return false;
}

/**
 * @class GUIEventDispatch::OriginTouche
 */
class GUIEventDispatch::OriginTouche {
public:
  
  OriginTouche() { XX_UNREACHABLE(); }
  OriginTouche(View* view)
  : _view(view)
  , _start_position(view_position(view))
  , _is_click_invalid(false), _is_click_down(false) {
    _view->retain();
  }
  ~OriginTouche() {
    _view->release();
  }
  static Vec2 view_position(View* view) {
    return Vec2(view->m_final_matrix[2], view->m_final_matrix[5]);
  }
  inline View* view() { return _view; }
  inline Vec2 view_start_position() { return _start_position; }
  inline bool is_click_invalid() { return _is_click_invalid; }
  inline bool is_click_down() { return _is_click_down; }
  inline void set_click_invalid() {
    _is_click_invalid = true;
    _is_click_down = false;
  }
  inline void set_click_down(bool value) {
    if ( !_is_click_invalid )
      _is_click_down = value;
  }
  inline Map<uint, GUITouch>& values() { return _touches; }
  inline GUITouch& operator[](uint id) { return _touches[id]; }
  inline uint count() { return _touches.length(); }
  inline bool has(uint id) { return _touches.has(id); }
  inline void del(uint id) { _touches.del(id); }
private:
  View* _view;
  Map<uint, GUITouch> _touches;
  Vec2  _start_position;
  bool  _is_click_invalid;
  bool  _is_click_down;
};

/**
 * @class GUIEventDispatch::Inl
 */
XX_DEFINE_INLINE_MEMBERS(GUIEventDispatch, Inl) {
public:
  #define _inl_di(self) static_cast<GUIEventDispatch::Inl*>(self)
  
  // -------------------------- touch --------------------------
  
  void touchstart_2(View* view, List<GUITouch>& in) {
    if ( view->receive() && view->m_visible_draw && in.length() ) {
      Array<GUITouch> change_touches;
      
      for ( auto i = in.begin(), e = in.end(); i != e; ) {
        auto j = i++;
        
        if ( view->overlap_test(Vec2(j.value().x, j.value().y)) ) {
          GUITouch& touch = j.value();
          touch.start_x = touch.x;
          touch.start_y = touch.y;
          touch.click_in = true;
          touch.view = view;
          
          if ( !m_origin_touches.has(view) ) {
            m_origin_touches.set(view, new OriginTouche(view));
          }
          (*m_origin_touches[view])[touch.id] = touch;
          
          change_touches.push( touch );
          in.del(j);
        }
      }
      
      if ( change_touches.length() ) { // notice
        auto evt = NewEvent<GUITouchEvent>(view, change_touches);
        _inl_view(view)->bubble_trigger(GUI_EVENT_TOUCHSTART, **evt); // emit event
        
        if ( !m_origin_touches[view]->is_click_down() ) { // trigger click down
          m_origin_touches[view]->set_click_down(true);
          auto evt = NewEvent<GUIHighlightedEvent>(view, HIGHLIGHTED_DOWN);
          _inl_view(view)->trigger_highlightted(**evt); // emit event
        }
      }
    }
  }
  
  /**
   * @func touchstart
   */
  void touchstart(View* view, List<GUITouch>& in) {
    
    if ( view->m_visible && in.length() ) {
      if ( view->m_visible_draw || view->m_need_draw ) {
        
        if ( view->m_last && view->as_clip() ) {
          List<GUITouch> in2;
          
          for ( auto i = in.begin(), e = in.end(); i != e; ) {
            auto j = i++;
            if ( view->overlap_test(Vec2(j.value().x, j.value().y)) ) {
              in2.push(j.value());
              in.del(j);
            }
          }
          
          View* v = view->m_last;
          while( v && in2.length() ) {
            touchstart(v, in2);
            v = v->m_prev;
          }
          
          touchstart_2(view, in2);
          
          if ( in2.length() ) {
            in.push(move(in2));
          }
        } else {
          View* v = view->m_last;
          while( v && in.length() ) {
            touchstart(v, in);
            v = v->m_prev;
          }
          touchstart_2(view, in);
        }
      }
      
    }
  }
  
  /**
   * @func touch_move
   */
  void touchmove(List<GUITouch>& in) {
    Map<PrtKey<View>, Array<GUITouch>> change_touches;
    
    for ( auto& i : in ) {
      GUITouch& in_touch = i.value();
      for ( auto& touches : m_origin_touches ) {
        if ( touches.value()->has(in_touch.id) ) {
          GUITouch& touch = (*touches.value())[in_touch.id];
          touch.x = in_touch.x;
          touch.y = in_touch.y;
          touch.force = in_touch.force;
          if ( !touches.value()->is_click_invalid() ) {
            touch.click_in = touch.view->overlap_test(Vec2(touch.x, touch.y));
            //if ( !touch.is_click_range ) {
            //  XX_DEBUG("Out click range");
            //}
          }
          change_touches[touch.view].push(touch);
          break;
        }
      }
    }
    
    for ( auto& i : change_touches ) {
      
      Array<GUITouch>& touchs = i.value();
      View* view = touchs[0].view;
      // emit event
      _inl_view(view)->bubble_trigger(
         GUI_EVENT_TOUCHMOVE,
         **NewEvent<GUITouchEvent>(view, i.value())
      );
      
      OriginTouche* origin_touche = m_origin_touches[view];
      
      if ( !origin_touche->is_click_invalid() ) { // no invalid
        Vec2 position = OriginTouche::view_position(view);
        Vec2 start_position = origin_touche->view_start_position();
        
        float d = sqrtf(powf((position.x() - start_position.x()), 2) +
                        powf((position.y() - start_position.y()), 2));
        // 视图位置移动超过2取消点击状态
        if ( d > 2 ) { // trigger invalid status
          if ( origin_touche->is_click_down() ) { // trigger style up
            // emit style status event
            auto evt = NewEvent<GUIHighlightedEvent>(view, HOVER_or_NORMAL(view));
            _inl_view(view)->trigger_highlightted(**evt);
          }
          origin_touche->set_click_invalid();
        }
        else { // no invalid
          
          if ( origin_touche->is_click_down() ) { // May trigger click up
            bool trigger_event = true;
            for ( auto& t : origin_touche->values() ) {
              if (t.value().click_in) {
                trigger_event = false; break;
              }
            }
            if ( trigger_event ) {
              origin_touche->set_click_down(false); // set up status
              // emit style status event
              auto evt = NewEvent<GUIHighlightedEvent>(view, HOVER_or_NORMAL(view));
              _inl_view(view)->trigger_highlightted(**evt);
            }
          } else { // May trigger click down
            for ( auto& item : touchs ) {
              if ( item.value().click_in ) { // find range == true
                origin_touche->set_click_down(true); // set down status
                // emit style down event
                auto evt = NewEvent<GUIHighlightedEvent>(view, HIGHLIGHTED_DOWN);
                _inl_view(view)->trigger_highlightted(**evt);
                break;
              }
            }
          }
        } // no invalid end
      } // if end
    } // each end
  }
  
  /**
   * @func touch_end
   */
  void touchend(List<GUITouch>& in, const GUIEventName& type) {
    Map<PrtKey<View>, Array<GUITouch>> change_touches;
    
    for ( auto& i : in ) {
      GUITouch& in_touch = i.value();
      for ( auto& item : m_origin_touches ) {
        if ( item.value()->has(in_touch.id) ) {
          GUITouch& touch = (*item.value())[in_touch.id];
          touch.x = in_touch.x;
          touch.y = in_touch.y;
          touch.force = in_touch.force;
          change_touches[touch.view].push(touch);
          item.value()->del(touch.id); // del touch point
          break;
        }
      }
    }
    
    for ( auto& i : change_touches ) { // views
      Array<GUITouch>& touchs = i.value();
      View* view = touchs[0].view;
      _inl_view(view)->bubble_trigger(type, **NewEvent<GUITouchEvent>(view, touchs)); // emit touch end event
      
      OriginTouche* origin_touche = m_origin_touches[view];
      
      if ( origin_touche->count() == 0 ) {
        if ( origin_touche->is_click_down() ) { // trigger click
          for ( auto& item : touchs ) {
            // find range == true
            if ( item.value().click_in ) {
              // emit style up event
              auto evt = NewEvent<GUIHighlightedEvent>(view, HOVER_or_NORMAL(view));
              _inl_view(view)->trigger_highlightted(**evt);
              
              if ( type == GUI_EVENT_TOUCHEND && view->final_visible() ) {
                auto evt = NewEvent<GUIClickEvent>(view, item.value().x, item.value().y);
                _inl_view(view)->bubble_trigger(GUI_EVENT_CLICK, **evt); // emit click event
              }
              break;
            }
          }
        }
        delete origin_touche;
        m_origin_touches.del(view); // del
      }
      //
    }
  }
  
  //  dispatch event
  
  void dispatch_touchstart(Se& evt) {
    GUILock lock;
    Root* r = app_->root();
    if (r) {
      touchstart(r, *static_cast<List<GUITouch>*>(evt.data));
    }
  }
  
  void dispatch_touchmove(Se& evt) {
    GUILock lock;
    touchmove(*static_cast<List<GUITouch>*>(evt.data));
  }
  
  void dispatch_touchend(Se& evt) {
    GUILock lock;
    touchend(*static_cast<List<GUITouch>*>(evt.data), GUI_EVENT_TOUCHEND);
  }
  
  void dispatch_touchcancel(Se& evt) {
    GUILock lock;
    touchend(*static_cast<List<GUITouch>*>(evt.data), GUI_EVENT_TOUCHCANCEL);
  }

  // -------------------------- keyboard --------------------------

  void dispatch_keyboard_down() {
    KeyboardKeyName name = m_keyboard->keyname();

    View* view = app_->focus_view();
    if ( !view ) view = app_->root();
    if ( view ) {
      View* focus_move = nullptr;
      SelectPanel* panel = nullptr;
      Direction direction = Direction::NONE;
      
      switch ( name ) {
        case KEYCODE_LEFT: direction = Direction::LEFT; break;  // left
        case KEYCODE_UP: direction = Direction::TOP; break;     // top
        case KEYCODE_RIGHT: direction = Direction::RIGHT; break; // right
        case KEYCODE_DOWN: direction = Direction::BOTTOM; break; // bottom
        default: break;
      }
      
      if ( direction != Direction::NONE ) {
        Button* button = view->as_button();
        if ( button ) {
          if ( (panel = button->panel()) && panel->enable_select() ) {
            focus_move = button->find_next_button(direction);
          }
        }
      }
      
      auto evt = NewEvent<GUIKeyEvent>(view, name,
        m_keyboard->shift(),
        m_keyboard->ctrl(), m_keyboard->alt(),
        m_keyboard->command(), m_keyboard->caps_lock(),
        m_keyboard->repeat(), m_keyboard->device(), m_keyboard->source()
      );
      
      evt->set_focus_move(focus_move);
      
      _inl_view(view)->bubble_trigger(GUI_EVENT_KEYDOWN, **evt);
      
      if ( evt->is_default() ) {
        
        if ( name == KEYCODE_ENTER ) {
          _inl_view(view)->bubble_trigger(GUI_EVENT_KEYENTER, **evt);
        } else if ( name == KEYCODE_VOLUME_UP ) {
          _inl_app(app_)->set_volume_up();
        } else if ( name == KEYCODE_VOLUME_DOWN ) {
          _inl_app(app_)->set_volume_down();
        }
        
        int keypress_code = m_keyboard->keypress();
        if ( keypress_code ) { // keypress
          evt->set_keycode( keypress_code );
          _inl_view(view)->bubble_trigger(GUI_EVENT_KEYPRESS, **evt);
        }

        if ( name == KEYCODE_CENTER && m_keyboard->repeat() == 0 ) {
          CGRect rect = view->screen_rect();
          auto evt = NewEvent<GUIHighlightedEvent>(view, HIGHLIGHTED_DOWN);
          _inl_view(view)->trigger_highlightted(**evt); // emit click status event
        }
        
        if ( evt->focus_move() ) {
          evt->focus_move()->focus();
        }
        
      } // if ( evt->is_default() ) {
    } // if ( view )
  }
  
  void dispatch_keyboard_up() {
    
    KeyboardKeyName name = m_keyboard->keyname();
    View* view = app_->focus_view();
    if ( !view ) view = app_->root();
    if ( view ) {
      auto evt = NewEvent<GUIKeyEvent>(view, name,
        m_keyboard->shift(),
        m_keyboard->ctrl(), m_keyboard->alt(),
        m_keyboard->command(), m_keyboard->caps_lock(),
        m_keyboard->repeat(), m_keyboard->device(), m_keyboard->source()
      );
      
      _inl_view(view)->bubble_trigger(GUI_EVENT_KEYUP, **evt);
      
      if ( evt->is_default() ) {
        if ( name == KEYCODE_BACK ) {
          CGRect rect = view->screen_rect();
          auto evt = NewEvent<GUIClickEvent>(view, rect.origin.x() + rect.size.x() / 2,
                                              rect.origin.y() + rect.size.y() / 2, true);
          _inl_view(view)->bubble_trigger(GUI_EVENT_BACK, **evt); // emit back
          
          if ( evt->is_default() ) {
            // pending gui application (挂起应用)
            app_->pending();
          }
        }
        else if ( name == KEYCODE_CENTER ) {
          auto evt = NewEvent<GUIHighlightedEvent>(view, HIGHLIGHTED_HOVER);
          _inl_view(view)->trigger_highlightted(**evt); // emit style status event
          
          CGRect rect = view->screen_rect();
          auto evt2 = NewEvent<GUIClickEvent>(view, rect.origin.x() + rect.size.x() / 2,
                                             rect.origin.y() + rect.size.y() / 2, true);
          _inl_view(view)->bubble_trigger(GUI_EVENT_CLICK, **evt2);
        } // 
      }
    }
  }
  
};

GUIEventDispatch::GUIEventDispatch(GUIApplication* app): app_(app), m_text_input(nullptr) {
  m_keyboard = KeyboardAdapter::create();
}

GUIEventDispatch::~GUIEventDispatch() {
  for (auto& i : m_origin_touches) {
    delete i.value();
  }
  Release(m_keyboard);
}

#define _loop static_cast<PostMessage*>(app_->main_loop())

void KeyboardAdapter::dispatch(int keycode, bool ascii,
                               bool down, int repeat, int device, int source) {
  async_callback(Cb([=](Se& evt) {
    GUILock lock;
    repeat_ = repeat; device_ = device;
    source_ = source;
    
    bool clear = transformation(keycode, ascii, down);
    
    if ( down ) {
      _inl_di(_inl_app(app_)->dispatch())->dispatch_keyboard_down();
    } else {
      _inl_di(_inl_app(app_)->dispatch())->dispatch_keyboard_up();
    }
    
    if ( clear ) {
      shift_ = alt_ = false;
      ctrl_ = command_ = false;
    }
  }), _loop);
}

void GUIEventDispatch::dispatch_touchstart(List<GUITouch>&& list) {
  async_callback(Cb(&Inl::dispatch_touchstart, _inl_di(this)), move(list), _loop);
}

void GUIEventDispatch::dispatch_touchmove(List<GUITouch>&& list) {
  async_callback(Cb(&Inl::dispatch_touchmove, _inl_di(this)), move(list), _loop);
}

void GUIEventDispatch::dispatch_touchend(List<GUITouch>&& list) {
  async_callback(Cb(&Inl::dispatch_touchend, _inl_di(this)), move(list), _loop);
}

void GUIEventDispatch::dispatch_touchcancel(List<GUITouch>&& list) {
  async_callback(Cb(&Inl::dispatch_touchcancel, _inl_di(this)), move(list), _loop);
}

void GUIEventDispatch::dispatch_ime_delete(int count) {
  async_callback(Cb([=](Se& d) {
    GUILock lock;
    if ( m_text_input ) {
      m_text_input->input_delete_text(count);
      bool can_backspace = m_text_input->input_can_backspace();
      bool can_delete = m_text_input->input_can_delete();
      _inl_app(app_)->ime_keyboard_can_backspace(can_backspace, can_delete);
    }
  }), _loop);
}

void GUIEventDispatch::dispatch_ime_insert(cString& text) {
  async_callback(Cb([=](Se& d) {
    GUILock lock;
    if ( m_text_input ) {
      m_text_input->input_insert_text(text);
    }
  }), _loop);
}

void GUIEventDispatch::dispatch_ime_marked(cString& text) {
  async_callback(Cb([=](Se& d) {
    GUILock lock;
    if ( m_text_input ) {
      m_text_input->input_marked_text(text);
    }
  }), _loop);
}

void GUIEventDispatch::dispatch_ime_unmark(cString& text) {
  async_callback(Cb([=](Se& d) {
    GUILock lock;
    if ( m_text_input ) {
      m_text_input->input_unmark_text(text);
    }
  }), _loop);
}

/**
 * @func make_text_input
 */
void GUIEventDispatch::make_text_input(TextInputProtocol* input) {
  if ( input != m_text_input ) {
    // TextInput* old = m_text_input;
    m_text_input = input;
    
    if ( input ) {
      _inl_app(app_)->ime_keyboard_open({
        true, input->keyboard_type(), input->keyboard_return_type()
      });
    } else {
      _inl_app(app_)->ime_keyboard_close();
    }
  } else {
    if ( input ) {
      _inl_app(app_)->ime_keyboard_open({
        false, input->keyboard_type(), input->keyboard_return_type()
      });
    }
  }
}

XX_END
