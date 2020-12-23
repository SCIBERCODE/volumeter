#pragma once
#include <JuceHeader.h>

namespace theme
{
	static auto grey_level(uint8_t level) {
		return Colour::greyLevel(jmap((float)level, 0.0f, 256.0f, 0.0f, 1.0f));
	}

	class light_ : public LookAndFeel_V4 {
	private:
		const Colour white   = grey_level(255);
		const Colour outline = grey_level(135);
		const Colour black   = grey_level(0);
	public:
		light_() {
			auto scheme = getLightColourScheme();
			scheme.setUIColour(ColourScheme::outline, outline);
			scheme.setUIColour(ColourScheme::UIColour::defaultText, black);
			setColourScheme(scheme);

			setColour(TextEditor::ColourIds::textColourId, black);
			setColour(TextEditor::ColourIds::highlightedTextColourId, black);
			setColour(Slider::textBoxBackgroundColourId, white);
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

		Font getLabelFont(Label &label) {
			return label.getFont().withHeight(14.0f);
		}

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
			Font font(fontSize);
			g.setFont(button.getToggleState() ? font.boldened() : font);

			if (!button.isEnabled())
				g.setOpacity(0.5f);

			g.drawFittedText(button.getButtonText(),
				button.getLocalBounds().withTrimmedLeft(roundToInt(tickWidth) + 10)
				.withTrimmedRight(2),
				Justification::centredLeft, 10);
		}
	};

	//=========================================================================================
	class checkbox_right_text_lf_ : public light_
	//=========================================================================================
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
			Font font(14.0f);
			g.setFont(button.getToggleState() ? font.boldened() : font);

			if (!button.isEnabled())
				g.setOpacity(0.5f);

			g.drawFittedText(button.getButtonText(),
				button.getLocalBounds().withTrimmedLeft(roundToInt(tickWidth) + 10)
				.withTrimmedRight(2),
				Justification::centredRight, 10);
		}
	};

	//=========================================================================================
	class checkbox_right_lf_ : public light_
	//=========================================================================================
	{
	public:
		void drawToggleButton(Graphics& g, ToggleButton& button,
			bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
		{
			auto fontSize = jmin(15.0f, button.getHeight() * 0.75f);
			auto tickWidth = fontSize * 1.1f;

			g.setColour(button.findColour(ToggleButton::textColourId));
			Font font(fontSize);
			g.setFont(button.getToggleState() ? font.boldened() : font);

			if (!button.isEnabled())
				g.setOpacity(0.5f);

			g.drawFittedText(button.getButtonText(),
				button.getLocalBounds().withTrimmedRight(6 + (int)tickWidth),
				Justification::centredRight, 10);

			//font.getStringWidth(button.getButtonText())
			drawTickBox(g, button, button.getWidth() - tickWidth, (button.getHeight() - tickWidth) * 0.5f,
				tickWidth, tickWidth,
				button.getToggleState(),
				button.isEnabled(),
				shouldDrawButtonAsHighlighted,
				shouldDrawButtonAsDown);
		}
	};
}