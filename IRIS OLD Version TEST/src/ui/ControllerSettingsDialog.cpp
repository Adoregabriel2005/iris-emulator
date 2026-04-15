#include "ControllerSettingsDialog.h"
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QMessageBox>

ControllerSettingsDialog::ControllerSettingsDialog(SDLInput* input, QWidget* parent)
    : QDialog(parent), m_input(input)
{
    setWindowTitle(tr("Controller Settings"));
    setMinimumWidth(480);

    auto* mainLayout = new QVBoxLayout(this);

    // Gamepad status
    auto* statusGroup = new QGroupBox(tr("Gamepad Status"), this);
    auto* statusLayout = new QHBoxLayout(statusGroup);
    m_gamepad_label = new QLabel(this);
    if (m_input->hasGamepad())
        m_gamepad_label->setText(tr("Connected: %1").arg(m_input->gamepadName()));
    else
        m_gamepad_label->setText(tr("No gamepad detected. Keyboard bindings only."));
    statusLayout->addWidget(m_gamepad_label);
    mainLayout->addWidget(statusGroup);

    // Key bindings
    auto* bindGroup = new QGroupBox(tr("Key Bindings"), this);
    auto* grid = new QGridLayout(bindGroup);

    grid->addWidget(new QLabel(tr("Action"), this), 0, 0);
    grid->addWidget(new QLabel(tr("Current Binding"), this), 0, 1);
    grid->addWidget(new QLabel(tr(""), this), 0, 2);

    for (int i = 0; i < static_cast<int>(InputAction::Count); i++) {
        auto action = static_cast<InputAction>(i);
        if (isLynxAction(action))
            continue; // Lynx actions have their own page
        int row = grid->rowCount();

        auto* nameLabel = new QLabel(QString::fromUtf8(SDLInput::actionName(action)), this);
        auto* bindLabel = new QLabel(this);
        auto* bindBtn = new QPushButton(tr("Bind..."), this);

        bindBtn->setProperty("action_index", i);
        connect(bindBtn, &QPushButton::clicked, this, &ControllerSettingsDialog::onBindButtonClicked);

        grid->addWidget(nameLabel, row, 0);
        grid->addWidget(bindLabel, row, 1);
        grid->addWidget(bindBtn, row, 2);

        m_rows.push_back({action, bindLabel, bindBtn});
    }

    mainLayout->addWidget(bindGroup);

    // Hint
    auto* hintLabel = new QLabel(tr(
        "<i>Keyboard: Arrows + WASD always work as secondary directions.<br>"
        "Gamepad: D-pad + left stick always work. A = Fire, Back = Select, Start = Reset.</i>"), this);
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* resetBtn = new QPushButton(tr("Reset to Defaults"), this);
    connect(resetBtn, &QPushButton::clicked, this, &ControllerSettingsDialog::onResetDefaults);
    btnLayout->addWidget(resetBtn);
    btnLayout->addStretch();

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        m_input->saveBindings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, [this]() {
        m_input->loadBindings(); // revert
        reject();
    });
    btnLayout->addWidget(buttonBox);
    mainLayout->addLayout(btnLayout);

    // Capture timer
    m_capture_timer = new QTimer(this);
    m_capture_timer->setInterval(50);
    connect(m_capture_timer, &QTimer::timeout, this, &ControllerSettingsDialog::onCaptureTimerTick);

    refreshBindingLabels();
}

ControllerSettingsDialog::~ControllerSettingsDialog()
{
    stopCapture();
}

void ControllerSettingsDialog::refreshBindingLabels()
{
    for (auto& row : m_rows) {
        InputBinding b = m_input->getBinding(row.action);
        row.label->setText(b.toDisplayString());
    }
}

void ControllerSettingsDialog::onBindButtonClicked()
{
    auto* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    int idx = btn->property("action_index").toInt();
    startCapture(static_cast<InputAction>(idx));
}

void ControllerSettingsDialog::startCapture(InputAction action)
{
    if (m_capturing)
        stopCapture();

    m_capturing = true;
    m_capture_action = action;
    m_capture_countdown = 100; // 5 seconds at 50ms interval

    // Find the button for this action
    for (auto& row : m_rows) {
        if (row.action == action) {
            m_capture_button = row.button;
            row.label->setText(tr("Press a key or button..."));
            row.button->setEnabled(false);
            break;
        }
    }

    grabKeyboard();
    m_capture_timer->start();
}

void ControllerSettingsDialog::stopCapture()
{
    m_capturing = false;
    m_capture_timer->stop();
    releaseKeyboard();
    if (m_input) m_input->clearAllKeyStates();
    if (m_capture_button) {
        m_capture_button->setEnabled(true);
        m_capture_button = nullptr;
    }
    refreshBindingLabels();
}

void ControllerSettingsDialog::onCaptureTimerTick()
{
    if (!m_capturing) {
        stopCapture();
        return;
    }

    m_capture_countdown--;
    if (m_capture_countdown <= 0) {
        stopCapture();
        return;
    }

    InputBinding binding = m_input->pollForBinding(true, true);
    if (binding.isValid()) {
        m_input->setBinding(m_capture_action, binding);
        stopCapture();
    }
}

void ControllerSettingsDialog::onResetDefaults()
{
    m_input->resetToDefaults();
    refreshBindingLabels();
}

void ControllerSettingsDialog::keyPressEvent(QKeyEvent* event)
{
    if (m_capturing) {
        // During capture: feed key to SDLInput and eat the event
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), true);
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

void ControllerSettingsDialog::keyReleaseEvent(QKeyEvent* event)
{
    if (m_capturing) {
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), false);
        event->accept();
        return;
    }
    QDialog::keyReleaseEvent(event);
}
