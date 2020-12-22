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
		addAndMakeVisible(cal_checkbox);
		cal_checkbox.setToggleState(_opt->load_int("calibrate", "0"), dontSendNotification);
		cal_checkbox.onClick = [this]
		{
			_opt->save("calibrate", cal_checkbox.getToggleState());
			signal.clear_stat(); // todo: сохранять статистику, переводя в вольты
		};

		addAndMakeVisible(table);
		table.setModel(this);
		table.setColour(ListBox::outlineColourId, Colours::grey);
		table.setOutlineThickness(1);
		table.setHeaderHeight(20);
		table.addMouseListener(this, true);
		table.getHeader().setStretchToFitActive(true);
		table.setRowHeight(20);
		table.getViewport()->setScrollBarsShown(true, false);

		int id = 1;
		for (auto const& column : columns)
			table.getHeader().addColumn(column.header, id++, column.width, 30, -1, TableHeaderComponent::notSortable);

		addAndMakeVisible(cal_label_add);
		addAndMakeVisible(cal_edit_name);
		addAndMakeVisible(cal_label_ch);
		addAndMakeVisible(cal_edit_ch[0]);
		addAndMakeVisible(cal_edit_ch[1]);
		addAndMakeVisible(cal_button);
		addAndMakeVisible(prefix_combo);
		cal_label_add.attachToComponent(&cal_edit_name, true);
		cal_label_ch.attachToComponent(&cal_edit_ch[0], true);
		cal_edit_name.setTextToShowWhenEmpty("Calibration Name (optional)", Colours::grey);
		cal_edit_ch[0].setTextToShowWhenEmpty("0.0", Colours::grey); // todo: change hint according to selected pref
		cal_edit_ch[1].setTextToShowWhenEmpty("0.0", Colours::grey);
		cal_edit_ch[0].setInputRestrictions(0, "0123456789.");
		cal_edit_ch[1].setInputRestrictions(0, "0123456789.");
		for (auto const& pref : _prefs)
			prefix_combo.addItem(pref.second + "V", pref.first + 1);

		prefix_combo.setSelectedId(_opt->load_int("pref", "0") + 1);
		prefix_combo.onChange = [this]
		{
			auto val = prefix_combo.getSelectedId();
			_opt->save("pref", val - 1);
		};

		cal_button.setButtonText("Add");
		cal_button.onClick = [=]
		{
			String ch_t[2] = { cal_edit_ch[0].getText(), cal_edit_ch[1].getText() };
			if (ch_t[0].isEmpty() || ch_t[1].isEmpty()) return;

			auto pref = _opt->load_int("pref", "0");
			double ch[2] = {
				ch_t[0].getDoubleValue() * pow(10.0, pref),
				ch_t[1].getDoubleValue() * pow(10.0, pref)
			};

			auto cals = _opt->load_xml("calibrations");
			if (cals == nullptr)
				cals = std::make_unique<XmlElement>("ROWS");

			auto* e  = cals->createNewChildElement("ROW");
			auto lrb = signal.get_lrb();

			e->setAttribute("name",       cal_edit_name.getText());
			e->setAttribute("left",       ch[0]);
			e->setAttribute("left_coef",  ch[0] / lrb.at(0));
			e->setAttribute("right",      ch[1]);
			e->setAttribute("right_coef", ch[1] / lrb.at(1)); // todo: check for nan

			_opt->save("calibrations", move(cals));
			update();
		};
	}

	~calibrations_component_() { }

	auto is_active() {
		return cal_checkbox.getToggleState() && selected != -1;
	}

	void update()
	{
		rows.clear();
		auto cals = _opt->load_xml("calibrations");
		if (cals != nullptr) {
			forEachXmlChildElement(*cals, el)
			{
				rows.push_back(cal_t{
					el->getStringAttribute("name"),
					el->getDoubleAttribute("left"),
					el->getDoubleAttribute("right"),
					el->getDoubleAttribute("left_coef"),
					el->getDoubleAttribute("right_coef")
				});
			}
		}
		selected = _opt->load_int("cal_index", "-1");
		table.updateContent();
	};

	auto coef() {
		double nope[2] = { 1.0, 1.0 };
		return selected == -1 ? nope : rows.at(selected).ch_coef;
	}
	   
	void selectedRowsChanged(int) { }
	void mouseMove(const MouseEvent &) override { }
	void sortOrderChanged(int, bool) override {	}

	void backgroundClicked(const MouseEvent &) {
		table.deselectAllRows();
	}

	void mouseDoubleClick(const MouseEvent &) { // bug: срабатывает на заголовке
		auto current = table.getSelectedRow();
		selected = current == selected ? -1 : current;
		_opt->save("cal_index", selected);
		repaint();
	}

	int getNumRows() override {
		return static_cast<int>(rows.size());
	}

	void paintRowBackground(Graphics& g, int row, int width, int height, bool is_selected) override	{
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
		case cell_data_t::name:  text = rows.at(row).name;                  break;
		case cell_data_t::left:  text = prefix(rows.at(row).ch[0], "V", 0); break;
		case cell_data_t::right: text = prefix(rows.at(row).ch[1], "V", 0); break;
		}

		auto text_color = getLookAndFeel().findColour(ListBox::textColourId);
		auto bg_color   = getLookAndFeel().findColour(ListBox::backgroundColourId);

		g.setColour(text_color);
		g.setFont(12.5f);
		g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
	}

	void resized() override {
		auto area = getLocalBounds();
		area.removeFromBottom(_magic::margin);
		auto bottom = area.removeFromBottom(_magic::height * 2 + _magic::margin);
		cal_button.setBounds(bottom.removeFromRight(_magic::label).withTrimmedLeft(_magic::margin));
		auto line = bottom.removeFromBottom(_magic::height);
		line.removeFromLeft(_magic::label);
		int edit_width = (line.getWidth() - _magic::label) / 2;
		cal_edit_ch[0].setBounds(line.removeFromLeft(edit_width));
		line.removeFromLeft(_magic::margin);
		cal_edit_ch[1].setBounds(line.removeFromLeft(edit_width));
		line.removeFromLeft(_magic::margin);
		prefix_combo.setBounds(line);
		bottom.removeFromBottom(_magic::margin);
		bottom.removeFromLeft(_magic::label);
		cal_edit_name.setBounds(bottom.removeFromBottom(_magic::height));
		area.removeFromBottom(_magic::margin);
		cal_checkbox.setBounds(area.removeFromTop(_magic::height));
		area.removeFromTop(_magic::margin);
		table.setBounds(area);
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
			_opt->save("cal_index", selected);
		}
		else if (del_row < selected)
		{
			selected--;
			_opt->save("cal_index", selected);
		}
		table.updateContent();

		auto cals = _opt->load_xml("calibrations");
		if (cals == nullptr) {
			cals = make_unique<XmlElement>("ROWS");
			cals->createNewChildElement("SELECTION")->setAttribute("cal_index", -1);
		}
		cals->deleteAllChildElements();

		for (auto row : rows)
		{
			auto* e = cals->createNewChildElement("ROW");
			e->setAttribute("name",       row.name);
			e->setAttribute("left",       row.ch[0]);
			e->setAttribute("left_coef",  row.ch_coef[0]);
			e->setAttribute("right",      row.ch[1]);
			e->setAttribute("right_coef", row.ch_coef[1]);
		}
		_opt->save("calibrations", move(cals));
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
	const std::vector<column_t> columns =
	{
		{ cell_data_t::name,   "Name",  300 },
		{ cell_data_t::left,   "Left",  100 },
		{ cell_data_t::right,  "Right", 100 },
		{ cell_data_t::button, "",      30  },
	};

	struct cal_t
	{
		String name;
		double ch[2];
		double ch_coef[2];
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
			button.setBoundsInset(juce::BorderSize<int>(2));
		}
		void set_row(int new_row) {
			row = new_row;
		}

	private:
		calibrations_component_& owner;
		TextButton				 button;
		int						 row;
	};

//=========================================================================================
private:
//=========================================================================================

	vector<cal_t> rows;
	int           selected = -1;
	signal_     & signal;

	TableListBox  table;
	TextEditor    cal_edit_ch[2], cal_edit_name;
	ComboBox	  prefix_combo;
	TextButton    cal_button;
	Label         cal_label_add { {}, "Name"            },
	              cal_label_ch  { {}, "Left/Right"      };
	ToggleButton  cal_checkbox  {     "Use Calibration" };	

	String getAttributeNameForColumnId(const int columnId) const
	{
		int id = 1;
		for (auto const column : columns) {
			if (columnId == id++) return column.header;
		}
		return {};
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(calibrations_component_)
};