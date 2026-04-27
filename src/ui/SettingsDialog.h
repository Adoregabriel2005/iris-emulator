#pragma once

#include <QtWidgets/QDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QLabel>
#include <QtCore/QTimer>
#include <QtGui/QKeyEvent>

class QVBoxLayout;
class SDLInput;

// ── Generic/2600 Controller Widget ──
class JoystickWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JoystickWidget(SDLInput* input, QWidget* parent = nullptr);
Q_SIGNALS:
    void bindingChanged();
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
private:
    struct HotZone { QRectF rect; int action; QString label; };
    QVector<HotZone> m_zones;
    SDLInput* m_input;
    int m_capture_action = -1;
    QTimer* m_capture_timer = nullptr;
    int m_capture_countdown = 0;
    void buildZones();
    void startCapture(int action);
    void stopCapture();
    void onCaptureTimerTick();
};

// ── Lynx Controller Widget ──
class LynxJoystickWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LynxJoystickWidget(SDLInput* input, QWidget* parent = nullptr);
Q_SIGNALS:
    void bindingChanged();
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
private:
    struct HotZone { QRectF rect; int action; QString label; };
    QVector<HotZone> m_zones;
    SDLInput* m_input;
    int m_capture_action = -1;
    QTimer* m_capture_timer = nullptr;
    int m_capture_countdown = 0;
    void buildZones();
    void startCapture(int action);
    void stopCapture();
    void onCaptureTimerTick();
};

// ── Jaguar Controller Widget ──
class JaguarJoystickWidget : public QWidget
{
    Q_OBJECT
public:
    explicit JaguarJoystickWidget(SDLInput* input, QWidget* parent = nullptr);
Q_SIGNALS:
    void bindingChanged();
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
private:
    struct HotZone { QRectF rect; int action; QString label; };
    QVector<HotZone> m_zones;
    SDLInput* m_input;
    int m_capture_action = -1;
    QTimer* m_capture_timer = nullptr;
    int m_capture_countdown = 0;
    void buildZones();
    void startCapture(int action);
    void stopCapture();
    void onCaptureTimerTick();
};

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(SDLInput* input, QWidget* parent = nullptr);
    ~SettingsDialog();
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
private Q_SLOTS:
    void onCategoryChanged(int currentRow);
    void onAccepted();
private:
    void createPages();
    QWidget* createVideoPage();
    QWidget* createAudioPage();
    QWidget* createControllerPage();
    QWidget* createLynxControllerPage();
    QWidget* createDirectoriesPage();
    QWidget* createInterfacePage();
    QWidget* createJaguarPage();
    QWidget* createDiscordPage();

    void addSectionTitle(QVBoxLayout* layout, const QString& title);
    void installTooltip(QWidget* widget, const QString& tip);

    SDLInput* m_input;
    QListWidget* m_category_list = nullptr;
    QStackedWidget* m_page_stack = nullptr;
    QDialogButtonBox* m_button_box = nullptr;
    QLabel* m_footer_label = nullptr;

    // Controls
    class QComboBox* m_filter_combo = nullptr;
    class QComboBox* m_tv_standard_combo = nullptr;
    class QCheckBox* m_vsync_check = nullptr;
    class QSpinBox* m_scanline_spin = nullptr;
    class QSpinBox* m_ghosting_spin = nullptr;
    class QComboBox* m_lynx_filter_combo = nullptr;
    QLabel* m_filter_preview = nullptr;

    class QComboBox* m_theme_combo = nullptr;
    class QComboBox* m_language_combo = nullptr;
    class QComboBox* m_startup_combo = nullptr;

    class QCheckBox* m_audio_enabled_check = nullptr;
    class QSlider* m_volume_slider = nullptr;
    class QComboBox* m_audio_device_combo = nullptr;
    class QSpinBox* m_audio_latency_spin = nullptr;

    class QListWidget* m_dirs_list = nullptr;
    class QLineEdit* m_covers_dir_edit = nullptr;

    JoystickWidget* m_joystick_widget = nullptr;
    LynxJoystickWidget* m_lynx_joystick_widget = nullptr;
    class QLineEdit* m_lynx_bios_path_edit = nullptr;
    class QPushButton* m_lynx_bios_browse_btn = nullptr;

    // Jaguar
    JaguarJoystickWidget* m_jag_joystick_widget = nullptr;
    class QComboBox* m_jag_video_combo = nullptr;
    class QComboBox* m_jag_bios_combo = nullptr;
    class QCheckBox* m_jag_gpu_check = nullptr;
    class QCheckBox* m_jag_dsp_check = nullptr;
    class QCheckBox* m_jaglink_check = nullptr;
    class QSpinBox* m_jaglink_port_spin = nullptr;
    class QCheckBox* m_separate_window_check = nullptr;

    QString m_original_theme;
};
