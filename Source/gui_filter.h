
// B[07]

class component_filter : public Component {
//=================================================================================================
private:
    const int _order_list[10] { 1, 2, 4, 10, 20, 30, 40, 60, 80, 100 };

    signal                        & _signal;
    application                   & _app;
    theme::checkbox_right_tick      _theme_right;

    GroupComponent                   group          { { }, L"Filter(s)"    };
    std::unique_ptr<ToggleButton>    checkboxes_type[+filter_type_t::__size];
    std::unique_ptr<Label>           labels_desc    [+filter_type_t::__size];
    std::unique_ptr<TextEditor>      edits_freq     [+filter_type_t::__size];
    std::unique_ptr<ComboBox>        combos_order   [+filter_type_t::__size];
//=================================================================================================
public:

    component_filter(application& app, signal& signal) :
        _signal(signal), _app(app)
        // T[13]
    {
        addAndMakeVisible(group);

        for (auto const& type : filter_type_t())
        {
            checkboxes_type[+type] = std::make_unique<ToggleButton>(type == filter_type_t::HIGH_PASS ? L"High pass" : L"Low pass");
            labels_desc    [+type] = std::make_unique<Label>();
            edits_freq     [+type] = std::make_unique<TextEditor>();
            combos_order   [+type] = std::make_unique<ComboBox>();

            auto button = checkboxes_type[+type].get();
            auto label  = labels_desc    [+type].get();
            auto edit   = edits_freq     [+type].get();
            auto combo  = combos_order   [+type].get();

            const auto option       = type == filter_type_t::HIGH_PASS ? option_t::pass_high       : option_t::pass_low;
            const auto option_freq  = type == filter_type_t::HIGH_PASS ? option_t::pass_high_freq  : option_t::pass_low_freq;
            const auto option_order = type == filter_type_t::HIGH_PASS ? option_t::pass_high_order : option_t::pass_low_order;

            addAndMakeVisible(button);
            addAndMakeVisible(label);
            addAndMakeVisible(edit);
            addAndMakeVisible(combo);

            // галка включения фильтра
            button->setToggleState(_app.get_int(option), dontSendNotification);
            button->setLookAndFeel(&_theme_right);
            button->onClick = [&, type, option] // T[12]
            {
                _app.save(option, checkboxes_type[+type]->getToggleState());
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

            edit->onTextChange = [this, type, option_freq, check_input] // T[12]
            {
                auto value = edits_freq[+type]->getText();
                if (check_input(checkboxes_type[+type].get(), value) || value.isEmpty()) {
                    _app.save(option_freq, value);
                    _signal.filter_init(type);
                }
            };

            edit->setInputRestrictions(0, L"0123456789.");
            auto value = _app.get_text(option_freq);
            if (check_input(button, value))
                edit->setText(value, true);

            // filter order
            for (auto const item : _order_list)
                combo->addItem(String(item), item);

            combo->setSelectedId(_app.get_int(option_order), dontSendNotification);
            combo->onChange = [this, type, option_order]
            {
                auto value = combos_order[+type]->getSelectedId();
                _app.save(option_order, value);
                _signal.set_order(type, value);
                _signal.filter_init(type);
            };
        }
    }

    ~component_filter() { }

    //=============================================================================================
    // juce callbacks

    void resized() override {
        //=========================================================================================
        auto area = getLocalBounds().reduced(theme::margin);
        area.removeFromTop(theme::height + theme::margin);
        for (auto const& type : filter_type_t())
        {
            auto line = area.removeFromTop(theme::height);
            line.removeFromRight(theme::margin);
            combos_order[+type]->setBounds(line.removeFromRight(theme::button_width));
            line.removeFromRight(theme::margin);
            checkboxes_type[+type]->setBounds(line.removeFromLeft(theme::button_width));
            line.removeFromLeft(theme::margin);
            labels_desc[+type]->setBounds(line.removeFromLeft(theme::label_width));
            line.removeFromLeft(theme::margin);
            edits_freq[+type]->setBounds(line);
            area.removeFromTop(theme::margin);
        }
        group.setBounds(getLocalBounds());
    }

    void prepare_to_play(const double sample_rate) {
        //=========================================================================================
        auto max_freq      = sample_rate / 2;
        auto empty_message = L"1 .. " + String(max_freq, 0) + L" Hz";

        for (auto& edit : edits_freq)
        {
            edit->setTextToShowWhenEmpty(empty_message, Colours::grey);
            edit->repaint();
            auto value = edit->getText().getIntValue();
            if (value > max_freq)
                edit->setText(String(max_freq), true);
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_filter)
};

