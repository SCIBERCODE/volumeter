
// bug: провалы вверх при переключении фильтра
//         ! и, соответственно, вниз при частично заполненном буфере, что происходит при переключении параметров

// todo: регулировка масштаба с галкой автомасштрабирования
//       скорость движения графика
//       сглаживание

class component_graph : public Component,
                        public Timer {
//=================================================================================================
private:
    struct range_t
    {
        uint64_t start;
        uint64_t stop;
    };
    enum class range_types_t : size_t
    {
        tone,
        zero,
        calibration,
        high_pass,
        low_pass
    };
    struct waiting_t
    {
        bool                         running        { };
        const size_t                 timer_value_ms { 100  }; // интервал обновления на экране и одновременного декремента остатка таймера
        size_t                       remain_ms;               // декремент остатка
        size_t                       interval_ms;             // полное время ожидания
        double                       progress_value { -1.0 }; // переменная в диапазоне 0.0 .. 1.0, отслеживаемая ProgressBar контролом
        std::unique_ptr<ProgressBar> progress_bar;
        std::unique_ptr<Label>       stub_bg;                 // полупрозрачная заливка
        std::unique_ptr<Label>       stub_text;
        waiting_event_t              event;
    };

    Rectangle<int>                   _plot;
    Rectangle<int>                   _plot_indented;
    std::unique_ptr<circular_buffer> _graph_data; // bug: очищать при смене устройства
    RectangleList<int>               _placed_extremes;
    waiting_t                        _wait;
    signal                         & _signal;
    application                    & _app;
//=================================================================================================
public:
    component_graph(application &app, signal &signal) :
        _signal(signal), _app(app)
    {
        reset();

        _wait.progress_bar = std::make_unique<ProgressBar>(_wait.progress_value);
        _wait.stub_bg      = std::make_unique<Label>();
        _wait.stub_text    = std::make_unique<Label>();

        _wait.stub_bg  ->setColour(Label::backgroundColourId, Colours::white.withAlpha(0.7f));
        _wait.stub_text->setJustificationType(Justification::centred);
        _wait.stub_text->setFont(_wait.stub_text->getFont().boldened());
        _wait.stub_text->setColour(Label::backgroundColourId, Colours::white);
        _wait.stub_text->setText(get_timer_text(device_init), dontSendNotification);

        _wait.progress_bar->setColour(ProgressBar::ColourIds::backgroundColourId, Colours::transparentBlack);
        _wait.progress_bar->setPercentageDisplay(false);

        addAndMakeVisible(_wait.stub_bg.get());
        addAndMakeVisible(_wait.stub_text.get());
        addAndMakeVisible(_wait.progress_bar.get());
    }

    ~component_graph() { }

    void reset() {
        auto display_width = 0;
        for (const auto& display : Desktop::getInstance().getDisplays().displays)
            display_width += display.userArea.getWidth();

        _graph_data = std::make_unique<circular_buffer>(display_width);
    }

    void enqueue(const channel_t channel, const double value_raw) {
        if (_app.get_int(option_t::graph_paused) == 0) {
            if (isfinite(value_raw)) {
                _graph_data->enqueue(channel, value_raw);

                if (channel == RIGHT)
                    repaint(); // todo: оптимизировать
            }
        }
    }

    void start_waiting(waiting_event_t event, size_t milisec = 0) {
        _wait.remain_ms   = milisec;
        _wait.interval_ms = milisec;
        _wait.running     = true;
        _wait.event       = event;
        _wait.stub_bg     ->setVisible(true);
        _wait.stub_text   ->setVisible(true);
        _wait.progress_bar->setVisible(true);
        resized();
        startTimer(_wait.timer_value_ms);
    }

    void stop_waiting() {
        _wait.running     = false;
        _wait.stub_bg     ->setVisible(false);
        _wait.stub_text   ->setVisible(false);
        _wait.stub_text   ->setText(get_timer_text(device_init), dontSendNotification);
        _wait.progress_bar->setVisible(false);
        stopTimer();
    }

    bool is_waiting() {
        return _wait.running;
    }

    // todo: иконка взаимной зависимости каналов (единое масштабирование)
    //       настройка сглаживания графика
    /** построение линии сигнала, масштабирование и отрисовка
    */
    void draw_graph_line(const channel_t channel, Graphics& g) {
        //=========================================================================================
        auto     offset = 0.0f;
        double   value;
        Path     path;
        uint32_t buff_handle;

        if (_plot_indented.isEmpty())
            return;

        buff_handle = _graph_data->get_first_value(channel, _plot.getWidth(), value); // bug: устранить мерцание в момент закольцовывания буфера
        if (buff_handle == circular_buffer::INVALID_HANDLE)
            return;

        for ( ; ; offset++)
        {
            if (!isfinite(value)) break;
            auto x = _plot.getRight() - offset;
            auto y = static_cast<float>(-value);
            if (path.isEmpty())
                path.startNewSubPath(x, y);

            path.lineTo(x, y);

            if (!_graph_data->get_next_value(buff_handle, value))
                break;
        }
        auto length = path.getLength();
        if (!isfinite(length) || path.getLength() == 0.0f || offset == 0.0f)
            return;

        const auto subpixel_correction = 0.2f;

        auto transform = path.getTransformToScaleToFit (
            _plot_indented.getWidth()  - offset,
            _plot_indented.getY()      - subpixel_correction,
            offset,
            _plot_indented.getHeight() + subpixel_correction * 2,
            false,
            Justification::right
        );
        if (!isfinite(transform.mat00) || !isfinite(transform.mat01) || !isfinite(transform.mat02) ||
            !isfinite(transform.mat10) || !isfinite(transform.mat11) || !isfinite(transform.mat12))
            return;

        path.applyTransform(transform);
        g.setColour(channel == LEFT ? Colours::black : Colours::green);
        g.strokePath(path, PathStrokeType(1.0f + subpixel_correction));
    }

    // todo: подсветка точки треугольником на линии графика
    //       точки лишь в позиции области, подвергшейся коррекции (можно настройкой)
    /** отрисовка экстремумов (правый канал перекрывает левый)
    */
    void draw_extremes(const channel_t channel, Graphics& g) {
        //=========================================================================================
        double   value;
        String   text[2];
        auto     width    = _plot.getWidth();
        auto     extremes = _graph_data->get_extremes(channel, width);
        uint32_t buff_handle;

        for (auto extremum = MIN; extremum < EXTREMES_SIZE; extremum++)
        {
            buff_handle = _graph_data->get_first_value(channel, width, value);
            if (buff_handle == circular_buffer::INVALID_HANDLE)
                continue;

            for (size_t offset = 0; ; offset++) // todo: ! оптимизация, хранить смещение и возвращать с get_extremes, вместо попиксельного поиска
            {
                //if (round_flt(value) == round_flt(_extremes[extremum]))
                //if (is_about_equal(value, _extremes[extremum]))

                // todo: ! избавиться от индусского кода
                text[0] = _app.print(_app.do_corrections(channel, value));
                text[1] = _app.print(_app.do_corrections(channel, extremes[extremum]));

                if (!isfinite(_app.do_corrections(channel, value)))
                    break;

                if (text[0] == text[1])
                {
                    const auto text_width = g.getCurrentFont().getStringWidth(text[0]);
                    Rectangle<int> rect
                    (
                        _plot.getRight() - offset - (text_width / 2),
                        extremum == MAX ? _plot.getY() + 1 : _plot_indented.getBottom() + 1,
                        text_width,
                        13
                    );
                    int overhung[2] {
                        rect.getX() - _plot.getX() - 2 /* отступ от рамки */,
                        _plot.getRight() - rect.getRight() - 2,
                    };
                    if (overhung[0] < 0) rect.setCentre(rect.getCentreX() - overhung[0], rect.getCentreY()); // надпись упирается слева
                    if (overhung[1] < 0) rect.setCentre(rect.getCentreX() + overhung[1], rect.getCentreY()); // справа

                    // правый график превалирует над левым, экстремумы левого не отображаются вовсе в случае
                    // даже частичного перекрытия значениями правого
                    if (channel == RIGHT || (channel == LEFT && !_placed_extremes.intersectsRectangle(rect)))
                    {
                        g.setColour(Colours::white);
                        g.fillRect(rect);
                        g.setColour(channel == LEFT ? Colours::black : Colours::green);
                        g.drawText(text[0], rect, Justification::centred, false);
                    }
                    if (channel == RIGHT)
                        _placed_extremes.add(rect);

                    break; // todo: несколько ключевых частот, в зависимости от свободного места (наи-большая/меньшая выделяется шрифтом или фоном)
                }
                if (!_graph_data->get_next_value(buff_handle, value))
                    break;

            }
        }
    }

    /** расшифровка цветов графика (в настоящий момент используется подсветка названия канала
        вместо заграмождения полей графика)
    */
    void draw_legend(Graphics& g) {
        //=========================================================================================
        const size_t line_width { 25 };
        Rectangle<int> rect
        (
            _plot_indented.getX() + 4,
            _plot_indented.getBottom(),
            _plot_indented.getWidth(),
            13
        );
        for (auto channel = LEFT; channel <= RIGHT; channel++)
        {
            const auto text = __channel_name.at(channel);
            const auto text_width = g.getCurrentFont().getStringWidth(text);
            g.setColour(channel == LEFT ? Colours::black : Colours::green);
            g.drawFittedText(text, rect.removeFromLeft(text_width + theme::margin), Justification::centredLeft, 1);
            auto rect_float = rect.removeFromLeft(line_width);
            g.fillRect(rect_float.toFloat().withY(rect.getY() + 1.0f).reduced(0, 5.6f));
            rect.removeFromLeft(theme::margin);
        }
    }

    // todo: график состава групп
    //       также отображать изменение параметров, влиящих на вид графика
    /** рисование линий, заливок и прочей индикации
    */
    void draw_indications(Graphics& g) {
        //=========================================================================================
        const float dotted_pattern[2] { 2.0f, 3.0f };
        Line<int> lines[2] {
            {_plot_indented.getX(), _plot_indented.getY(),      _plot_indented.getRight(), _plot_indented.getY()      },
            {_plot_indented.getX(), _plot_indented.getBottom(), _plot_indented.getRight(), _plot_indented.getBottom() }
        };
        g.setColour(Colours::grey);
        g.drawDashedLine(lines[0].toFloat(), dotted_pattern, _countof(dotted_pattern));
        g.drawDashedLine(lines[1].toFloat(), dotted_pattern, _countof(dotted_pattern));
    }

    String get_timer_text(waiting_event_t event) {
        if (event == buffer_fill)
        {
            auto remain = _wait.remain_ms >= _wait.timer_value_ms ?
                String(_wait.remain_ms / 1000.0f, 1) + L" sec" : L"";
            return L"Waiting for the buffer to fill... " + remain;
        }
        if (event == device_init)
        {
            return L"Device initialization...";
        }
        return "";
    }

    //=============================================================================================
    // juce callbacks

    void resized() override {
        //=========================================================================================
        auto area         = getLocalBounds();
        auto text         = get_timer_text(_wait.event); // максимальной шириной считаем первую по порядку строку
        auto text_f       = _wait.stub_text->getFont();
        auto text_w       = text_f.getStringWidth(text);
        auto text_h       = roundToInt(text_f.getHeight());
        auto area_text    = area.withSizeKeepingCentre(text_w, text_h);

        _wait.stub_bg     ->setBounds(area);
        _wait.stub_text   ->setBounds(area_text);
        _wait.progress_bar->setBounds(area_text.withCentre(Point<int>(area_text.getCentreX(), area_text.getY() + text_h + theme::margin * 2)));
    }

    void paint(Graphics& g) override {
        //=========================================================================================
        _plot          = getLocalBounds();
        _plot_indented = getLocalBounds();

        _plot_indented.reduce(0, theme::graph_indent);
        g.setColour(Colours::white);
        g.fillRect(_plot);

        draw_indications(g);

        _placed_extremes.clear();
        for (auto channel = RIGHT; channel < CHANNEL_SIZE; channel--)
        {
            if (channel == LEFT  && _app.get_int(option_t::graph_left ) == 0) continue;
            if (channel == RIGHT && _app.get_int(option_t::graph_right) == 0) continue;

            draw_graph_line(channel, g);
            draw_extremes  (channel, g);
        }
        g.setColour(Colours::grey);
        g.drawRect(_plot);
    }

    /** обновление таймера на заглушке ожидания
    */
    void timerCallback() override {
        //=========================================================================================
        _wait.stub_text->setText(get_timer_text(_wait.event), dontSendNotification);
        if (_wait.remain_ms >= _wait.timer_value_ms) // bug: на бОльших частотах дискретизации
        {
            _wait.remain_ms -= _wait.timer_value_ms;
            _wait.progress_value = jmap(
                static_cast<double>(_wait.interval_ms - _wait.remain_ms),
                0.0, static_cast<double>(_wait.interval_ms),
                0.0, 1.0);
        }
        else
            _wait.progress_value = -1.0;

        repaint();
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_graph)
};

