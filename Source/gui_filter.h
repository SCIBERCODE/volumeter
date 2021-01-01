#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> _opt;

class filter_component_ : public Component
{
protected:
    const vector<int> order_list = { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200, 300, 500 };
    const array<pair<const wchar_t *, const wchar_t *>, FILTER_TYPE_SIZE> type_text =
    {
        make_pair(L"High pass", L"pass_high"),
        make_pair(L"Low pass" , L"pass_low" )
    };

public:

    filter_component_(signal_& signal) : signal(signal)
    // todo: filter_component_(shared_ptr<signal_> signal) : signal(move(signal))
    {
        //addAndMakeVisible(slider_freq); // todo: добавить какую-нибудь графику

        addAndMakeVisible(group);
        addAndMakeVisible(label_filter);
        label_filter.setColour(Label::backgroundColourId, _theme->get_bg_color());

        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            checkbox_type.at(type) = make_unique<ToggleButton>(type_text.at(type).first);
            label_desc   .at(type) = make_unique<Label>();
            edit_freq    .at(type) = make_unique<TextEditor>();
            combo_order  .at(type) = make_unique<ComboBox>();

            auto button = checkbox_type.at(type).get();
            auto label  = label_desc   .at(type).get();
            auto edit   = edit_freq    .at(type).get();
            auto combo  = combo_order  .at(type).get();

            auto option = String(type_text.at(type).second);

            addAndMakeVisible(button);
            addAndMakeVisible(label);
            addAndMakeVisible(edit);
            addAndMakeVisible(combo);

            // галка включения фильтра
            button->setToggleState(_opt->get_int(option), dontSendNotification);
            button->setLookAndFeel(&theme_right);
            button->onClick = [&, type, option]
            {
                _opt->save(option, checkbox_type.at(type)->getToggleState());
                signal.filter_init(type);
            };
            // поле для ввода частоты
            label->setText("Freq/Order", dontSendNotification);
            label->setJustificationType(Justification::centredRight);

            auto check_input = [&](ToggleButton *check, String value_str)
            {
                auto value_int = value_str.getIntValue();
                auto enabled = value_int > 0 && value_int <= _opt->get_int(L"sample_rate") / 2;
                check->setEnabled(enabled);
                if (!enabled) check->setToggleState(false, sendNotification);
                return enabled;
            };

            edit->onTextChange = [&, type, option]
            {
                auto value = edit_freq.at(type)->getText();
                if (check_input(checkbox_type.at(type).get(), value) || value.isEmpty()) {
                    _opt->save(option + L"_freq", value);
                    signal.filter_init(type);
                }
            };

            edit->setInputRestrictions(0, L"0123456789.");
            auto value = _opt->get_text(option + L"_freq");
            if (check_input(button, value))
                edit->setText(value, true);

            // выпадайка с порядком фильтра
            for (auto const item : order_list)
                combo->addItem(String(item), item);

            combo->setSelectedId(_opt->get_int(option + L"_order"), dontSendNotification);
            combo->onChange = [&, type, option]
            {
                auto value = combo_order.at(type)->getSelectedId();
                _opt->save(option + L"_order", value);
                signal.set_order(type, value);
                signal.filter_init(type);
            };
        }
    }

    ~filter_component_() { }

    void resized() override
    {
        auto area = getLocalBounds().reduced(theme::margin);
        area.removeFromTop(theme::height + theme::margin);
        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            auto line = area.removeFromTop(theme::height);
            line.removeFromRight(theme::margin);
            combo_order.at(type)->setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            edit_freq.at(type)->setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            label_desc.at(type)->setBounds(line.removeFromRight(theme::label_width));
            line.removeFromRight(theme::margin);
            checkbox_type.at(type)->setBounds(line);
            area.removeFromTop(theme::margin);
        }
        group.setBounds(getLocalBounds());
        _theme->set_header_label_bounds(label_filter);
    }

    void prepare_to_play(double sample_rate)
    {
        auto max_freq = sample_rate / 2;
        auto empty_message = "1 .. " + String(max_freq) + L" Hz";
        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            auto edit = edit_freq.at(type).get();
            edit->setTextToShowWhenEmpty(empty_message, Colours::grey);
            auto value = edit->getText().getIntValue();
            if (value > max_freq)
                edit->setText(String(max_freq), true);
        }
    }

private:
    signal_                 & signal;
    theme::checkbox_right_lf_ theme_right;
    Label                     label_filter { { }, L"Filter(s)" };
    GroupComponent            group;

    array<unique_ptr<ToggleButton>, FILTER_TYPE_SIZE> checkbox_type;
    array<unique_ptr<Label>,        FILTER_TYPE_SIZE> label_desc;
    array<unique_ptr<TextEditor>,   FILTER_TYPE_SIZE> edit_freq;
    array<unique_ptr<ComboBox>,     FILTER_TYPE_SIZE> combo_order;
};
