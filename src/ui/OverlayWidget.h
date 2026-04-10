#pragma once

#include <QtWidgets/QWidget>
#include <QtGui/QPaintEvent>
#include <QtGui/QKeyEvent>

class OverlayWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverlayWidget(QWidget* parent = nullptr);
    ~OverlayWidget();

    void showOverlay();
    void hideOverlay();
    bool isOverlayVisible() const { return m_visible; }

Q_SIGNALS:
    void resumeClicked();
    void resetClicked();
    void saveStateClicked();
    void loadStateClicked();
    void screenshotClicked();
    void settingsClicked();
    void quitClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupButtons();

    bool m_visible = false;
};
