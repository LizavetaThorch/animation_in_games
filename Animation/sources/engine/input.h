#pragma once
#include <map>
#include <functional>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_events.h>



class Input
{
  std::map<SDL_Keycode, bool> keyMap;

public:

  template<typename T>
  struct Event
  {
    using Delegate = std::function<void (const T &)>;
    std::vector<Delegate> callbacks;

    Event &operator+=(Delegate &&delegate)
    {
      callbacks.emplace_back(std::move(delegate));
      return *this;
    }

    void operator()(const T & event) const
    {
      for (const Delegate& delegate : callbacks)
        delegate(event);
    }
  };

  Event<SDL_KeyboardEvent> onKeyboardEvent;
  Event<SDL_MouseButtonEvent> onMouseButtonEvent;
  Event<SDL_MouseMotionEvent> onMouseMotionEvent;
  Event<SDL_MouseWheelEvent> onMouseWheelEvent;

  void event_process(const SDL_KeyboardEvent &event)
  {
    onKeyboardEvent(event);
    SDL_Keycode key = event.keysym.sym;
    if (!event.repeat)
    {
      if (event.state == SDL_PRESSED)
        keyMap[key] = true;
      if (event.state == SDL_RELEASED)
        keyMap[key] = false;
    }
  }
  void event_process(const SDL_MouseButtonEvent &event) { onMouseButtonEvent(event); }
  void event_process(const SDL_MouseMotionEvent &event) { onMouseMotionEvent(event); }
  void event_process(const SDL_MouseWheelEvent &event) { onMouseWheelEvent(event); }

  float get_key(SDL_Keycode keycode)
  {
    auto it = keyMap.find(keycode);
    return (it != keyMap.end() ? it->second : false) ? 1.f : 0.f;
  }
};

extern Input input;