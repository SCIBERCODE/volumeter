
// bug: самовозбуд стейнберга при выкрученной громкости на lpf 15000

class component_filter_ : public Component {
//=================================================================================================
private:
    const int _order_list[14] { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200, 300, 500 };

    theme::checkbox_right_tick_      theme_right;
    GroupComponent                   group        { { }, L"Filter(s)" };
    std::unique_ptr<ToggleButton>    checkbox_type[FILTER_TYPE_SIZE];
    std::unique_ptr<Label>           label_desc   [FILTER_TYPE_SIZE];
    std::unique_ptr<TextEditor>      edit_freq    [FILTER_TYPE_SIZE];
    std::unique_ptr<ComboBox>        combo_order  [FILTER_TYPE_SIZE];

    signal_                        & _signal;
    application_                   & _app;
//=================================================================================================
public:

    component_filter_(application_& app, signal_& signal) :
        _signal(signal), _app(app)
    // todo: component_filter_(shared_ptr<signal_> signal) : signal(move(signal))
    {
        addAndMakeVisible(group);

        for (auto type = HIGH_PASS; type < FILTER_TYPE_SIZE; type++)
        {
            checkbox_type[type] = std::make_unique<ToggleButton>(type == HIGH_PASS ? L"High pass" : L"Low pass");
            label_desc   [type] = std::make_unique<Label>();
            edit_freq    [type] = std::make_unique<TextEditor>();
            combo_order  [type] = std::make_unique<ComboBox>();

            auto button = checkbox_type[type].get();
            auto label  = label_desc   [type].get();
            auto edit   = edit_freq    [type].get();
            auto combo  = combo_order  [type].get();

            const auto option       = type == HIGH_PASS ? option_t::pass_high       : option_t::pass_low;
            const auto option_freq  = type == HIGH_PASS ? option_t::pass_high_freq  : option_t::pass_low_freq;
            const auto option_order = type == HIGH_PASS ? option_t::pass_high_order : option_t::pass_low_order;

            addAndMakeVisible(button);
            addAndMakeVisible(label);
            addAndMakeVisible(edit);
            addAndMakeVisible(combo);

            // галка включения фильтра
            button->setToggleState(_app.get_int(option), dontSendNotification);
            button->setLookAndFeel(&theme_right);
            button->onClick = [&, type, option]
            {
                _app.save(option, checkbox_type[type]->getToggleState());
                _signal.filter_init(type);
            };
            // поле для ввода частоты
            label->setText("Freq/Order", dontSendNotification);
            label->setJustificationType(Justification::centredRight);

            auto check_input = [this](ToggleButton *check, String value_text)
            {
                auto value_int = value_text.getIntValue();
                auto enabled = value_int > 0 && value_int <= _app.get_int(option_t::sample_rate) / 2;
                check->setEnabled(enabled);
                check->setTooltip(enabled ? L"" : L"The frequency value must be entered!");
                if (!enabled) check->setToggleState(false, sendNotification);
                return enabled;
            };

            edit->onTextChange = [this, type, option_freq, check_input]
            {
                auto value = edit_freq[type]->getText();
                if (check_input(checkbox_type[type].get(), value) || value.isEmpty()) {
                    _app.save(option_freq, value);
                    _signal.filter_init(type);
                }
            };

            edit->setInputRestrictions(0, L"0123456789.");
            auto value = _app.get_text(option_freq);
            if (check_input(button, value))
                edit->setText(value, true);

            // порядок фильтра
            for (auto const item : _order_list)
                combo->addItem(String(item), item);

            combo->setSelectedId(_app.get_int(option_order), dontSendNotification);
            combo->onChange = [&, type, option]
            {
                auto value = combo_order[type]->getSelectedId();
                _app.save(option_order, value);
                _signal.set_order(type, value);
                _signal.filter_init(type);
            };
        }
    }

    //=============================================================================================
    // juce callbacks

    void resized() override {
        //=========================================================================================
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

    void prepare_to_play(const double sample_rate) {
        //=========================================================================================
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

