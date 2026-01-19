#ifndef TYPETRACE_COLOR_UTILS_HPP
#define TYPETRACE_COLOR_UTILS_HPP

#include <cmath>
#include <format>
#include <gdkmm/rgba.h>
#include <gio/gio.h>
#include <glibmm/ustring.h>
#include <string>
#include <tuple>

namespace typetrace::frontend::utils {

struct Rgb
{
    float r, g, b;
};

struct Hsv
{
    float h, s, v;
};

[[nodiscard]]
inline auto hsv_to_rgb(Hsv hsv) -> Rgb
{
    float c = hsv.v * hsv.s;
    float x = c * (1 - std::abs(std::fmod(hsv.h * 6, 2.0f) - 1));
    float m = hsv.v - c;

    float r_prime = 0, g_prime = 0, b_prime = 0;

    if (hsv.h >= 0 && hsv.h < 1.0f / 6.0f) {
        r_prime = c;
        g_prime = x;
    } else if (hsv.h >= 1.0f / 6.0f && hsv.h < 2.0f / 6.0f) {
        r_prime = x;
        g_prime = c;
    } else if (hsv.h >= 2.0f / 6.0f && hsv.h < 3.0f / 6.0f) {
        g_prime = c;
        b_prime = x;
    } else if (hsv.h >= 3.0f / 6.0f && hsv.h < 4.0f / 6.0f) {
        g_prime = x;
        b_prime = c;
    } else if (hsv.h >= 4.0f / 6.0f && hsv.h < 5.0f / 6.0f) {
        r_prime = x;
        b_prime = c;
    } else if (hsv.h >= 5.0f / 6.0f && hsv.h < 1.0f) {
        r_prime = c;
        b_prime = x;
    }

    return {r_prime + m, g_prime + m, b_prime + m};
}

[[nodiscard]]
inline auto rgb_to_hsv(Rgb rgb) -> Hsv
{
    float r = rgb.r;
    float g = rgb.g;
    float b = rgb.b;

    float c_max = std::max({r, g, b});
    float c_min = std::min({r, g, b});
    float delta = c_max - c_min;

    float h = 0;
    if (delta == 0) {
        h = 0;
    } else if (c_max == r) {
        h = std::fmod((g - b) / delta, 6.0f);
    } else if (c_max == g) {
        h = ((b - r) / delta) + 2;
    } else {
        h = ((r - g) / delta) + 4;
    }
    h /= 6.0f;
    if (h < 0) {
        h += 1.0f;
    }

    float s = (c_max == 0) ? 0 : delta / c_max;
    float v = c_max;

    return {h, s, v};
}

[[nodiscard]]
inline auto parse_color_string(const std::string& color_str) -> Gdk::RGBA
{
    Gdk::RGBA rgba;
    if (color_str.rfind("rgb(", 0) == 0) {
        // Simple parsing for rgb(r,g,b)
        int r = 0, g = 0, b = 0;
        if (sscanf(color_str.c_str(), "rgb(%d,%d,%d)", &r, &g, &b) == 3) {
            rgba.set_rgba(r / 255.0, g / 255.0, b / 255.0, 1.0);
        } else {
            rgba.set_rgba(0, 0, 1.0, 1.0); // Default blue
        }
    } else {
        try {
            rgba.set(color_str);
        }
        catch (...) {
            rgba.set("blue");
        }
    }
    return rgba;
}

class HeatmapColorScheme
{
  public:
    static constexpr float LUMINANCE_THRESHOLD = 0.5f;

    explicit HeatmapColorScheme(Glib::RefPtr<Gio::Settings> settings)
        : m_settings(std::move(settings))
    {}

    virtual ~HeatmapColorScheme() = default;

    virtual auto get_color_gradient() -> std::pair<Rgb, Rgb> = 0;

    auto calculate_color_for_key(float normalized_count) -> std::pair<std::string, std::string>
    {
        auto [beg_color, end_color] = get_color_gradient();

        float r = beg_color.r + normalized_count * (end_color.r - beg_color.r);
        float g = beg_color.g + normalized_count * (end_color.g - beg_color.g);
        float b = beg_color.b + normalized_count * (end_color.b - beg_color.b);

        int r_int = static_cast<int>(r * 255);
        int g_int = static_cast<int>(g * 255);
        int b_int = static_cast<int>(b * 255);

        std::string bg_color = std::format("rgb({}, {}, {})", r_int, g_int, b_int);
        float luminance = 0.3f * r + 0.6f * g + 0.1f * b;
        std::string text_color = (luminance < LUMINANCE_THRESHOLD) ? "white" : "black";

        return {bg_color, text_color};
    }

