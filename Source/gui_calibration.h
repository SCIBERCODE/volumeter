
class component_calibration : public Component,
                              public TableListBoxModel {
//=================================================================================================
private:
    enum class cell_data_t { name, left, right, button };

    struct column_t
    {
        const cell_data_t type;
        const wchar_t    *header;
        const size_t      width;
    };

    struct cal_t
    {
        String name;
        double channels[CHANNEL_SIZE];
        double coeffs  [CHANNEL_SIZE];
    };

    const std::vector<column_t> _columns
    {
        { cell_data_t::name,   L"Name",  300 },
        { cell_data_t::left,   L"Left",  100 },
        { cell_data_t::right,  L"Right", 100 },
        { cell_data_t::button, L"",      30  } // T[06]
    };

    signal           & _signal; // T[07]
    application      & _app;
    std::vector<cal_t> _rows;
    int                _selected { -1 };

    GroupComponent      group;
    ToggleButton        checkbox_cal       {      L"Use Calibration" };
    Label               label_cal_add      { { }, L"Name"            },
                        label_cal_channels { { }, L"Left/Right"      };
    TableListBox        table_cals;
    TextEditor          editor_cal_name;
    TextEditor          editor_cal_channels[CHANNEL_SIZE];
    ComboBox            combo_prefix;
    TextButton          button_cal_add;
//=================================================================================================
public:

    component_calibration(application &app, signal &signal) :
        _signal(signal), _app(app)
    {
        addAndMakeVisible(group);
        addAndMakeVisible(checkbox_cal);
        checkbox_cal.setToggleState(_app.get_int(option_t::calibrate), dontSendNotification);
        checkbox_cal.onClick = [this]
        {
            _app.save(option_t::calibrate, checkbox_cal.getToggleState());
            _signal.extremes_clear(); // T[08]
        };

        addAndMakeVisible(table_cals);
        table_cals.setModel(this);
        table_cals.setColour(ListBox::outlineColourId, Colours::grey);
        table_cals.setOutlineThickness(1);
        table_cals.setHeaderHeight(20);
        table_cals.addMouseListener(this, true);
        table_cals.getHeader().setStretchToFitActive(true);
        table_cals.setRowHeight(20);
        table_cals.getViewport()->setScrollBarsShown(true, false);

        size_t id = 0;
        for (auto const& column : _columns)
            table_cals.getHeader().addColumn(column.header, ++id, column.width, 30, -1, TableHeaderComponent::notSortable); // T[09]

        addAndMakeVisible(label_cal_add);
        addAndMakeVisible(editor_cal_name); // T[10]
        addAndMakeVisible(label_cal_channels);
        addAndMakeVisible(button_cal_add);
        addAndMakeVisible(combo_prefix);

        for (auto& editor : editor_cal_channels) {
            addAndMakeVisible(editor);
            editor.setTextToShowWhenEmpty(L"0.0", Colours::grey); // T[11]
            editor.setInputRestrictions(0, L"0123456789.");

            editor.onTextChange = [this] {
                auto enabled =
                    editor_cal_channels[0].getText().length() &&
                    editor_cal_channels[1].getText().length();
                button_cal_add.setEnabled(enabled);
                button_cal_add.setTooltip(enabled ? L"" : L"You must enter values for both channels!");
            };
        }
        editor_cal_channels[0].onTextChange();

        label_cal_add.attachToComponent(&editor_cal_name, true);
        label_cal_channels.attachToComponent(&editor_cal_channels[LEFT], true);
        editor_cal_name.setTextToShowWhenEmpty(L"Calibration name (optional)", Colours::grey);
        for (auto const& pref : __prefs)
            combo_prefix.addItem(pref.second + L"V", pref.first + 1);

        combo_prefix.setSelectedId(_app.get_int(option_t::prefix) + 1);
        combo_prefix.onChange = [this]
        {
            _app.save(option_t::prefix, combo_prefix.getSelectedId() - 1);
        };

        button_cal_add.setConnectedEdges(ToggleButton::ConnectedOnLeft);
        button_cal_add.setButtonText(L"Add");
        button_cal_add.onClick = [=]
        {
            double channels[CHANNEL_SIZE];
            const auto pref = _app.get_int(option_t::prefix);

            for (auto channel = LEFT; channel < CHANNEL_SIZE; channel++)
            {
                auto channel_text = editor_cal_channels[channel].getText();
                if (channel_text.isEmpty())
                    return;
                channels[channel] = channel_text.getDoubleValue() * pow(10.0, pref);
            }

            auto cals = _app.get_xml(option_t::calibrations);
            if (cals == nullptr)
                cals = std::make_unique<XmlElement>(StringRef(L"ROWS"));

            auto* e  = cals->createNewChildElement(StringRef(L"ROW"));
            auto rms = _signal.get_rms(INPUT);

            e->setAttribute(Identifier(L"name"),        editor_cal_name.getText());
            e->setAttribute(Identifier(L"left"),        channels[LEFT ]);
            e->setAttribute(Identifier(L"left_coeff"),  channels[LEFT ] / rms.at(LEFT ));
            e->setAttribute(Identifier(L"right"),       channels[RIGHT]);
            e->setAttribute(Identifier(L"right_coeff"), channels[RIGHT] / rms.at(RIGHT)); // T[04]

            _app.save(option_t::calibrations, cals.get());
            update();
        };
    }

    ~component_calibration() { }

    void update()
    {
        _rows.clear();
        if (auto cals = _app.get_xml(option_t::calibrations)) {
            forEachXmlChildElement(*cals, el)
            {
                _rows.push_back(cal_t{
                    el->getStringAttribute(Identifier(L"name")),
                    el->getDoubleAttribute(Identifier(L"left")),
                    el->getDoubleAttribute(Identifier(L"right")),
                    el->getDoubleAttribute(Identifier(L"left_coeff")),
                    el->getDoubleAttribute(Identifier(L"right_coeff"))
                });
            }
        }
        _selected = _app.get_int(option_t::calibrations_index);
        table_cals.updateContent();
    };

    double get_coeff(const channel_t channel) {
        return _selected == -1 ? 1.0 : _rows.at(_selected).coeffs[channel];
    }

    void delete_row(const int del_row)
    {
        _rows.erase(_rows.begin() + del_row);

        if (_selected == del_row)
        {
            _selected = -1;
            _app.save(option_t::calibrations_index, _selected);
        }
        else if (del_row < _selected)
        {
            _selected--;
            _app.save(option_t::calibrations_index, _selected);
        }
        table_cals.updateContent();

        auto cals = _app.get_xml(option_t::calibrations);
        if (cals == nullptr) {
            cals = std::make_unique<XmlElement>(StringRef(L"ROWS"));
            cals->createNewChildElement(StringRef(L"SELECTION"))->setAttribute(Identifier(L"cal_index"), -1);
        }
        cals->deleteAllChildElements();

        for (const auto& row : _rows)
        {
            auto* e = cals->createNewChildElement(StringRef(L"ROW"));
            e->setAttribute(Identifier(L"name"),        row.name);
            e->setAttribute(Identifier(L"left"),        row.channels[LEFT ]);
            e->setAttribute(Identifier(L"left_coeff"),  row.coeffs  [LEFT ]);
            e->setAttribute(Identifier(L"right"),       row.channels[RIGHT]);
            e->setAttribute(Identifier(L"right_coeff"), row.coeffs  [RIGHT]);
        }
        _app.save(option_t::calibrations, cals.get());
    }

    //=============================================================================================
    // juce callbacks

    void backgroundClicked(const MouseEvent &) {
        //=========================================================================================
        table_cals.deselectAllRows();
    }

    void mouseDoubleClick(const MouseEvent &) { // B[08]
        //=========================================================================================
        auto current = table_cals.getSelectedRow();
        _selected = current == _selected ? -1 : current;
        _app.save(option_t::calibrations_index, _selected);
        repaint();
    }

    int getNumRows() override {
        //=========================================================================================
        return static_cast<int>(_rows.size());
    }

    void paintRowBackground(Graphics& g, int row, int width, int height, bool is_selected) override {
        //=========================================================================================
        auto bg_color = getLookAndFeel().findColour(ListBox::backgroundColourId)
            .interpolatedWith(getLookAndFeel().findColour(ListBox::textColourId), 0.03f);

        if (row == _selected)
            bg_color = Colours::silver.withAlpha(0.7f);
        if (is_selected)
            bg_color = Colours::silver.withAlpha(0.5f);

        g.fillAll(bg_color);

        if (row == _selected)
            g.drawRect(0, 0, width, height);
    }

    void paintCell(Graphics& g, int row, int column_id, int width, int height, bool /*is_selected*/) override {
        //=========================================================================================
        auto data_selector = _columns.at(column_id - 1).type;
        String text(theme::empty);

        switch (data_selector) {
        case cell_data_t::name:  text = _rows.at(row).name;                            break;
        case cell_data_t::left:  text = prefix(_rows.at(row).channels[LEFT],  L"V", 0); break;
        case cell_data_t::right: text = prefix(_rows.at(row).channels[RIGHT], L"V", 0); break;
        }

        auto text_color = getLookAndFeel().findColour(ListBox::textColourId);
        auto bg_color   = getLookAndFeel().findColour(ListBox::backgroundColourId);

        g.setColour(text_color);
        g.setFont(12.5f);
        g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
    }

    void resized() override {
        //=========================================================================================
        auto area = getLocalBounds().reduced(theme::margin * 2);
        area.removeFromTop   (theme::margin * 3);

        auto bottom = area.removeFromBottom(theme::height * 2 + theme::margin);
        auto line = bottom.removeFromBottom(theme::height);
        line.removeFromLeft(theme::label_width);
        button_cal_add.setBounds(line.removeFromRight(theme::button_width));
        auto edit_width = line.getWidth() / 3;
        editor_cal_channels[LEFT].setBounds(line.removeFromLeft(edit_width - 5));
        line.removeFromLeft(theme::margin);
        editor_cal_channels[RIGHT].setBounds(line.removeFromLeft(edit_width - 5));
        line.removeFromLeft(theme::margin); // B[09]
        line.removeFromRight(theme::margin);
        combo_prefix.setBounds(line);

        bottom.removeFromBottom(theme::margin);
        bottom.removeFromLeft(theme::label_width);
        editor_cal_name.setBounds(bottom.removeFromBottom(theme::height));
        area.removeFromBottom(theme::margin);
        table_cals.setBounds(area);

        group.setBounds(getLocalBounds());
        _app.get_theme()->set_header_checkbox_bounds(checkbox_cal);
    }

    // B[10]
    Component* refreshComponentForCell(int row, int column_id, bool /*is_selected*/, Component* component) override {
        //=========================================================================================
        if (_columns.at(column_id - 1).type == cell_data_t::button)
        {
            auto* button = static_cast<table_custom_button*>(component);
            if (button == nullptr)
                button = std::make_unique<table_custom_button>(*this).release();

            button->set_row(row);
            return button;
        }
        return nullptr;
    }
//=================================================================================================
private:
    class table_custom_button : public Component
    {
    private:
        component_calibration& _owner;
        TextButton             _button;
        int                    _row;
    public:
        table_custom_button(component_calibration& owner) : _owner(owner) // T[05]
        {
            addAndMakeVisible(_button);
            _button.setButtonText(L"X");
            _button.setTooltip(L"Remove this calibration");
            _button.setConnectedEdges(TextButton::ConnectedOnBottom | TextButton::ConnectedOnLeft | TextButton::ConnectedOnRight | TextButton::ConnectedOnTop);
            _button.onClick = [this]
            {
                _owner.delete_row(_row);
            };
        }

        ~table_custom_button() { }

        void resized() override {
            _button.setBoundsInset(BorderSize<int>(2));
        }

        void set_row(const int new_row) {
            _row = new_row;
        }
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(table_custom_button)
    };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_calibration)
};

