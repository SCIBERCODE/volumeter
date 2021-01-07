#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_>     __opt;
extern unique_ptr<theme::light_> __theme;

// bug: самовозбуд стейнберга при выкрученной громкости на lpf 15000

class component_filter_ : public Component
{
protected:
    const vector<int> order_list { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200, 300, 500 };
    const pair<const wchar_t *, const wchar_t *> type_text[FILTER_TYPE_SIZE] =
    {
        make_pair(L"High pass", L"pass_high"),
        make_pair(L"Low pass" , L"pass_low" )
    };
private:
    signal_                   & signal;
    theme::checkbox_right_tick_ theme_right;
    GroupComponent              group { { }, L"Filter(s)" };
    unique_ptr<ToggleButton>    checkbox_type[FILTER_TYPE_SIZE];
    unique_ptr<Label>           label_desc   [FILTER_TYPE_SIZE];
    unique_ptr<TextEditor>      edit_freq    [FILTER_TYPE_SIZE];
    unique_ptr<ComboBox>        combo_order  [FILTER_TYPE_SIZE];

public:

    component_filter_(signal_& signal) : signal(signal)
    // todo: component_filter_(shared_ptr<signal_> signal) : signal(move(signal))
    {
        addAndMakeVisible(group);

        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            checkbox_type[type] = make_unique<ToggleButton>(type_text[type].first);
            label_desc   [type] = make_unique<Label>();
            edit_freq    [type] = make_unique<TextEditor>();
            combo_order  [type] = make_unique<ComboBox>();

            auto button = checkbox_type[type].get();
            auto label  = label_desc   [type].get();
            auto edit   = edit_freq    [type].get();
            auto combo  = combo_order  [type].get();

            auto option = String(type_text[type].second);

            addAndMakeVisible(button);
            addAndMakeVisible(label);
            addAndMakeVisible(edit);
            addAndMakeVisible(combo);

            // галка включения фильтра
            button->setToggleState(__opt->get_int(option), dontSendNotification);
            button->setLookAndFeel(&theme_right);
            button->onClick = [&, type, option]
            {
                __opt->save(option, checkbox_type[type]->getToggleState());
                signal.filter_init(type);
            };
            // поле для ввода частоты
            label->setText("Freq/Order", dontSendNotification);
            label->setJustificationType(Justification::centredRight);

            auto check_input = [&](ToggleButton *check, String value_str)
            {
                auto value_int = value_str.getIntValue();
                auto enabled = value_int > 0 && value_int <= __opt->get_int(L"sample_rate") / 2;
                check->setEnabled(enabled);
                if (!enabled) check->setToggleState(false, sendNotification);
                return enabled;
            };

            edit->onTextChange = [&, type, option]
            {
                auto value = edit_freq[type]->getText();
                if (check_input(checkbox_type[type].get(), value) || value.isEmpty()) {
                    __opt->save(option + L"_freq", value);
                    signal.filter_init(type);
                }
            };

            edit->setInputRestrictions(0, L"0123456789.");
            auto value = __opt->get_text(option + L"_freq");
            if (check_input(button, value))
                edit->setText(value, true);

            // порядок фильтра
            for (auto const item : order_list)
                combo->addItem(String(item), item);

            combo->setSelectedId(__opt->get_int(option + L"_order"), dontSendNotification);
            combo->onChange = [&, type, option]
            {
                auto value = combo_order[type]->getSelectedId();
                __opt->save(option + L"_order", value);
                signal.set_order(type, value);
                signal.filter_init(type);
            };
        }
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(theme::margin);
        area.removeFromTop(theme::height + theme::margin);
        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            auto line = area.removeFromTop(theme::height);
            line.removeFromRight(theme::margin);
            combo_order[type]->setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            checkbox_type[type]->setBounds(line.removeFromLeft(theme::button_width));
            line.removeFromLeft(theme::margin);
            label_desc[type]->setBounds(line.removeFromLeft(theme::label_width));
            line.removeFromLeft(theme::margin);
            edit_freq[type]->setBounds(line);
            area.removeFromTop(theme::margin);
        }
        group.setBounds(getLocalBounds());
    }

    void prepare_to_play(const double sample_rate)
    {
        auto max_freq = sample_rate / 2;
        auto empty_message = "1 .. " + String(max_freq, 0) + L" Hz";
        for (auto& edit : edit_freq)
        {
            edit->setTextToShowWhenEmpty(empty_message, Colours::grey);
            edit->repaint();
            auto value = edit->getText().getIntValue();
            if (value > max_freq)
                edit->setText(String(max_freq), true);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_filter_)

};
