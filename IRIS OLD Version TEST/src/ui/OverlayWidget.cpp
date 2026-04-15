#include "OverlayWidget.h"

#include <QtGui/QPainter>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QLabel>

OverlayWidget::OverlayWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    hide();
    setupButtons();
}

OverlayWidget::~OverlayWidget()
{
}

void OverlayWidget::setupButtons()
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto* title = new QLabel(QStringLiteral("PAUSED"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(QStringLiteral(
        "QLabel { color: white; font-size: 28px; font-weight: bold; "
        "letter-spacing: 4px; margin-bottom: 16px; }"));
    layout->addWidget(title);

    auto makeButton = [this, layout](const QString& text) -> QPushButton* {
        auto* btn = new QPushButton(text, this);
        btn->setFixedWidth(220);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(
            "QPushButton {"
            "  background-color: rgba(255,255,255,30);"
            "  color: white;"
            "  border: 1px solid rgba(255,255,255,80);"
            "  border-radius: 6px;"
            "  font-size: 14px;"
            "  font-weight: 500;"
            "  padding: 6px 16px;"
            "}"
            "QPushButton:hover {"
            "  background-color: rgba(0,120,255,150);"
            "  border-color: rgba(0,120,255,200);"
            "}"
            "QPushButton:pressed {"
            "  background-color: rgba(0,80,200,200);"
            "}"));
        layout->addWidget(btn, 0, Qt::AlignCenter);
        return btn;
    };

    auto* resumeBtn     = makeButton(tr("Resume"));
    auto* resetBtn      = makeButton(tr("Reset"));
    auto* saveStateBtn  = makeButton(tr("Save State"));
    auto* loadStateBtn  = makeButton(tr("Load State"));
    auto* screenshotBtn = makeButton(tr("Screenshot"));
    auto* settingsBtn   = makeButton(tr("Controller Settings"));
    auto* quitBtn       = makeButton(tr("Quit to Game List"));

    connect(resumeBtn,     &QPushButton::clicked, this, &OverlayWidget::resumeClicked);
    connect(resetBtn,      &QPushButton::clicked, this, &OverlayWidget::resetClicked);
    connect(saveStateBtn,  &QPushButton::clicked, this, &OverlayWidget::saveStateClicked);
    connect(loadStateBtn,  &QPushButton::clicked, this, &OverlayWidget::loadStateClicked);
    connect(screenshotBtn, &QPushButton::clicked, this, &OverlayWidget::screenshotClicked);
    connect(settingsBtn,   &QPushButton::clicked, this, &OverlayWidget::settingsClicked);
    connect(quitBtn,       &QPushButton::clicked, this, &OverlayWidget::quitClicked);
}

void OverlayWidget::showOverlay()
{
    m_visible = true;
    raise();
    show();
    setFocus();
}

void OverlayWidget::hideOverlay()
{
    m_visible = false;
    hide();
}

void OverlayWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, 180));
}

void OverlayWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        emit resumeClicked();
        return;
    }
    QWidget::keyPressEvent(event);
}
