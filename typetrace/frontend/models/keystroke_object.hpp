#ifndef TYPETRACE_KEYSTROKE_OBJECT_HPP
#define TYPETRACE_KEYSTROKE_OBJECT_HPP

#include "types.hpp"

#include <glibmm/object.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

namespace typetrace::frontend::models {

class KeystrokeObject : public Glib::Object
{
  public:
    KeystrokeObject(const KeystrokeEvent& event)
        : Glib::ObjectBase(typeid(KeystrokeObject)),
          m_key_code(event.key_code),
          m_count(event.count),
          m_key_name(event.key_name),
          m_date(event.date)
    {}

    static Glib::RefPtr<KeystrokeObject> create(const KeystrokeEvent& event)
    {
        return Glib::make_refptr_for_instance<KeystrokeObject>(new KeystrokeObject(event));
    }

    [[nodiscard]]
    auto get_key_code() const -> int
    {
        return static_cast<int>(m_key_code);
    }

    [[nodiscard]]
    auto get_count() const -> int
    {
        return m_count;
    }

    [[nodiscard]]
    auto get_key_name() const -> Glib::ustring
    {
        return m_key_name;
    }

    [[nodiscard]]
    auto get_date() const -> Glib::ustring
    {
        return m_date;
    }

  private:
    std::size_t m_key_code;
    int m_count;
    Glib::ustring m_key_name;
    Glib::ustring m_date;
};

} // namespace typetrace::frontend::models

#endif // TYPETRACE_KEYSTROKE_OBJECT_HPP