    auto get_gradient_css() -> std::string
    {
        auto [beg_color, end_color] = get_color_gradient();
        int beg_r = static_cast<int>(beg_color.r * 255);
        int beg_g = static_cast<int>(beg_color.g * 255);
        int beg_b = static_cast<int>(beg_color.b * 255);

        int end_r = static_cast<int>(end_color.r * 255);
        int end_g = static_cast<int>(end_color.g * 255);
        int end_b = static_cast<int>(end_color.b * 255);

        return std::format(
          ".gradient-bar {{\n" "    background: linear-gradient(to right, rgb({}, {}, {}), rgb({}, {}, {}));\n" "}}",
          beg_r,
          beg_g,
          beg_b,
          end_r,
          end_g,
          end_b);
    }

  protected:
    Glib::RefPtr<Gio::Settings> m_settings;
};

class MultiColorHeatmap : public HeatmapColorScheme
{
  public:
    using HeatmapColorScheme::HeatmapColorScheme;

    auto get_color_gradient() -> std::pair<Rgb, Rgb> override
    {
        auto beg_rgba = parse_color_string(m_settings->get_string("heatmap-begin-color"));
        auto end_rgba = parse_color_string(m_settings->get_string("heatmap-end-color"));

        Rgb beg_color{static_cast<float>(beg_rgba.get_red()),
                      static_cast<float>(beg_rgba.get_green()),
                      static_cast<float>(beg_rgba.get_blue())};
        Rgb end_color{static_cast<float>(end_rgba.get_red()),
                      static_cast<float>(end_rgba.get_green()),
                      static_cast<float>(end_rgba.get_blue())};

        if (m_settings->get_boolean("reverse-heatmap-gradient")) {
            return {end_color, beg_color};
        }
        return {beg_color, end_color};
    }
};

class SingleColorHeatmap : public HeatmapColorScheme
{
  public:
    using HeatmapColorScheme::HeatmapColorScheme;

    auto get_color_gradient() -> std::pair<Rgb, Rgb> override
    {
        auto beg_rgba = parse_color_string(m_settings->get_string("heatmap-single-color"));
        return generate_gradient_from_color(beg_rgba);
    }

  protected:
    auto generate_gradient_from_color(Gdk::RGBA color_rgba) -> std::pair<Rgb, Rgb>
    {
        Rgb rgb{static_cast<float>(color_rgba.get_red()),
                static_cast<float>(color_rgba.get_green()),
                static_cast<float>(color_rgba.get_blue())};
        Hsv hsv = rgb_to_hsv(rgb);

        // Lighter version
        float light_s = std::max(0.2f, hsv.s * 0.6f);
        float light_v = std::min(1.0f, hsv.v * 1.5f);
        Rgb beg_color = hsv_to_rgb({hsv.h, light_s, light_v});

        // Darker version
        float dark_s = std::min(1.0f, hsv.s * 1.5f);
        float dark_v = std::max(0.15f, hsv.v * 0.45f);
        Rgb end_color = hsv_to_rgb({hsv.h, dark_s, dark_v});

        if (m_settings->get_boolean("reverse-heatmap-gradient")) {
            return {end_color, beg_color};
        }
        return {beg_color, end_color};
    }
};

[[nodiscard]]
inline auto get_color_scheme(Glib::RefPtr<Gio::Settings> settings)
  -> std::unique_ptr<HeatmapColorScheme>
{
    bool use_single_color = settings->get_boolean("use-single-color-heatmap");
    // bool use_accent_color = settings->get_boolean("use-accent-color");

    if (use_single_color) {
        return std::make_unique<SingleColorHeatmap>(settings);
    }
    return std::make_unique<MultiColorHeatmap>(settings);
}

} // namespace typetrace::frontend::utils

#endif // TYPETRACE_COLOR_UTILS_HPP
