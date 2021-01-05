#pragma once
#include <JuceHeader.h>
#include <memory>
#include "settings.h"

#define _USE_MATH_DEFINES
#include <math.h>

const auto MF_PI   = static_cast<float>(M_PI);
const auto MF_PI_2 = static_cast<float>(M_PI_2);

const map<int, String> __prefs {
    { -15, L"f" },
    { -12, L"p" },
    {  -9, L"n" },
    {  -6, L"μ" },
    {  -3, L"m" },
    {   0, L""  },
    {   3, L"k" },
    {   6, L"M" }
};

enum volume_t : size_t {
    LEFT = 0,
    RIGHT,
    BALANCE,
    VOLUME_SIZE
};

const map<volume_t, String> __stat_captions {
    { LEFT,    L"Left"    },
    { RIGHT,   L"Right"   },
    { BALANCE, L"Balance" }
};

enum extremum_t : size_t {
    MIN = 0,
    MAX,
    EXTREMES_SIZE
};

enum filter_type_t : size_t {
    HIGH_PASS = 0,
    LOW_PASS,
    FILTER_TYPE_SIZE
};

enum labels_stat_column_t : size_t {
    LABEL = 0,
    VALUE,
    EXTREMES,
    LABELS_STAT_COLUMN_SIZE
};

template <typename T>
void operator ++(T& value, int) // todo: проверить на вызовы с другими типами помимо перечислений, описанных выше
{
    value = static_cast<T>(value + 1);
}

void operator --(volume_t& value, int)
{
    value = value == 0 ? VOLUME_SIZE : static_cast<volume_t>(value - 1); // todo: странные дела, лучше от этого избавиться
}

String prefix  (double value, const wchar_t *unit, size_t numder_of_decimals);
String prefix_v(double value);

bool is_about_equal(float a, float b);
bool round_flt     (float value);

class main_component_;

class application_ : public JUCEApplication
{
public:
    application_();
    ~application_() override;

    const String getApplicationName() override;
    const String getApplicationVersion() override;
    void  shutdown() override;
    void  initialise(const String&) override;
    bool  moreThanOneInstanceAllowed() override;

protected:
    class main_window_ : public DocumentWindow
    {
    public:
        main_window_(const String& name, JUCEApplication& a);
        void closeButtonPressed() override;

    private:
        JUCEApplication&            _app;
        unique_ptr<main_component_> _content;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (main_window_)
    };

private:
    unique_ptr<main_window_> _main_window;

};
