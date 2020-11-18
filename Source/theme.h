#pragma once
namespace theme
{
	enum color_ids {
		plot_background,
		plot_bars,
		plot_text_dark,
		plot_graph,
		plot_graph_missed,
		plot_axiliary,
		plot_png_text,
		plot_png_background,

		marker_scan,
		outline,
	};

	static auto grey_level(uint8_t level) {
		return Colour::greyLevel(jmap((float)level, 0.0f, 256.0f, 0.0f, 1.0f));
	}

	class light : public LookAndFeel_V4 {
	private:
		const Colour white   = grey_level(255);
		const Colour lighter = grey_level(225);
		const Colour base    = grey_level(184);
		const Colour outline = grey_level(135);
		const Colour dark    = grey_level(113);
		const Colour darker  = grey_level(83);
		const Colour black   = grey_level(0);
	public:
		light() {

			auto scheme = getLightColourScheme();
			scheme.setUIColour(ColourScheme::outline, outline);
			scheme.setUIColour(ColourScheme::UIColour::defaultText, black);
			setColourScheme(scheme);

			setColour(TextEditor::ColourIds::textColourId, black);
			setColour(TextEditor::ColourIds::highlightedTextColourId, black);

			setColour(color_ids::plot_background, white);
			setColour(color_ids::plot_bars, lighter);
			setColour(color_ids::plot_text_dark, darker);
			setColour(color_ids::plot_graph, black);
			setColour(color_ids::plot_graph_missed, Colours::red.withAlpha(0.6f));
			setColour(color_ids::plot_axiliary, outline);
			setColour(color_ids::plot_png_text, black);
			setColour(color_ids::plot_png_background, white);

			setColour(color_ids::marker_scan, base);
			setColour(color_ids::outline, outline);

			setColour(ResizableWindow::backgroundColourId, grey_level(239));
		}

		void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g,
			int w, int h, int titleSpaceX, int titleSpaceW,
			const Image* icon, bool drawTitleTextOnLeft)
		{
			LookAndFeel_V4::drawDocumentWindowTitleBar(window, g, w, (int)(h * 0.95), titleSpaceX, titleSpaceW, icon, drawTitleTextOnLeft);
		}

		Font getComboBoxFont(ComboBox &) {
			return Font(13.5f);
		}
	};

	class checkbox_right_lf : public light
	{
	public:
		void drawToggleButton(Graphics& g, ToggleButton& button,
			bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
		{
			auto fontSize = jmin(15.0f, button.getHeight() * 0.75f);
			auto tickWidth = fontSize * 1.1f;

			drawTickBox(g, button, 4.0f, (button.getHeight() - tickWidth) * 0.5f,
				tickWidth, tickWidth,
				button.getToggleState(),
				button.isEnabled(),
				shouldDrawButtonAsHighlighted,
				shouldDrawButtonAsDown);

			g.setColour(button.findColour(ToggleButton::textColourId));
			g.setFont(fontSize);

			if (!button.isEnabled())
				g.setOpacity(0.5f);

			g.drawFittedText(button.getButtonText(),
				button.getLocalBounds().withTrimmedLeft(roundToInt(tickWidth) + 10)
				.withTrimmedRight(2),
				Justification::centredRight, 10);
		}
	};


} // namespace theme