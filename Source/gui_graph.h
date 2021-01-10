#pragma once
#include <JuceHeader.h>
#include "signal.h"

extern unique_ptr<settings_> __opt;

enum waiting_event_t {
    device_init,
    buffer_fill
};

// bug: провалы вверх при переключении фильтра
//         ! и, соответственно, вниз при частично заполненном буфере, что происходит при переключении параметров

class component_graph_ : public Component,
                         public Timer
{
protected:
    struct range_t
    {
        uint64_t start;
        uint64_t stop;
    };
    enum class range_types_t : size_t
    {
        tone,
        zero,
        //calibration, // todo: пересчитывать все значения, график не должен визуально изменяться
        high_pass,
        low_pass
    };
    struct waiting_t
    {
        bool                    running        = false;
        const size_t            timer_value_ms = 100;   // интервал обновления на экране и одновременного декремента остатка таймера
        size_t                  remain_ms;              // декремент остатка
        size_t                  interval_ms;            // полное время ожидания
        double                  progress_value = -1.0;  // переменная в диапазоне 0.0 .. 1.0, отслеживаемая ProgressBar контролом
        unique_ptr<ProgressBar> progress_bar;
        unique_ptr<Label>       stub_bg;                // полупрозрачная заливка
        unique_ptr<Label>       stub_text;
        waiting_event_t         event;
    };
private:
    Rectangle<int>       _plot;
    Rectangle<int>       _plot_indented;
    unique_ptr<double[]> _extremes;
    unique_ptr<circle_>  _graph_data; // bug: очищать при смене устройства
    RectangleList<int>   _placed_extremes;
    waiting_t            _wait;

public:
    component_graph_()
    {
        reset();

        _wait.progress_bar = make_unique<ProgressBar>(_wait.progress_value);
        _wait.stub_bg      = make_unique<Label>();
        _wait.stub_text    = make_unique<Label>();

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
    ~component_graph_() { }

    void reset() {
        auto display_width = 0;
        for (const auto& display : Desktop::getInstance().getDisplays().displays)
            display_width += display.userArea.getWidth();

        _graph_data = make_unique<circle_>(display_width);
    }

    void enqueue(const vector<double>& rms) {
        if (__opt->get_int(L"graph_paused") == 0) {
            if (isfinite(rms.at(LEFT))) {
                _graph_data->enqueue(LEFT,  static_cast<float>(rms.at(LEFT )));
                _graph_data->enqueue(RIGHT, static_cast<float>(rms.at(RIGHT)));
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
        auto   offset = 0.0f;
        double value;
        Path   path;

        if (_plot_indented.isEmpty())
            return;

        if (!_graph_data->get_first_value(channel, _plot.getWidth(), value)) // bug: устранить мерцание в момент закольцовывания буфера
            return;

        for ( ; ; offset++)
        {
            if (!isfinite(value)) break;
            auto  x = _plot.getRight() - offset;
            auto y = static_cast<float>(-value * (_plot_indented.getHeight() / abs(_extremes[MIN] - _extremes[MAX])));
            if (path.isEmpty())
                path.startNewSubPath(x, y);

            path.lineTo(x, y);

            if (!_graph_data->get_next_value(value))
                break;
        }
        if (path.getLength() == 0.0f || offset == 0.0f) // bug: как минимум единажды этой проверки оказалось недостаточно
            return;

        const auto subpixel_correction = 0.2f;

        // bug: масштабирование на гомогенных данных роняет алгоритм
        path.scaleToFit(
            _plot_indented.getWidth()  - offset,
            _plot_indented.getY()      - subpixel_correction,
            offset,
            _plot_indented.getHeight() + subpixel_correction * 2,
            false
        );
        g.setColour(channel == LEFT ? Colours::black : Colours::green);

        /*PathFlatteningIterator it(path);
        while (it.next());
        PathFlatteningIterator it2(path);

        size_t count = 0;
        while (it2.next()) {
            if (count > it.subPathIndex - 10) break;
            Point<float> a(it2.x1, it2.y1); Point<float> b(it2.x2, it2.y2);
            Line<float> line(a, b);
            g.drawLine(line);
            count++;
        }*/

        g.strokePath(path, PathStrokeType(1.0f + subpixel_correction));
    }

    // todo: подсветка точки треугольником на линии графика
    //       точки лишь в позиции области, подвергшейся коррекции (можно настройкой)
    /** отрисовка экстремумов (правый канал перекрывает левый)
    */
    void draw_extremes(const channel_t channel, Graphics& g) {
        //=========================================================================================
        double value;
        for (auto extremum = MIN; extremum < EXTREMES_SIZE; extremum++)
        {
            size_t offset = 0;
            if (!_graph_data->get_first_value(channel, _plot.getWidth(), value))
                continue;

            for ( ; ; offset++)
            {
                //if (round_flt(value) == round_flt(extremes.at(MAX)))
                //if (is_about_equal(value, extremes.at(MAX)))
                if (String(value, 3) == String(_extremes[extremum], 3)) // todo: индостан
                {
                    const auto text = String(value, 3) + L" dB";
                    const auto text_width = g.getCurrentFont().getStringWidth(text);
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
                        g.drawText(text, rect, Justification::centred, false);
                    }
                    if (channel == RIGHT)
                        _placed_extremes.add(rect);

                    break; // todo: несколько ключевых частот, в зависимости от свободного места
                }
                if (!_graph_data->get_next_value(value))
                    break;

                if (!isfinite(value))
                    break;
            }
        }
    }

    /** расшифровка цветов графика, в настоящий момент используется подсветка названия канала
        вместо заграмождения полей графика
    */
    void draw_legend(Graphics& g) {
        //=========================================================================================
        const size_t line_width = 25;
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
    void resized() override {
        //=========================================================================================
        auto area         = getLocalBounds();
        auto text         = get_timer_text(_wait.event); // максимальной шириной считаем первую по порядку строку
        auto text_w       = _wait.stub_text->getFont().getStringWidth(text);
        auto text_h       = roundToInt(_wait.stub_text->getFont().getHeight());
        auto area_text    = area.withSizeKeepingCentre(text_w, text_h);

        _wait.stub_bg     ->setBounds(area);
        _wait.stub_text   ->setBounds(area_text);
        _wait.progress_bar->setBounds(area_text.withCentre(Point<int>(area_text.getCentreX(), area_text.getY() + text_h + theme::margin * 2)));
    }

    //=============================================================================================
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
            if (channel == LEFT  && __opt->get_int(L"graph_left" ) == 0) continue;
            if (channel == RIGHT && __opt->get_int(L"graph_right") == 0) continue;

            _extremes = _graph_data->get_extremes(channel, _plot.getWidth());

            draw_graph_line(channel, g);
            draw_extremes  (channel, g);
        }
        g.setColour(Colours::grey);
        g.drawRect(_plot);
    }

    //=============================================================================================
    void timerCallback() override
        //=========================================================================================
    {
        _wait.stub_text->setText(get_timer_text(_wait.event), dontSendNotification);
        if (_wait.remain_ms >= _wait.timer_value_ms)
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
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(component_graph_)

};
