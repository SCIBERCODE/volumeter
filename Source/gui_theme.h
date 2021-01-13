
namespace theme
{
    const auto MF_PI   = static_cast<float>(M_PI);
    const auto MF_PI_2 = static_cast<float>(M_PI_2);

    const size_t height       = 20;
    const size_t margin       = 5;
    const size_t label_width  = 80;
    const size_t button_width = 100;
    const size_t graph_indent = 15;
    const auto   empty        = L"--";
    const auto   font_height  = 15.0f;

    static auto grey_level(const float level) {
        return Colour::greyLevel(jmap(level, 0.0f, 256.0f, 0.0f, 1.0f));
    }

    class light_ : public LookAndFeel_V4 {
    private:
        const Colour white   = grey_level(255);
        const Colour bg      = grey_level(239);
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
            setColour(ResizableWindow::backgroundColourId, bg);

            setColour(TooltipWindow::backgroundColourId, Colours::lightyellow);
            setColour(TooltipWindow::outlineColourId, black);
            setColour(TooltipWindow::textColourId, black);
        }

        auto get_bg_color() {
            return bg;
        }

        void drawDocumentWindowTitleBar(DocumentWindow& window, Graphics& g,
            int w, int h, int titleSpaceX, int titleSpaceW,
            const Image* icon, bool drawTitleTextOnLeft)
        {
            LookAndFeel_V4::drawDocumentWindowTitleBar(window, g, w, static_cast<int>(h * 0.95), titleSpaceX, titleSpaceW, icon, drawTitleTextOnLeft);
        }

        Font getLabelFont(Label &label_component) {
            return label_component.getFont().withHeight(14.0f);
        }

        void set_header_checkbox_bounds(ToggleButton& button) {
            Font font(font_height);
            auto text_width = font.getStringWidth(button.getButtonText());
            button.setBounds(margin * 2, 0, roundToInt(text_width + font_height * 1.5f + margin * 2), height);
        }

        void drawToggleButton(Graphics& g, ToggleButton& button, bool highlighted, bool down)
        {
            auto font_size  = jmin(font_height, button.getHeight() * 0.75f);
            auto tick = font_size * 1.1f;

            g.setColour(get_bg_color());
            g.fillAll();

            drawTickBox(g, button, 4.0f, (button.getHeight() - tick) * 0.5f,
                tick, tick,
                button.getToggleState(),
                button.isEnabled(),
                highlighted,
                down);

            g.setColour(button.findColour(ToggleButton::textColourId));
            Font font(font_size);
            g.setFont(button.getToggleState() ? font.boldened() : font);

            if (!button.isEnabled())
                g.setOpacity(0.5f);

            g.drawFittedText(button.getButtonText(),
                button.getLocalBounds().withTrimmedLeft(roundToInt(tick) + 10)
                .withTrimmedRight(2),
                Justification::centredLeft, 10);
        }

        void drawGroupComponentOutline(Graphics& g, int _width, int _height,
            const String& text, const Justification& /*position*/,
            GroupComponent& group)
        {
            const auto text_edge_gap = 4.0f;

            Font f(font_height);
            Path p;
            auto y      = f.getAscent() - 2.0f;
            auto w      = static_cast<float>(_width);
            auto h      = static_cast<float>(_height) - y;
            auto cs     = jmin(5.0f, w * 0.5f, h * 0.5f);
            auto cs2    = 2.0f * cs;
            auto text_w = text.isEmpty() ? 0.0f : f.getStringWidth(text) + text_edge_gap * 2.0f;
            auto text_x = cs + text_edge_gap;

            p.startNewSubPath(text_x + text_w + 2.0f, y);
            p.lineTo(w - cs, y);

            p.addArc(w - cs2, y, cs2, cs2, 0.0f, MF_PI_2);
            p.lineTo(w, y + h - cs);

            p.addArc(w - cs2, y + h - cs2, cs2, cs2, MF_PI_2, MF_PI);
            p.lineTo(cs, y + h);

            p.addArc(0.0f, y + h - cs2, cs2, cs2, MF_PI, MF_PI * 1.5f);
            p.lineTo(0.0f, y + cs);

            p.addArc(0.0f, y, cs2, cs2, MF_PI * 1.5f, MF_PI * 2.0f);
            p.lineTo(0.0f + text_x, y);

            g.setColour(group.findColour(GroupComponent::outlineColourId));
            g.strokePath(p, PathStrokeType(1.0f));

            g.setColour(group.findColour(GroupComponent::textColourId));
            g.setFont(f);
            g.drawText(text,
                roundToInt(text_x), 2,
                roundToInt(text_w),
                roundToInt(font_height),
                Justification::centred, false
            );
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(light_)

    };

    class checkbox_left_tick_ : public light_ {
        //=========================================================================================
        public:
        void drawToggleButton(Graphics& g, ToggleButton& button,
            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto font_size  = jmin(font_height, button.getHeight() * 0.75f);
            auto tick_width = font_size * 1.1f;
            auto props      = button.getProperties();
            auto checked    = button.getToggleState();

            drawTickBox(g, button, 4.0f, (button.getHeight() - tick_width) * 0.5f,
                tick_width, tick_width,
                button.getToggleState(),
                button.isEnabled(),
                shouldDrawButtonAsHighlighted,
                shouldDrawButtonAsDown);

            auto text_color_default = button.findColour(ToggleButton::textColourId);
            auto text_color = Colour(
                static_cast<int>(props.getWithDefault("color",
                static_cast<int>(text_color_default.getARGB()))));

            Font font(14.0f);
            g.setColour(checked ? text_color      : text_color_default);
            g.setFont  (checked ? font.boldened() : font);

            if (!button.isEnabled())
                g.setOpacity(0.5f);

            g.drawFittedText(button.getButtonText(),
                button.getLocalBounds().withTrimmedLeft(roundToInt(tick_width) + 10).withTrimmedRight(2),
                Justification::centredRight, 10);

        }
    };

    /** изменение стандартного левого положения галочки у checkbox на правое,
        выделение жирным шрифтом при включении
    */
    class checkbox_right_tick_ : public light_ {
        //=========================================================================================
        public:
        void drawToggleButton(Graphics& g, ToggleButton& button,
            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
        {
            auto fontSize = jmin(font_height, button.getHeight() * 0.75f);
            auto tickWidth = fontSize * 1.1f;

            g.setColour(button.findColour(ToggleButton::textColourId));
            Font font(fontSize);
            g.setFont(button.getToggleState() ? font.boldened() : font);

            if (!button.isEnabled())
                g.setOpacity(0.5f);

            g.drawFittedText(button.getButtonText(),
                button.getLocalBounds().withTrimmedRight(6 + static_cast<int>(tickWidth)),
                Justification::centredRight, 10);

            drawTickBox(g, button, button.getWidth() - tickWidth, (button.getHeight() - tickWidth) * 0.5f,
                tickWidth, tickWidth,
                button.getToggleState(),
                button.isEnabled(),
                shouldDrawButtonAsHighlighted,
                shouldDrawButtonAsDown);
        }
    };
 }

