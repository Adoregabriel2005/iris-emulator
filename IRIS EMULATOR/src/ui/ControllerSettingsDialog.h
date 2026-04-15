#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>
#include "SDLInput.h"

class ControllerSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ControllerSettingsDialog(SDLInput* input, QWidget* parent = nullptr);
    ~ControllerSettingsDialog();

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private Q_SLOTS:
    void onBindButtonClicked();
    void onCaptureTimerTick();
    void onResetDefaults();

private:
    void refreshBindingLabels();
    void startCapture(InputAction action);
    void stopCapture();

    SDLInput* m_input;
    QLabel* m_gamepad_label = nullptr;

    struct BindingRow {
        InputAction action;
        QLabel* label;
        QPushButton* button;
    };
    QVector<BindingRow> m_rows;

    // Capture state
    bool m_capturing = false;
    InputAction m_capture_action;
    QPushButton* m_capture_button = nullptr;
    QTimer* m_capture_timer = nullptr;
    int m_capture_countdown = 0;
};
