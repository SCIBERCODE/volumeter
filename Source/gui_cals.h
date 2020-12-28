#pragma once 
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> _opt;

class calibrations_component_ : public Component,
                                public TableListBoxModel
{
public:

    calibrations_component_(signal_& s) : signal(s)
    {
        addAndMakeVisible(checkbox_cal);
        checkbox_cal.setToggleState(_opt->load_int("checkbox_cal"), dontSendNotification);
        checkbox_cal.onClick = [this]
        {
            _opt->save("checkbox_cal", checkbox_cal.getToggleState());
            signal.minmax_clear(); // todo: сохранять статистику, переводя в вольты
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

        int id = 1;
        for (auto const& column : columns)
            table_cals.getHeader().addColumn(column.header, id++, column.width, 30, -1, TableHeaderComponent::notSortable);

        addAndMakeVisible(label_cal_add);
        addAndMakeVisible(editor_cal_name);
        addAndMakeVisible(label_cal_channels);
        addAndMakeVisible(editor_cal_channels.at(LEFT));
        addAndMakeVisible(editor_cal_channels.at(RIGHT));
        addAndMakeVisible(button_cal_add);
        addAndMakeVisible(combo_prefix);

        label_cal_add.attachToComponent(&editor_cal_name, true);
        label_cal_channels.attachToComponent(&editor_cal_channels.at(LEFT), true);
        editor_cal_name.setTextToShowWhenEmpty("Calibration Name (optional)", Colours::grey);
        editor_cal_channels.at(LEFT) .setTextToShowWhenEmpty("0.0", Colours::grey); // todo: change hint according to selected pref
        editor_cal_channels.at(RIGHT).setTextToShowWhenEmpty("0.0", Colours::grey);
        editor_cal_channels.at(LEFT) .setInputRestrictions(0, "0123456789.");
        editor_cal_channels.at(RIGHT).setInputRestrictions(0, "0123456789.");
        for (auto const& pref : _prefs)
            combo_prefix.addItem(pref.second + "V", pref.first + 1);

        combo_prefix.setSelectedId(_opt->load_int("combo_prefix") + 1);
        combo_prefix.onChange = [this]
        {
            _opt->save("combo_prefix", combo_prefix.getSelectedId() - 1);
        };

        button_cal_add.setButtonText("Add");
        button_cal_add.onClick = [=]
        {
            array<String, 2> channels_text
            {
                editor_cal_channels.at(LEFT) .getText(),
                editor_cal_channels.at(RIGHT).getText()
            };
            if (channels_text.at(LEFT).isEmpty() || channels_text.at(RIGHT).isEmpty()) return;
            auto pref = _opt->load_int("combo_prefix");
            array<double, 2> channels
            {
                channels_text.at(LEFT) .getDoubleValue() * pow(10.0, pref),
                channels_text.at(RIGHT).getDoubleValue() * pow(10.0, pref)
            };

            auto cals = _opt->load_xml("table_cals");
            if (cals == nullptr)
                cals = make_unique<XmlElement>("ROWS");

            auto* e  = cals->createNewChildElement("ROW");
            auto rms = signal.get_rms();

            e->setAttribute("name",        editor_cal_name.getText());
            e->setAttribute("left",        channels.at(LEFT));
            e->setAttribute("left_coeff",  channels.at(LEFT)  / rms.at(LEFT));
            e->setAttribute("right",       channels.at(RIGHT));
            e->setAttribute("right_coeff", channels.at(RIGHT) / rms.at(RIGHT)); // todo: check for nan

            _opt->save("table_cals", cals.get());
            update();
        };
    }

    auto is_active() {
        return checkbox_cal.getToggleState() && selected != -1;
    }

    void update()
    {
        rows.clear();
        auto cals = _opt->load_xml("table_cals");
        if (cals != nullptr) {
            forEachXmlChildElement(*cals, el)
            {
                rows.push_back(cal_t{
                    el->getStringAttribute("name"),
                    el->getDoubleAttribute("left"),
                    el->getDoubleAttribute("right"),
                    el->getDoubleAttribute("left_coeff"),
                    el->getDoubleAttribute("right_coeff")
                });
            }
        }
        selected = _opt->load_int("table_cals_row");
        table_cals.updateContent();
    };

    double get_coeff(level_t channel) {
        return selected == -1 ? 1.0 : rows.at(selected).coeff.at(channel);
    }

    void selectedRowsChanged(int) { }
    void mouseMove(const MouseEvent &) override { }
    void sortOrderChanged(int, bool) override { }

    void backgroundClicked(const MouseEvent &) {
        table_cals.deselectAllRows();
    }

    void mouseDoubleClick(const MouseEvent &) { // bug: срабатывает на заголовке
        auto current = table_cals.getSelectedRow();
        selected = current == selected ? -1 : current;
        _opt->save("table_cals_row", selected);
        repaint();
    }

    int getNumRows() override {
        return static_cast<int>(rows.size());
    }

    void paintRowBackground(Graphics& g, int row, int width, int height, bool is_selected) override {
        auto bg_color = getLookAndFeel().findColour(ListBox::backgroundColourId)
            .interpolatedWith(getLookAndFeel().findColour(ListBox::textColourId), 0.03f);

        if (row == selected)
            bg_color = Colours::silver.withAlpha(0.7f);
        if (is_selected)
            bg_color = Colours::silver.withAlpha(0.5f);

        g.fillAll(bg_color);

        if (row == selected)
            g.drawRect(0, 0, width, height);
    }

    void paintCell(Graphics& g, int row, int column_id, int width, int height, bool /*selected*/) override
    {
        auto data_selector = columns.at(column_id - 1).type;
        String text = "-----";

        switch (data_selector) {
        case cell_data_t::name:  text = rows.at(row).name;                     break;
        case cell_data_t::left:  text = prefix(rows.at(row).channel.at(LEFT),  "V", 0); break;
        case cell_data_t::right: text = prefix(rows.at(row).channel.at(RIGHT), "V", 0); break;
        }

        auto text_color = getLookAndFeel().findColour(ListBox::textColourId);
        auto bg_color   = getLookAndFeel().findColour(ListBox::backgroundColourId);

        g.setColour(text_color);
        g.setFont(12.5f);
        g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
    }

    void resized() override {
        auto area = getLocalBounds();
        area.removeFromBottom(theme::margin);
        auto bottom = area.removeFromBottom(theme::height * 2 + theme::margin);
        button_cal_add.setBounds(bottom.removeFromRight(theme::label).withTrimmedLeft(theme::margin));
        auto line = bottom.removeFromBottom(theme::height);
        line.removeFromLeft(theme::label);
        int edit_width = (line.getWidth() - theme::label) / 2;
        editor_cal_channels.at(LEFT).setBounds(line.removeFromLeft(edit_width));
        line.removeFromLeft(theme::margin);
        editor_cal_channels.at(RIGHT).setBounds(line.removeFromLeft(edit_width));
        line.removeFromLeft(theme::margin);
        combo_prefix.setBounds(line);
        bottom.removeFromBottom(theme::margin);
        bottom.removeFromLeft(theme::label);
        editor_cal_name.setBounds(bottom.removeFromBottom(theme::height));
        area.removeFromBottom(theme::margin);
        checkbox_cal.setBounds(area.removeFromTop(theme::height));
        area.removeFromTop(theme::margin);
        table_cals.setBounds(area);
    }

    Component* refreshComponentForCell(int row, int column_id, bool /*selected*/, Component* component) override
    {
        if (columns.at(column_id - 1).type == cell_data_t::button)
        {
            auto* button = static_cast<_table_custom_button*>(component);
            if (button == nullptr)
                button = new _table_custom_button(*this);

            button->set_row(row);
            return button;
        }
        jassert(component == nullptr);
        return nullptr;
    }

    void delete_row(int del_row)
    {
        rows.erase(rows.begin() + del_row);

        if (selected == del_row)
        {
            selected = -1;
            _opt->save("table_cals_row", selected);
        }
        else if (del_row < selected)
        {
            selected--;
            _opt->save("table_cals_row", selected);
        }
        table_cals.updateContent();

        auto cals = _opt->load_xml("table_cals");
        if (cals == nullptr) {
            cals = make_unique<XmlElement>("ROWS");
            cals->createNewChildElement("SELECTION")->setAttribute("cal_index", -1);
        }
        cals->deleteAllChildElements();

        for (const auto& row : rows)
        {
            auto* e = cals->createNewChildElement("ROW");
            e->setAttribute("name",        row.name);
            e->setAttribute("left",        row.channel.at(LEFT));
            e->setAttribute("left_coeff",  row.coeff.at(LEFT));
            e->setAttribute("right",       row.channel.at(RIGHT));
            e->setAttribute("right_coeff", row.coeff.at(RIGHT));
        }
        _opt->save("table_cals", cals.get());
    }

//=========================================================================================
protected:
//=========================================================================================

    enum class cell_data_t { name, left, right, button };
    struct column_t
    {
        cell_data_t type;
        char*       header;
        uint16_t    width;
    };
    const vector<column_t> columns =
    {
        { cell_data_t::name,   "Name",  300 },
        { cell_data_t::left,   "Left",  100 },
        { cell_data_t::right,  "Right", 100 },
        { cell_data_t::button, "",      30  },
    };

    struct cal_t
    {
        String           name;
        array<double, 2> channel;
        array<double, 2> coeff;
    };

    class _table_custom_button : public Component
    {
    public:
        _table_custom_button(calibrations_component_& td) : owner(td)
        {
            addAndMakeVisible(button);
            button.setButtonText("X");
            button.setConnectedEdges(TextButton::ConnectedOnBottom | TextButton::ConnectedOnLeft | TextButton::ConnectedOnRight | TextButton::ConnectedOnTop);
            button.onClick = [this]
            {
                owner.delete_row(row);
            };
        }
        void resized() override {
            button.setBoundsInset(BorderSize<int>(2));
        }
        void set_row(int new_row) {
            row = new_row;
        }

    private:
        calibrations_component_& owner;
        TextButton               button;
        int                      row;
    };

//=========================================================================================
private:
//=========================================================================================

    vector<cal_t>        rows;
    int                  selected = -1;
    signal_&             signal;

    TableListBox         table_cals;
    array<TextEditor, 2> editor_cal_channels;
    TextEditor           editor_cal_name;
    ComboBox             combo_prefix;
    TextButton           button_cal_add;
    Label                label_cal_add      { {}, "Name"            },
                         label_cal_channels { {}, "Left/Right"      };
    ToggleButton         checkbox_cal       {     "Use Calibration" };

    String getAttributeNameForColumnId(const int columnId) const
    {
        int id = 1;
        for (auto const column : columns) {
            if (columnId == ++id) return column.header;
        }
        return {};
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(calibrations_component_)
};
