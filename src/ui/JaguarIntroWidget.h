#pragma once

#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

class JaguarIntroWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JaguarIntroWidget(QWidget* parent = nullptr);
    ~JaguarIntroWidget();

    void play();

Q_SIGNALS:
    void finished();

protected:
    void paintEvent(QPaintEvent*) override;

private Q_SLOTS:
    void onTick();

private:
    void renderFrame(QPainter& p, float t);

    QTimer*       m_timer   = nullptr;
    QElapsedTimer m_elapsed;
    bool          m_playing = false;

    static constexpr float DURATION_MS = 3200.f;
};
