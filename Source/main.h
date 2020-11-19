#pragma once
#include <memory>
#include "theme.h"
using namespace std;

#include <spuce/filters/iir.h>
#include <spuce/filters/butterworth_iir.h>

extern unique_ptr<ApplicationProperties> _settings;

const vector<float>    _buff_size_list = { 0.1f, 0.2f, 0.5f, 1.0f, 2.0f, 10.0f };
const vector<int>      _tone_list      = { 10, 20, 200, 500, 1000, 5000, 10000, 20000 };
const vector<int>      _order_list     = { 1, 2, 4, 10, 20, 40, 60, 80, 100, 120, 140, 200 };
const map<int, String> _prefs          = {
	{ -15,  "f" },
	{ -12,  "p" },
	{  -9,  "n" },
	{  -6, L"μ" },
	{  -3,  "m" },
	{   0,  ""  },
	{   3,  "k" },
	{   6,  "M" }
};

struct cal_t {
	String name;
	double ch[2];
	double ch_coef[2];
};

auto prefix(double value, String unit, size_t numder_of_decimals)
{
	auto   symbol    = String();
	double new_value = value;

	auto exp = (int)(floor(log10(value) / 3.0) * 3.0);
	if (_prefs.count(exp))
	{
		symbol = _prefs.at(exp);
		new_value = value * pow(10.0, -exp);
		if (new_value >= 1000.0)
		{
			new_value /= 1000.0;
			exp += 3;
		}
	}
	return String(new_value, (int)numder_of_decimals) + " " + symbol + unit;
}

auto prefix_v(double value) {
	return prefix(value, "V", 5);
}

void _SAVE(const String& key, const var& value) {
	_settings->getUserSettings()->setValue(key, value);
	_settings->saveIfNeeded();
}

auto _LOAD_STR(const String& key, const String& default_value = "") {
	return _settings->getUserSettings()->getValue(key, default_value);
}

auto _LOAD_INT(const String& key, const String& default_value = "1000") {
	return _LOAD_STR(key, default_value).getIntValue();
}

//==============================================================================
class table_cals_ : public Component, public TableListBoxModel
{
private:
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
public:
	vector<cal_t> rows;
	int selected = -1;

	auto coef() {
		double nope[2] = { 1.0, 1.0 };
		return selected == -1 ? nope : rows.at(selected).ch_coef;
	}

	table_cals_()
	{
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
		for (auto const column : columns) {
			table.getHeader().addColumn(column.header, id++, column.width, 30, -1, TableHeaderComponent::notSortable);
		}
	}

	~table_cals_() {
	}

	void selectedRowsChanged(int) {
	}

	void backgroundClicked(const MouseEvent &) {
		table.deselectAllRows();
	}

	void mouseDoubleClick(const MouseEvent &) { // bug: срабатывает на заголовке
		auto current = table.getSelectedRow();
		selected = current == selected ? -1 : current;
		_SAVE("cal_index", selected);
		repaint();
	}

	void mouseMove(const MouseEvent &) override {
	}

	int getNumRows() override {
		return (int)rows.size();
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
		case cell_data_t::name:  text = rows.at(row).name;  break;
		case cell_data_t::left:  text = prefix(rows.at(row).ch[0], "V", 0); break;
		case cell_data_t::right: text = prefix(rows.at(row).ch[1], "V", 0); break;
		}

		auto text_color = getLookAndFeel().findColour(ListBox::textColourId);
		auto bg_color   = getLookAndFeel().findColour(ListBox::backgroundColourId);

		Font font{ 12.5f };
		//if (row == selected) font.setBold(true);

		g.setColour(text_color);
		g.setFont(font);
		g.drawText(text, 2, 0, width - 4, height, Justification::centredLeft, true);
	}

	void sortOrderChanged(int, bool) override {
	}

	void reload() {
		table.updateContent();
	}

	void resized() override {
		auto area = getLocalBounds();
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

	void delete_row(int del_row) {
		rows.erase(rows.begin() + del_row);

		if (selected == del_row)
		{
			selected = -1;
			_SAVE("cal_index", selected);
		}
		else if (del_row < selected)
		{
			selected--;
			_SAVE("cal_index", selected);
		}
		table.updateContent();

		auto cals = _settings->getUserSettings()->getXmlValue("calibrations");
		if (cals == nullptr) {
			cals = std::make_unique<XmlElement>("ROWS");
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
		_settings->getUserSettings()->setValue("calibrations", cals.get());
		_settings->saveIfNeeded();
	}

private:
	TableListBox table;

	String getAttributeNameForColumnId(const int columnId) const
	{
		int id = 1;
		for (auto const column : columns) {
			if (columnId == id++) return column.header;
		}
		return {};
	}

	class _table_custom_button : public Component
	{
	public:
		_table_custom_button(table_cals_& td) : owner(td) {
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
		table_cals_& owner;
		TextButton   button;
		int          row;
	};

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(table_cals_)
};

//==============================================================================
class sin_
{
private:
	double
		sampling_rate,
		freq,
		angle_delta,
		angle;
public:
	sin_() : sampling_rate(44100.0), angle(0.0), angle_delta(0.0), freq(0.0) {}

	void set_sample_rate(double sample_rate) {
		sampling_rate = sample_rate;
	}
	void reset() {
		angle = 0;
	}
	void set_freq(float frequency) {
		freq = frequency;
		auto cycles_per_sample = frequency / sampling_rate;
		angle_delta = cycles_per_sample * 2.0 * 3.14159265358979323846;
	}
	auto sample() {
		auto current_sample = sin(angle);
		angle += angle_delta;
		return static_cast<float>(current_sample);
	}
	auto get_freq() {
		return static_cast<float>(freq);
	}
};

//==============================================================================
template <class T> class circular_buffer {
private:
	std::unique_ptr<T[]> buffer;
	size_t               head = 0;
	size_t               tail = 0;
	size_t               max_size;
	T                    empty_item;
public:
	void clear() {
		for (size_t k = 0; k < max_size; k++) {
			buffer[k] = 555;
		}
	}
	circular_buffer<T>(size_t max_size)
		: buffer(std::unique_ptr<T[]>(new T[max_size])), max_size(max_size)
	{
		//std::generate(std::begin(buffer), std::end(buffer), [] { return 0; });
		clear();
	};

	void enqueue(T item)
	{
		if (max_size == 0) return;
		buffer[tail] = item * item;
		tail = (tail + 1) % max_size;
	}

	double get_value()
	{
		double sum  = 0.0;
		size_t size = 0;

		for (size_t k = 0; k < max_size; ++k)
		{
			auto sample = buffer[k];
			if (sample != 555 && sample) {
				sum += sample;
				size++;
			}
		}
		return sqrt(sum / size);
	}
};

//==============================================================================
class MainContentComponent : public AudioAppComponent,
                             public Timer,
                             public ChangeListener
{
public:
	void clear_stat() {
		for (size_t k = 0; k < 3; k++) {
			minmax[k][0] = 555;
			minmax[k][1] = 555;
		}
	}

	void changeListenerCallback(ChangeBroadcaster* /*source*/) {
	}

	MainContentComponent()
	{
		buff[0].reset(new circular_buffer<float>(0));
		buff[1].reset(new circular_buffer<float>(0));

		addAndMakeVisible (&zero_button);
		zero_button.setButtonText ("Set zero");
		zero_button.onClick = [this]
		{
			zero[0] = gain2db(buff[0]->get_value());
			zero[1] = gain2db(buff[1]->get_value());
			clear_stat();
		};

		addAndMakeVisible(&reset_zero_button);
		reset_zero_button.setButtonText("Reset zero");
		reset_zero_button.onClick = [this]
		{
			zero[0] = 0; zero[1] = 0;
			clear_stat();
		};

		addAndMakeVisible(reset_minmax);
		reset_minmax.setButtonText("Reset stat");
		reset_minmax.onClick = [this]
		{
			clear_stat();
		};

		addAndMakeVisible(tone_checkbox);
		addAndMakeVisible(tone_combo);
		tone_checkbox.setToggleState(_LOAD_INT("tone", "0"), dontSendNotification);
		tone_checkbox.setLookAndFeel(&theme_right_text);
		tone_checkbox.onClick = [this] {
			clear_stat();
			_SAVE("tone", tone_checkbox.getToggleState());
		};
		for (auto const item : _tone_list)
			tone_combo.addItem(prefix(item, "Hz", 0), item);

		tone_combo.setSelectedId(_LOAD_INT("tone_value"));
		tone_combo.onChange = [this]
		{
			auto val = tone_combo.getSelectedId();
			osc.set_freq((float)val);
			osc.reset();
			_SAVE("tone_value", val);
		};

		addAndMakeVisible(buff_size_combo);
		addAndMakeVisible(buff_size_label);
		buff_size_label.attachToComponent(&buff_size_combo, true);
		for (auto const item : _buff_size_list)
			buff_size_combo.addItem(prefix(item, "s", 0), (int)(item * 1000));

		buff_size_combo.setSelectedId(_LOAD_INT("buff_size"));
		buff_size_combo.onChange = [this]
		{
			auto val = buff_size_combo.getSelectedId();
			if (val && sample_rate) {
				auto samples = (size_t)(val / (1000.0 / sample_rate));
				buff[0].reset(new circular_buffer<float>(samples));
				buff[1].reset(new circular_buffer<float>(samples));
			}
			_SAVE("buff_size", val);
		};		

		addAndMakeVisible(l_label_value); addAndMakeVisible(l_label_value_minmax);
		addAndMakeVisible(l_label);
		l_label.attachToComponent(&l_label_value, true);
		l_label_value.setFont(l_label_value.getFont().boldened());
		l_label_value_minmax.setJustificationType(Justification::right);

		addAndMakeVisible(r_label_value); addAndMakeVisible(r_label_value_minmax);
		addAndMakeVisible(r_label);
		r_label.attachToComponent(&r_label_value, true);
		r_label_value.setFont(r_label_value.getFont().boldened());
		r_label_value_minmax.setJustificationType(Justification::right);

		addAndMakeVisible(b_label_value); addAndMakeVisible(b_label_value_minmax);
		addAndMakeVisible(balance_label);
		balance_label.attachToComponent(&b_label_value, true);
		b_label_value.setFont(b_label_value.getFont().boldened());
		b_label_value_minmax.setJustificationType(Justification::right);

		auto cal_table_update = [=]
		{
			table_cals.rows.clear();
			cal_edit_name.setText(String());
			cal_edit_ch[0].setText(String());
			cal_edit_ch[1].setText(String());
			
			auto cals = _settings->getUserSettings()->getXmlValue("calibrations");
			if (cals != nullptr)
			{
				forEachXmlChildElement(*cals, el)
				{
					table_cals.rows.push_back(cal_t{
						el->getStringAttribute("name"),
						el->getDoubleAttribute("left"),
						el->getDoubleAttribute("right"),
						el->getDoubleAttribute("left_coef"),
						el->getDoubleAttribute("right_coef")
					});
				}
			}
			table_cals.selected = _LOAD_INT("cal_index", "-1");
			table_cals.reload();
		};

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
		for (auto const pref : _prefs)
			prefix_combo.addItem(pref.second + "V", pref.first + 1);

		prefix_combo.setSelectedId(_LOAD_INT("pref", "0") + 1);
		prefix_combo.onChange = [this]
		{
			auto val = prefix_combo.getSelectedId();
			_SAVE("pref", val - 1);
		};

		cal_button.setButtonText("Add");
		cal_button.onClick = [=]
		{
			String ch_t[2] = { cal_edit_ch[0].getText(), cal_edit_ch[1].getText() };
			if (ch_t[0].isEmpty() || ch_t[1].isEmpty()) return;

			auto pref = _LOAD_INT("pref", "0");
			double ch[2] = {
				ch_t[0].getDoubleValue() * pow(10.0, pref),
				ch_t[1].getDoubleValue() * pow(10.0, pref)
			};

			auto cals = _settings->getUserSettings()->getXmlValue("calibrations");
			if (cals == nullptr) {
				cals = std::make_unique<XmlElement>("ROWS");
			}
			auto* e = cals->createNewChildElement("ROW");
			e->setAttribute("name",       cal_edit_name.getText());
			e->setAttribute("left",       ch[0]);
			e->setAttribute("left_coef",  ch[0] / buff[0]->get_value());
			e->setAttribute("right",      ch[1]);
			e->setAttribute("right_coef", ch[1] / buff[1]->get_value()); // todo: check for nan

			_settings->getUserSettings()->setValue("calibrations", cals.get());
			_settings->saveIfNeeded();
			cal_table_update();
		};

		setAudioChannels(2, 2);

		const OwnedArray<AudioIODeviceType>& types = deviceManager.getAvailableDeviceTypes();

		addAndMakeVisible(types_combo);
		addAndMakeVisible(outputs_combo);
		addAndMakeVisible(rates_combo);
		if (types.size() > 1)
		{
			int index = 0;
			for (int i = 0; i < types.size(); ++i)
			{
				auto name_type = types.getUnchecked(i)->getTypeName();
				types_combo.addItem(name_type, i + 1);
				if (name_type == "ASIO") index = i;
			}

			types_combo.setSelectedItemIndex(index);
			types_combo.onChange = [this]
			{
				AudioIODeviceType *type = deviceManager.getAvailableDeviceTypes()[types_combo.getSelectedId() - 1];
				if (type)
				{
					deviceManager.setCurrentAudioDeviceType(type->getTypeName(), true);
					
					const StringArray devs(type->getDeviceNames());
					outputs_combo.clear(dontSendNotification);

					auto device = _LOAD_STR("device");

					for (int i = 0; i < devs.size(); ++i) {
						outputs_combo.addItem(devs[i], i + 1);
						if (device == devs[i])
							outputs_combo.setSelectedItemIndex(i);
					}
					if (device.isEmpty())
						outputs_combo.setSelectedItemIndex(0);
				}
			};
		}
		outputs_combo.onChange = [this]
		{
			buff[0]->clear(); buff[1]->clear();
			clear_stat();

			_SAVE("device", outputs_combo.getText());
			auto sample_rate = _LOAD_INT("sample_rate", "48000");

			auto config = deviceManager.getAudioDeviceSetup();
			config.outputDeviceName = outputs_combo.getText();
			config.inputDeviceName = config.outputDeviceName;
			config.useDefaultInputChannels = true;
			config.useDefaultOutputChannels = true;
			config.sampleRate = sample_rate;
			deviceManager.setAudioDeviceSetup(config, true);

			if (auto* currentDevice = deviceManager.getCurrentAudioDevice()) {
				rates_combo.clear(dontSendNotification);
				for (auto rate : currentDevice->getAvailableSampleRates())
				{
					auto intRate = roundToInt(rate);
					rates_combo.addItem(String(intRate) + " Hz", intRate);
				}
				rates_combo.setSelectedId(sample_rate, dontSendNotification);
			}
		};
		rates_combo.onChange = [this]
		{
			_SAVE("sample_rate", rates_combo.getSelectedId());
			outputs_combo.onChange();
		};

		addAndMakeVisible(cal_checkbox);
		cal_checkbox.setToggleState(_LOAD_INT("calibrate", "0"), dontSendNotification);
		cal_checkbox.onClick = [this] 
		{
			_SAVE("calibrate", cal_checkbox.getToggleState());
			clear_stat(); // todo: сохранять статистику, переводя в вольты
		};

		// === filter ==================================================================

		addAndMakeVisible(bpfH_checkbox);
		addAndMakeVisible(bpfL_checkbox);
		addAndMakeVisible(high_slider);
		addAndMakeVisible(order_combo);

		bpfH_checkbox.setToggleState(_LOAD_INT("use_bpfH", "0"), dontSendNotification);
		bpfH_checkbox.setLookAndFeel(&theme_right);

		bpfH_checkbox.onClick = [this] {
			_SAVE("use_bpfH", bpfH_checkbox.getToggleState());
			filter_init();
		};
		bpfL_checkbox.setToggleState(_LOAD_INT("use_bpfL", "0"), dontSendNotification);
		bpfL_checkbox.onClick = [this] {
			_SAVE("use_bpfL", bpfL_checkbox.getToggleState());
			filter_init();
		};

		high_slider.setSliderStyle(Slider::LinearHorizontal);
		high_slider.setTextBoxStyle(Slider::TextBoxRight, false, 60, 20);
		high_slider.setRange(20, 20000, 1);
		high_slider.setValue(15000, dontSendNotification);
		high_slider.onDragEnd = [this] {
			filter_init();
		};

		for (auto const item : _order_list)
			order_combo.addItem(String(item), item);

		order_combo.setSelectedId(120, dontSendNotification);
		order_combo.setTooltip("Filter Order");
		order_combo.onChange = [this]
		{
			order = order_combo.getSelectedId();
			filter_init();
		};

		addAndMakeVisible(table_cals);
		cal_table_update();

		setSize(420, 670);
		startTimer(100);
	}

	~MainContentComponent() override
	{
		shutdownAudio();
		tone_checkbox.setLookAndFeel(nullptr);
		bpfH_checkbox.setLookAndFeel(nullptr);
	}

	void filter_init()
	{
		{
			lock_guard<mutex> locker(audio_process);
			high_slider.setRange(20.0, sample_rate / 2.0, 1.0);

			if (bpfH_checkbox.getToggleState())
			{
				spuce::iir_coeff bpfH(order, spuce::filter_type::high);
				spuce::butterworth_iir(bpfH, 20.0 / sample_rate, 3.0);
				iir[0].reset(new spuce::iir<spuce::float_type, spuce::float_type>(bpfH));
			}
			if (bpfL_checkbox.getToggleState())
			{
				spuce::iir_coeff bpfL(order, spuce::filter_type::low);
				spuce::butterworth_iir(bpfL, high_slider.getValue() / sample_rate, 3.0);
				iir[1].reset(new spuce::iir<spuce::float_type, spuce::float_type>(bpfL));
			}
		}
		high_slider.repaint();
	};

	void prepareToPlay (int /*samples_per_block*/, double sample_rate_) override
	{
		sample_rate = sample_rate_;
		buff_size_combo.onChange();

		osc.set_sample_rate(sample_rate);
		osc.set_freq((float)_LOAD_INT("tone_value"));
		osc.reset();

		filter_init();
	}

	void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
	{
		auto r = bufferToFill.buffer->getArrayOfReadPointers();
		auto w = bufferToFill.buffer->getArrayOfWritePointers();

		for (int sample_idx = 0; sample_idx < bufferToFill.numSamples; ++sample_idx)
		{
			if (r[0][sample_idx]) buff[0]->enqueue(r[0][sample_idx]);
			if (r[1][sample_idx]) buff[1]->enqueue(r[1][sample_idx]);
		}

		if (tone_checkbox.getToggleState())
		{
			for (int sample = 0; sample < bufferToFill.numSamples; ++sample)
			{
				auto tone_sample = osc.sample();
				w[0][sample] = tone_sample;
				w[1][sample] = tone_sample;
			}
		}
		else {
			if (audio_process.try_lock())
			{
				auto use_bpfH = bpfH_checkbox.getToggleState() && iir[0];
				auto use_bpfL = bpfL_checkbox.getToggleState() && iir[1];

				if (use_bpfH || use_bpfL)
				{
					for (size_t ch = 0; ch < 2; ++ch) {
						for (int sample_idx = 0; sample_idx < bufferToFill.numSamples; ++sample_idx)
						{
							spuce::float_type sample = r[ch][sample_idx]; // random.nextFloat();
							if (use_bpfH) sample = iir[0]->clock(sample);
							if (use_bpfL) sample = iir[1]->clock(sample);
							w[ch][sample_idx] = (float)sample;
						}
						if (use_bpfH) iir[0]->reset();
						if (use_bpfL) iir[1]->reset();
					}
				}
				audio_process.unlock();
			}
		}
		//else bufferToFill.clearActiveBufferRegion();
	}

	void releaseResources() override { }

	void resized() override
	{
		const size_t height = 20;
		const size_t margin = 5;
		const size_t label  = 75;

		auto area = getLocalBounds().reduced(margin * 2);

		area.removeFromBottom(margin);
		auto bottom = area.removeFromBottom(height * 2 + margin);
		cal_button.setBounds(bottom.removeFromRight(label).withTrimmedLeft(margin));
		auto line = bottom.removeFromBottom(height);
		line.removeFromLeft(label);
		int edit_width = (line.getWidth() - label) / 2;
		cal_edit_ch[0].setBounds(line.removeFromLeft(edit_width));
		line.removeFromLeft(margin);
		cal_edit_ch[1].setBounds(line.removeFromLeft(edit_width));
		line.removeFromLeft(margin);
		prefix_combo.setBounds(line);
		bottom.removeFromBottom(margin);
		bottom.removeFromLeft(label);
		cal_edit_name.setBounds(bottom.removeFromBottom(height));
		area.removeFromBottom(margin);
		table_cals.setBounds(area.removeFromBottom(200));
		area.removeFromBottom(margin);
		cal_checkbox.setBounds(area.removeFromBottom(height));
		area.removeFromBottom(height);

		area.removeFromBottom(margin);
		reset_minmax.setBounds(area.removeFromBottom(height));
		area.removeFromBottom(margin);
		reset_zero_button.setBounds(area.removeFromBottom(height));
		area.removeFromBottom(margin);
		zero_button.setBounds(area.removeFromBottom(height));
		area.removeFromBottom(margin);

		types_combo.setBounds(area.removeFromTop(height));
		area.removeFromTop(margin);
		outputs_combo.setBounds(area.removeFromTop(height));
		area.removeFromTop(margin);
		rates_combo.setBounds(area.removeFromTop(height));
		area.removeFromTop(margin);

		line = area.removeFromTop(height);
		bpfH_checkbox.setBounds(line.removeFromLeft(60));
		high_slider.setBounds(line.removeFromLeft(line.getWidth() - (60 + label) - margin * 2));
		line.removeFromLeft(margin);
		order_combo.setBounds(line.removeFromLeft(60 + margin));
		line.removeFromLeft(margin);
		bpfL_checkbox.setBounds(line);
		area.removeFromTop(margin);

		area.removeFromTop(height + margin);

		line = area.removeFromTop(height);
		line.removeFromLeft(label);
		buff_size_combo.setBounds(line);
		area.removeFromTop(margin);
		line = area.removeFromTop(height);
		tone_checkbox.setBounds(line.removeFromLeft(label - margin));
		line.removeFromLeft(margin);
		tone_combo.setBounds(line);
		area.removeFromTop(margin);

		area.removeFromLeft(label);
		auto right = area.removeFromRight(getWidth() / 2);
		right.removeFromRight(margin * 2);

		l_label_value.setBounds(area.removeFromTop(height));
		area.removeFromTop(margin);
		r_label_value.setBounds(area.removeFromTop(height));
		area.removeFromTop(margin);
		b_label_value.setBounds(area.removeFromTop(height));

		l_label_value_minmax.setBounds(right.removeFromTop(height));
		right.removeFromTop(margin);
		r_label_value_minmax.setBounds(right.removeFromTop(height));
		right.removeFromTop(margin);
		b_label_value_minmax.setBounds(right.removeFromTop(height));
	}

	double gain2db(double gain) {
		return 20 * log10(gain);
	}

	void display(Label &lbl, String text) {
		if (text.contains("inf") || text.contains("nan"))
		{
			lbl.setText("--", dontSendNotification);
			lbl.setColour(Label::ColourIds::textColourId, Colours::grey);
		}
		else {
			lbl.setText(text, dontSendNotification);
			lbl.setColour(Label::ColourIds::textColourId, Colours::black);
		}
	}

	auto display(String text) {
		if (text.contains("inf") || text.contains("nan") || text.contains("555.000"))
		{
			return String("--");
		}
		return text;
	}

	void timerCallback() override
	{
		auto l = buff[0]->get_value();
		auto r = buff[1]->get_value();
		function<String(double)> print;

		auto calibrate = cal_checkbox.getToggleState() && table_cals.selected != -1;
		if (calibrate)
		{
			l *= table_cals.coef()[0]; // bug: при неучтённом префиксе
			r *= table_cals.coef()[1];
			print = prefix_v;
		}
		else {
			l = gain2db(l) - zero[0];
			r = gain2db(r) - zero[1];
			print = [](double value) { return String(value, 3) + " dB"; };
		}
		auto b = abs(l - r);

		auto minmax_r = [&](double *stat, double val) {
			if (display(String(val, 3)) != "--")
			{
				if (stat[0] == 555) stat[0] = val;
				if (stat[1] == 555) stat[1] = val;
				stat[0] = min(stat[0], val); // bug: при вольтах, при переключении нижняя граница не похожа на правду
				stat[1] = max(stat[1], val);
			}
		};

		minmax_r(minmax[0], l);
		minmax_r(minmax[1], r);
		minmax_r(minmax[2], b);

		display(l_label_value, print(l)); l_label_value_minmax.setText(display(print(minmax[0][0])) + " .. " + display(print(minmax[0][1])), dontSendNotification);
		display(r_label_value, print(r)); r_label_value_minmax.setText(display(print(minmax[1][0])) + " .. " + display(print(minmax[1][1])), dontSendNotification);
		display(b_label_value, print(b)); b_label_value_minmax.setText(display(print(minmax[2][0])) + " .. " + display(print(minmax[2][1])), dontSendNotification);

		if (zero_button.isEnabled()       == calibrate) zero_button.setEnabled(!calibrate);
		if (reset_zero_button.isEnabled() == calibrate) reset_zero_button.setEnabled(!calibrate);
	}

private:
	TextButton zero_button, reset_zero_button, reset_minmax, cal_button;
	TextEditor cal_edit_ch[2], cal_edit_name;
	ComboBox   types_combo, outputs_combo, rates_combo, buff_size_combo, tone_combo, prefix_combo, order_combo;
	Slider     high_slider;

	Label
		buff_size_label { {}, "Buff size"   },
		cal_label       { {}, "Cal"         },
		l_label         { {}, "Left"        }, l_label_value, l_label_value_minmax,
		r_label         { {}, "Right"       }, r_label_value, r_label_value_minmax,
		balance_label   { {}, "Balance"     }, b_label_value, b_label_value_minmax,
		cal_label_add	{ {}, "Name"        },
		cal_label_ch	{ {}, "Left/Right"  };
	ToggleButton
		tone_checkbox   { "Tone"            },
		cal_checkbox    { "Use Calibration" },
		bpfH_checkbox   { "bpfH"            }, bpfL_checkbox { "bpfL" };

	table_cals_ table_cals;
	sin_        osc;

	theme::checkbox_right_text_lf theme_right_text;
	theme::checkbox_right_lf      theme_right;
	
	Random random;

	double zero[2]      = { 0, 0 };
	double minmax[3][2] = { { 555, 555 }, { 555, 555 }, { 555, 555 } };

	double sample_rate = 0.0;
	size_t order       = 120;

	unique_ptr<circular_buffer<float>>                           buff[2];
	unique_ptr<spuce::iir<spuce::float_type, spuce::float_type>> iir[2];

	mutex audio_process;	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};