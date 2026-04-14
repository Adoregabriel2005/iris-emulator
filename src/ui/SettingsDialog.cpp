#include "SettingsDialog.h"
#include "ControllerSettingsDialog.h"
#include "Themes.h"
#include "SDLInput.h"
#include "../DiscordRPC.h"
#include <QGroupBox>

#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QBoxLayout>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtGui/QDesktopServices>
#include <QtCore/QUrl>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollArea>

// ========================== JoystickWidget ==========================

JoystickWidget::JoystickWidget(SDLInput* input, QWidget* parent)
    : QWidget(parent), m_input(input)
{
    setMinimumSize(340, 320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    m_capture_timer = new QTimer(this);
    m_capture_timer->setInterval(50);
    connect(m_capture_timer, &QTimer::timeout, this, &JoystickWidget::onCaptureTimerTick);

    buildZones();
}

void JoystickWidget::buildZones()
{
    m_zones.clear();
    // Zones will be built relative to widget size in paintEvent
    // These are normalized (0..1) coordinates
    m_zones = {
        {{0.35, 0.01, 0.30, 0.14}, static_cast<int>(InputAction::Up),    "UP"},
        {{0.35, 0.55, 0.30, 0.14}, static_cast<int>(InputAction::Down),  "DOWN"},
        {{0.12, 0.25, 0.18, 0.18}, static_cast<int>(InputAction::Left),  "LEFT"},
        {{0.70, 0.25, 0.18, 0.18}, static_cast<int>(InputAction::Right), "RIGHT"},
        {{0.78, 0.72, 0.20, 0.14}, static_cast<int>(InputAction::Fire),  "FIRE"},
        {{0.05, 0.78, 0.20, 0.08}, static_cast<int>(InputAction::Select),      "SELECT"},
        {{0.30, 0.78, 0.20, 0.08}, static_cast<int>(InputAction::Reset),       "RESET"},
        {{0.55, 0.78, 0.20, 0.08}, static_cast<int>(InputAction::P0DiffToggle),"DIFF"},
        {{0.05, 0.90, 0.20, 0.08}, static_cast<int>(InputAction::ColorBWToggle), "COLOR/BW"},
    };
}

void JoystickWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Joystick base (rounded dark rectangle)
    QRectF base(w * 0.05, h * 0.02, w * 0.90, h * 0.70);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(50, 50, 55));
    p.drawRoundedRect(base, 18, 18);

    // Stick well (circle in center)
    QPointF center(w * 0.50, h * 0.38);
    double wellR = qMin(w, h) * 0.18;
    p.setBrush(QColor(30, 30, 35));
    p.drawEllipse(center, wellR, wellR);

    // Stick shaft
    p.setBrush(QColor(20, 20, 20));
    p.drawEllipse(center, wellR * 0.35, wellR * 0.35);

    // Stick knob (top)
    p.setBrush(QColor(200, 60, 30));
    p.drawEllipse(center + QPointF(0, -wellR * 0.10), wellR * 0.28, wellR * 0.28);

    // Fire button (right side, big red)
    QRectF fireArea(w * 0.78, h * 0.20, w * 0.14, h * 0.35);
    p.setBrush(QColor(180, 40, 20));
    p.drawRoundedRect(fireArea, 8, 8);

    // Console switches area
    QRectF switchArea(w * 0.05, h * 0.75, w * 0.90, h * 0.24);
    p.setBrush(QColor(60, 55, 50));
    p.drawRoundedRect(switchArea, 10, 10);

    // Wood grain stripes
    p.setPen(QPen(QColor(100, 70, 40, 80), 1));
    for (int i = 0; i < 6; i++) {
        double y = switchArea.top() + 4 + i * (switchArea.height() / 7.0);
        p.drawLine(QPointF(switchArea.left() + 6, y), QPointF(switchArea.right() - 6, y));
    }
    p.setPen(Qt::NoPen);

    // Draw hot zones with bindings
    QFont labelFont = font();
    labelFont.setPixelSize(11);
    labelFont.setBold(true);
    p.setFont(labelFont);

    for (const auto& zone : m_zones) {
        QRectF r(zone.rect.x() * w, zone.rect.y() * h, zone.rect.width() * w, zone.rect.height() * h);

        bool isCapturing = (m_capture_action == zone.action);
        QColor bg = isCapturing ? QColor(0, 120, 255, 200) : QColor(0, 88, 208, 140);
        p.setBrush(bg);
        p.setPen(QPen(QColor(255, 255, 255, 100), 1));
        p.drawRoundedRect(r, 6, 6);

        // Label
        p.setPen(Qt::white);
        QString text = zone.label;
        if (isCapturing) {
            text = tr("Press...");
        } else {
            InputBinding binding = m_input->getBinding(static_cast<InputAction>(zone.action));
            if (binding.isValid())
                text += "\n" + binding.toDisplayString();
        }
        p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
}

void JoystickWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_capture_action >= 0) return; // already capturing

    int w = width(), h = height();
    QPointF pos = event->position();
    for (const auto& zone : m_zones) {
        QRectF r(zone.rect.x() * w, zone.rect.y() * h, zone.rect.width() * w, zone.rect.height() * h);
        if (r.contains(pos)) {
            startCapture(zone.action);
            return;
        }
    }
}

void JoystickWidget::startCapture(int action)
{
    m_capture_action = action;
    m_capture_countdown = 100; // 5 seconds
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    grabKeyboard();
    m_capture_timer->start();
    update();
}

void JoystickWidget::stopCapture()
{
    m_capture_action = -1;
    m_capture_timer->stop();
    releaseKeyboard();
    setFocusPolicy(Qt::NoFocus);
    if (m_input) m_input->clearAllKeyStates();
    update();
}

void JoystickWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_capture_action >= 0) {
        // During capture: feed key to SDLInput and eat the event
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), true);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void JoystickWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (m_capture_action >= 0) {
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), false);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void JoystickWidget::focusOutEvent(QFocusEvent* event)
{
    // If we lose focus during capture, stop capturing
    if (m_capture_action >= 0)
        stopCapture();
    QWidget::focusOutEvent(event);
}

void JoystickWidget::onCaptureTimerTick()
{
    if (m_capture_action < 0) { stopCapture(); return; }

    m_capture_countdown--;
    if (m_capture_countdown <= 0) { stopCapture(); return; }

    InputBinding binding = m_input->pollForBinding(true, true);
    if (binding.isValid()) {
        m_input->setBinding(static_cast<InputAction>(m_capture_action), binding);
        m_input->saveBindings();
        stopCapture();
        emit bindingChanged();
    }
}

// ========================== SettingsDialog ==========================

SettingsDialog::SettingsDialog(SDLInput* input, QWidget* parent)
    : QDialog(parent), m_input(input)
{
    setWindowTitle(tr("Settings"));
    setMinimumSize(780, 540);
    resize(850, 580);

    // Save original theme for reverting on Cancel
    {
        QSettings s;
        m_original_theme = s.value("UI/Theme", "darkfusionblue").toString();
    }

    auto* outerLayout = new QVBoxLayout(this);

    auto* bodyLayout = new QHBoxLayout();

    // Left: category list (PCSX2 sidebar style)
    m_category_list = new QListWidget(this);
    m_category_list->setFixedWidth(160);
    m_category_list->setIconSize(QSize(24, 24));
    m_category_list->setSpacing(2);
    m_category_list->setStyleSheet(QStringLiteral(
        "QListWidget { border: none; background: transparent; font-size: 13px; }"
        "QListWidget::item { padding: 8px 12px; border-radius: 6px; }"
        "QListWidget::item:selected { background-color: rgba(0, 88, 208, 150); }"
        "QListWidget::item:hover:!selected { background-color: rgba(255,255,255,20); }"));

    auto addCategory = [this](const QString& name, const QString& iconTheme) {
        auto* item = new QListWidgetItem(QIcon::fromTheme(iconTheme), name);
        m_category_list->addItem(item);
    };

    addCategory(tr("Video"), QStringLiteral("tv-2-line"));
    addCategory(tr("Audio"), QStringLiteral("volume-up-line"));
    addCategory(tr("2600 Controls"), QStringLiteral("controller-line"));
    addCategory(tr("Lynx Controls"), QStringLiteral("controller-line"));
    addCategory(tr("Directories"), QStringLiteral("folder-add-line"));
    addCategory(tr("Interface"), QStringLiteral("settings-3-line"));
    addCategory(tr("Discord"), QStringLiteral("discord-line"));

    bodyLayout->addWidget(m_category_list);

    // Right: stacked pages
    auto* rightLayout = new QVBoxLayout();

    m_page_stack = new QStackedWidget(this);
    createPages();
    rightLayout->addWidget(m_page_stack, 1);

    // Bottom buttons
    m_button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(m_button_box, &QDialogButtonBox::accepted, this, &SettingsDialog::onAccepted);
    connect(m_button_box, &QDialogButtonBox::rejected, this, [this]() {
        // Revert live theme preview if user cancels
        QSettings s;
        if (s.value("UI/Theme").toString() != m_original_theme) {
            s.setValue("UI/Theme", m_original_theme);
            Themes::UpdateApplicationTheme();
        }
        reject();
    });
    connect(m_button_box->button(QDialogButtonBox::Apply), &QPushButton::clicked,
        this, &SettingsDialog::onAccepted);
    rightLayout->addWidget(m_button_box);

    bodyLayout->addLayout(rightLayout, 1);
    outerLayout->addLayout(bodyLayout, 1);

    // Footer tooltip bar
    auto* footerSep = new QFrame(this);
    footerSep->setFrameShape(QFrame::HLine);
    footerSep->setFrameShadow(QFrame::Sunken);
    outerLayout->addWidget(footerSep);

    m_footer_label = new QLabel(tr("Hover over an option to see its description."), this);
    m_footer_label->setFixedHeight(22);
    m_footer_label->setStyleSheet(QStringLiteral(
        "QLabel { color: #aaa; font-size: 11px; padding: 2px 6px; }"));
    outerLayout->addWidget(m_footer_label);

    connect(m_category_list, &QListWidget::currentRowChanged,
        this, &SettingsDialog::onCategoryChanged);
    m_category_list->setCurrentRow(0);
}

SettingsDialog::~SettingsDialog()
{
}

bool SettingsDialog::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::Enter) {
        auto* w = qobject_cast<QWidget*>(obj);
        if (w && !w->statusTip().isEmpty())
            m_footer_label->setText(w->statusTip());
    } else if (event->type() == QEvent::Leave) {
        m_footer_label->setText(tr("Hover over an option to see its description."));
    }
    return QDialog::eventFilter(obj, event);
}

void SettingsDialog::installTooltip(QWidget* widget, const QString& tip)
{
    widget->setStatusTip(tip);
    widget->installEventFilter(this);
}

void SettingsDialog::addSectionTitle(QVBoxLayout* layout, const QString& title)
{
    if (layout->count() > 0)
        layout->addSpacing(8);

    auto* label = new QLabel(title, this);
    QFont f = label->font();
    f.setBold(true);
    f.setPointSize(f.pointSize() + 1);
    label->setFont(f);
    layout->addWidget(label);

    auto* line = new QFrame(this);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet(QStringLiteral("QFrame { color: rgba(255,255,255,30); }"));
    layout->addWidget(line);
    layout->addSpacing(2);
}

void SettingsDialog::createPages()
{
    m_page_stack->addWidget(createVideoPage());
    m_page_stack->addWidget(createAudioPage());
    m_page_stack->addWidget(createControllerPage());
    m_page_stack->addWidget(createLynxControllerPage());
    m_page_stack->addWidget(createDirectoriesPage());
    m_page_stack->addWidget(createInterfacePage());
    m_page_stack->addWidget(createDiscordPage());
}

void SettingsDialog::onCategoryChanged(int currentRow)
{
    if (currentRow >= 0 && currentRow < m_page_stack->count())
        m_page_stack->setCurrentIndex(currentRow);
}

// ==================== VIDEO PAGE ====================

QWidget* SettingsDialog::createVideoPage()
{
    QSettings settings;

    auto* page = new QWidget(this);
    auto* scroll = new QScrollArea(this);
    auto* inner = new QWidget();
    auto* layout = new QVBoxLayout(inner);
    layout->setContentsMargins(8, 4, 8, 4);
    scroll->setWidget(inner);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* pageLayout = new QVBoxLayout(page);
    pageLayout->setContentsMargins(0,0,0,0);
    pageLayout->addWidget(scroll);

    // ── Geral ──
    addSectionTitle(layout, tr("General"));

    auto* stdRow = new QHBoxLayout();
    stdRow->addWidget(new QLabel(tr("TV Standard (2600):"), inner));
    m_tv_standard_combo = new QComboBox(inner);
    m_tv_standard_combo->addItem(tr("NTSC (60 Hz)"), QStringLiteral("NTSC"));
    m_tv_standard_combo->addItem(tr("PAL (50 Hz)"),  QStringLiteral("PAL"));
    m_tv_standard_combo->addItem(tr("SECAM (50 Hz)"),QStringLiteral("SECAM"));
    m_tv_standard_combo->setCurrentIndex(m_tv_standard_combo->findData(settings.value("Video/TVStandard","NTSC").toString()));
    installTooltip(m_tv_standard_combo, tr("TV system for Atari 2600. NTSC = 60 Hz, PAL/SECAM = 50 Hz."));
    stdRow->addWidget(m_tv_standard_combo, 1);
    layout->addLayout(stdRow);

    m_vsync_check = new QCheckBox(tr("VSync"), inner);
    m_vsync_check->setChecked(settings.value("Video/VSync", true).toBool());
    installTooltip(m_vsync_check, tr("Sync to display refresh rate. Prevents tearing."));
    layout->addWidget(m_vsync_check);

    // ── Filtro Atari 2600 ──
    addSectionTitle(layout, tr("Atari 2600 — Screen Filter"));

    auto* filterArea = new QHBoxLayout();
    auto* filterOpts = new QVBoxLayout();

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel(tr("Filter:"), inner));
    m_filter_combo = new QComboBox(inner);
    m_filter_combo->addItem(tr("None (Sharp Pixels)"),  QStringLiteral("none"));
    m_filter_combo->addItem(tr("CRT Scanlines"),         QStringLiteral("scanlines"));
    m_filter_combo->addItem(tr("CRT Full (Phosphor + Vignette + RGB Mask)"), QStringLiteral("crt"));
    m_filter_combo->addItem(tr("Smooth (Bilinear)"),     QStringLiteral("smooth"));
    {
        QString cur = settings.value("Video/Filter","none").toString();
        int idx = m_filter_combo->findData(cur);
        m_filter_combo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
    installTooltip(m_filter_combo, tr("Screen filter for Atari 2600. CRT Full simulates phosphor glow, RGB mask and vignette."));
    filterRow->addWidget(m_filter_combo, 1);
    filterOpts->addLayout(filterRow);

    auto* scanRow = new QHBoxLayout();
    scanRow->addWidget(new QLabel(tr("Intensity:"), inner));
    m_scanline_spin = new QSpinBox(inner);
    m_scanline_spin->setRange(0, 100);
    m_scanline_spin->setSuffix("%");
    m_scanline_spin->setValue(settings.value("Video/ScanlineIntensity", 50).toInt());
    installTooltip(m_scanline_spin, tr("Strength of the scanline/phosphor effect. 50% is a good starting point."));
    scanRow->addWidget(m_scanline_spin, 1);
    filterOpts->addLayout(scanRow);
    filterArea->addLayout(filterOpts, 1);

    // Preview
    m_filter_preview = new QLabel(inner);
    m_filter_preview->setFixedSize(160, 120);
    m_filter_preview->setAlignment(Qt::AlignCenter);
    m_filter_preview->setStyleSheet("QLabel { background:#1a1a2e; border:1px solid #333; border-radius:6px; }");

    auto updatePreview = [this]() {
        QPixmap pix(160, 120);
        pix.fill(QColor(26,26,46));
        QPainter p(&pix);
        // Grid colorido
        p.setPen(Qt::NoPen);
        QColor cols[] = {QColor(220,50,50),QColor(50,180,50),QColor(50,100,220),QColor(220,180,50)};
        for (int r=0;r<6;r++) for (int c=0;c<8;c++) {
            p.setBrush(cols[(r+c)%4]);
            p.drawRect(c*20, r*16+4, 18, 14);
        }
        QString f = m_filter_combo->currentData().toString();
        int intensity = m_scanline_spin->value();
        if (f == "scanlines" || f == "crt") {
            // Scanlines reais: linhas escuras alternadas
            p.setBrush(QColor(0,0,0, intensity*200/100));
            for (int y=1; y<120; y+=3) p.drawRect(0,y,160,1);
            // Phosphor glow sutil
            p.setBrush(QColor(255,255,200, intensity*15/100));
            for (int y=0; y<120; y+=3) p.drawRect(0,y,160,2);
        }
        if (f == "crt") {
            // RGB mask
            for (int x=0;x<160;x+=3) {
                p.setBrush(QColor(255,0,0,intensity*20/100));   p.drawRect(x,  0,1,120);
                p.setBrush(QColor(0,0,255,intensity*20/100));   p.drawRect(x+2,0,1,120);
            }
            // Vignette
            QRadialGradient g(80,60,90);
            g.setColorAt(0,QColor(0,0,0,0)); g.setColorAt(0.6,QColor(0,0,0,40)); g.setColorAt(1,QColor(0,0,0,160));
            p.setBrush(g); p.drawRect(pix.rect());
        }
        if (f == "smooth") { p.setBrush(QColor(255,255,255,20)); p.drawRect(pix.rect()); }
        // Label
        p.setPen(QColor(255,255,255,200));
        QFont lf = p.font(); lf.setPixelSize(10); p.setFont(lf);
        QStringList names={"None","Scanlines","CRT Full","Smooth"};
        int ci = m_filter_combo->currentIndex();
        p.drawText(pix.rect().adjusted(0,0,0,-4), Qt::AlignBottom|Qt::AlignHCenter, ci<names.size()?names[ci]:"?");
        p.end();
        m_filter_preview->setPixmap(pix);
    };
    connect(m_filter_combo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, updatePreview);
    connect(m_scanline_spin, QOverload<int>::of(&QSpinBox::valueChanged),         this, updatePreview);
    updatePreview();

    auto syncScan = [this]() {
        QString f = m_filter_combo->currentData().toString();
        m_scanline_spin->setEnabled(f=="scanlines"||f=="crt");
    };
    connect(m_filter_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, syncScan);
    syncScan();

    filterArea->addWidget(m_filter_preview);
    layout->addLayout(filterArea);

    // ── Filtro Atari Lynx ──
    addSectionTitle(layout, tr("Atari Lynx — Screen Filter"));

    auto* lynxHint = new QLabel(
        tr("<i style='color:#aaa;'>The Lynx has a 160x102 backlit LCD. Use LCD Grid to simulate the pixel structure,"
           " or LCD Ghosting to replicate the motion blur of the original hardware.</i>"), inner);
    lynxHint->setWordWrap(true);
    layout->addWidget(lynxHint);

    auto* lynxFilterRow = new QHBoxLayout();
    lynxFilterRow->addWidget(new QLabel(tr("Lynx Filter:"), inner));
    m_lynx_filter_combo = new QComboBox(inner);
    m_lynx_filter_combo->addItem(tr("None (Sharp)"),                    QStringLiteral("none"));
    m_lynx_filter_combo->addItem(tr("Smooth (Bilinear)"),               QStringLiteral("smooth"));
    m_lynx_filter_combo->addItem(tr("LCD Grid (pixel structure)"),      QStringLiteral("lcd"));
    m_lynx_filter_combo->addItem(tr("LCD Ghosting (motion blur)"),      QStringLiteral("lcdblur"));
    m_lynx_filter_combo->addItem(tr("CRT Scanlines"),                   QStringLiteral("scanlines"));
    {
        QString cur = settings.value("Video/LynxFilter","smooth").toString();
        int idx = m_lynx_filter_combo->findData(cur);
        m_lynx_filter_combo->setCurrentIndex(idx >= 0 ? idx : 1);
    }
    installTooltip(m_lynx_filter_combo, tr("Screen filter applied when running Atari Lynx games."));
    lynxFilterRow->addWidget(m_lynx_filter_combo, 1);
    layout->addLayout(lynxFilterRow);

    auto* ghostRow = new QHBoxLayout();
    ghostRow->addWidget(new QLabel(tr("LCD Ghosting Strength:"), inner));
    m_ghosting_spin = new QSpinBox(inner);
    m_ghosting_spin->setRange(0, 100);
    m_ghosting_spin->setSuffix("%");
    m_ghosting_spin->setValue(settings.value("Video/LCDGhosting", 40).toInt());
    installTooltip(m_ghosting_spin, tr("How much the previous frame bleeds into the current one. Simulates the slow LCD response of the original Lynx."));
    ghostRow->addWidget(m_ghosting_spin, 1);
    layout->addLayout(ghostRow);

    auto* lcdIntRow = new QHBoxLayout();
    lcdIntRow->addWidget(new QLabel(tr("LCD Grid Intensity:"), inner));
    auto* lcdIntSpin = new QSpinBox(inner);
    lcdIntSpin->setObjectName("lynx_lcd_intensity");
    lcdIntSpin->setRange(0, 100);
    lcdIntSpin->setSuffix("%");
    lcdIntSpin->setValue(settings.value("Video/LCDGridIntensity", 60).toInt());
    installTooltip(lcdIntSpin, tr("Darkness of the gaps between LCD pixels. Higher = more visible grid."));
    lcdIntRow->addWidget(lcdIntSpin, 1);
    layout->addLayout(lcdIntRow);

    // Habilitar/desabilitar controles Lynx conforme filtro
    auto syncLynx = [this, lcdIntSpin]() {
        QString f = m_lynx_filter_combo->currentData().toString();
        m_ghosting_spin->setEnabled(f == "lcdblur");
        lcdIntSpin->setEnabled(f == "lcd" || f == "lcdblur");
    };
    connect(m_lynx_filter_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, syncLynx);
    syncLynx();

    layout->addStretch();
    return page;
}

// ==================== AUDIO PAGE ====================

QWidget* SettingsDialog::createAudioPage()
{
    QSettings settings;

    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("Audio Output"));

    m_audio_enabled_check = new QCheckBox(tr("Enable Audio"), page);
    m_audio_enabled_check->setChecked(settings.value("Audio/Enabled", true).toBool());
    installTooltip(m_audio_enabled_check, tr("Enable or disable all audio output from the emulator."));
    layout->addWidget(m_audio_enabled_check);

    auto* deviceRow = new QHBoxLayout();
    deviceRow->addWidget(new QLabel(tr("Output Device:"), page));
    m_audio_device_combo = new QComboBox(page);
    m_audio_device_combo->addItem(tr("System Default"));
    // Enumerate SDL audio devices if available
    int audioDevCount = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < audioDevCount; i++) {
        const char* name = SDL_GetAudioDeviceName(i, 0);
        if (name)
            m_audio_device_combo->addItem(QString::fromUtf8(name));
    }
    QString savedDevice = settings.value("Audio/Device", "").toString();
    int devIdx = m_audio_device_combo->findText(savedDevice);
    m_audio_device_combo->setCurrentIndex(devIdx >= 0 ? devIdx : 0);
    installTooltip(m_audio_device_combo, tr("Select the audio output device. 'System Default' uses whatever your OS reports."));
    deviceRow->addWidget(m_audio_device_combo, 1);
    layout->addLayout(deviceRow);

    auto* volRow = new QHBoxLayout();
    volRow->addWidget(new QLabel(tr("Volume:"), page));
    m_volume_slider = new QSlider(Qt::Horizontal, page);
    m_volume_slider->setRange(0, 100);
    m_volume_slider->setValue(settings.value("Audio/Volume", 80).toInt());
    installTooltip(m_volume_slider, tr("Master volume for all emulated audio (0–100)."));
    volRow->addWidget(m_volume_slider, 1);
    auto* volLabel = new QLabel(QString::number(m_volume_slider->value()) + "%", page);
    volLabel->setFixedWidth(36);
    connect(m_volume_slider, &QSlider::valueChanged, volLabel, [volLabel](int v) {
        volLabel->setText(QString::number(v) + "%");
    });
    volRow->addWidget(volLabel);
    layout->addLayout(volRow);

    addSectionTitle(layout, tr("Latency"));

    auto* latRow = new QHBoxLayout();
    latRow->addWidget(new QLabel(tr("Buffer Size:"), page));
    m_audio_latency_spin = new QSpinBox(page);
    m_audio_latency_spin->setRange(16, 256);
    m_audio_latency_spin->setSuffix(QStringLiteral(" ms"));
    m_audio_latency_spin->setValue(settings.value("Audio/Latency", 64).toInt());
    installTooltip(m_audio_latency_spin, tr("Audio buffer size in milliseconds. Lower values reduce latency but may cause crackling."));
    latRow->addWidget(m_audio_latency_spin, 1);
    layout->addLayout(latRow);

    // Enable/disable audio controls based on checkbox
    auto syncAudioEnabled = [this]() {
        bool on = m_audio_enabled_check->isChecked();
        m_audio_device_combo->setEnabled(on);
        m_volume_slider->setEnabled(on);
        m_audio_latency_spin->setEnabled(on);
    };
    connect(m_audio_enabled_check, &QCheckBox::toggled, this, syncAudioEnabled);
    syncAudioEnabled();

    layout->addStretch();
    return page;
}

// ==================== CONTROLLER PAGE ====================

QWidget* SettingsDialog::createControllerPage()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("Joystick Mapping"));

    auto* infoLabel = new QLabel(tr("Click a button on the joystick below to rebind it. Press a key or gamepad button within 5 seconds."), page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: #aaa; margin-bottom: 4px;"));
    layout->addWidget(infoLabel);

    m_joystick_widget = new JoystickWidget(m_input, page);
    layout->addWidget(m_joystick_widget, 1);

    addSectionTitle(layout, tr("Gamepad Status"));

    auto* statusRow = new QHBoxLayout();
    QLabel* gpLabel;
    if (m_input->hasGamepad())
        gpLabel = new QLabel(tr("Connected: %1").arg(m_input->gamepadName()), page);
    else
        gpLabel = new QLabel(tr("No gamepad detected. Keyboard only."), page);
    gpLabel->setStyleSheet(QStringLiteral("color: #aaa;"));
    statusRow->addWidget(gpLabel, 1);

    auto* resetBtn = new QPushButton(tr("Reset Defaults"), page);
    installTooltip(resetBtn, tr("Restore all controller bindings to their factory defaults."));
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_input->resetToDefaults();
        m_input->saveBindings();
        m_joystick_widget->update();
    });
    statusRow->addWidget(resetBtn);
    layout->addLayout(statusRow);

    return page;
}

// ==================== DIRECTORIES PAGE ====================

QWidget* SettingsDialog::createDirectoriesPage()
{
    QSettings settings;

    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("ROM Directories"));

    m_dirs_list = new QListWidget(page);
    m_dirs_list->addItems(settings.value("GameList/Directories").toStringList());
    installTooltip(m_dirs_list, tr("Folders that will be scanned for ROM files (.bin, .a26, .rom, .lnx, .lyx)."));
    layout->addWidget(m_dirs_list);

    auto* romBtnLayout = new QHBoxLayout();
    auto* addDirBtn = new QPushButton(tr("Add..."), page);
    installTooltip(addDirBtn, tr("Add a new folder to the ROM search list."));
    auto* removeDirBtn = new QPushButton(tr("Remove"), page);
    installTooltip(removeDirBtn, tr("Remove the selected folder from the search list."));
    auto* openDirBtn = new QPushButton(tr("Open in Explorer"), page);
    installTooltip(openDirBtn, tr("Open the selected ROM folder in your file manager."));

    romBtnLayout->addWidget(addDirBtn);
    romBtnLayout->addWidget(removeDirBtn);
    romBtnLayout->addWidget(openDirBtn);
    romBtnLayout->addStretch();
    layout->addLayout(romBtnLayout);

    connect(addDirBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Add ROM Directory"));
        if (!dir.isEmpty()) {
            for (int i = 0; i < m_dirs_list->count(); i++) {
                if (m_dirs_list->item(i)->text() == dir)
                    return;
            }
            m_dirs_list->addItem(dir);
        }
    });
    connect(removeDirBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_dirs_list->currentItem();
        if (item) delete item;
    });
    connect(openDirBtn, &QPushButton::clicked, this, [this]() {
        auto* item = m_dirs_list->currentItem();
        if (item)
            QDesktopServices::openUrl(QUrl::fromLocalFile(item->text()));
    });

    addSectionTitle(layout, tr("Cover Art"));

    auto* coverRow = new QHBoxLayout();
    coverRow->addWidget(new QLabel(tr("Covers folder:"), page));
    m_covers_dir_edit = new QLineEdit(page);
    m_covers_dir_edit->setText(settings.value("Covers/Directory").toString());
    m_covers_dir_edit->setPlaceholderText(tr("(auto-detect from ROM folders)"));
    installTooltip(m_covers_dir_edit, tr("Path where cover art images are stored. Leave blank to auto-detect from ROM directories."));
    coverRow->addWidget(m_covers_dir_edit, 1);

    auto* browseBtn = new QPushButton(tr("Browse..."), page);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Covers Directory"));
        if (!dir.isEmpty())
            m_covers_dir_edit->setText(dir);
    });
    coverRow->addWidget(browseBtn);

    auto* openCoverBtn = new QPushButton(tr("Open"), page);
    installTooltip(openCoverBtn, tr("Open the cover art folder in your file manager."));
    connect(openCoverBtn, &QPushButton::clicked, this, [this]() {
        QString dir = m_covers_dir_edit->text();
        if (!dir.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
    });
    coverRow->addWidget(openCoverBtn);
    layout->addLayout(coverRow);

    auto* coverHint = new QLabel(
        tr("<i>Place cover images (PNG/JPG) matching the ROM filename.<br>"
           "Example: \"Pitfall!.png\" for \"Pitfall!.bin\"</i>"), page);
    coverHint->setWordWrap(true);
    coverHint->setStyleSheet(QStringLiteral("color: #888; padding: 4px 0;"));
    layout->addWidget(coverHint);

    layout->addStretch();
    return page;
}

// ==================== INTERFACE PAGE ====================

QWidget* SettingsDialog::createInterfacePage()
{
    QSettings settings;

    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("Appearance"));

    auto* themeRow = new QHBoxLayout();
    themeRow->addWidget(new QLabel(tr("Theme:"), page));
    m_theme_combo = new QComboBox(page);
    m_theme_combo->addItem(tr("Dark Fusion Blue"), QStringLiteral("darkfusionblue"));
    m_theme_combo->addItem(tr("Dark Fusion"), QStringLiteral("darkfusion"));
    m_theme_combo->addItem(tr("Fusion (Light)"), QStringLiteral("fusion"));
    m_theme_combo->addItem(tr("Grey Matter"), QStringLiteral("GreyMatter"));
    m_theme_combo->addItem(tr("AMOLED"), QStringLiteral("AMOLED"));
    QString currentTheme = settings.value("UI/Theme", "darkfusionblue").toString();
    int themeIdx = m_theme_combo->findData(currentTheme);
    m_theme_combo->setCurrentIndex(themeIdx >= 0 ? themeIdx : 0);
    installTooltip(m_theme_combo, tr("Change the overall color scheme of the application."));
    themeRow->addWidget(m_theme_combo, 1);
    layout->addLayout(themeRow);

    // Live preview: apply theme immediately when selection changes
    connect(m_theme_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        QSettings settings;
        settings.setValue("UI/Theme", m_theme_combo->currentData().toString());
        Themes::UpdateApplicationTheme();
    });

    auto* langRow = new QHBoxLayout();
    langRow->addWidget(new QLabel(tr("Language:"), page));
    m_language_combo = new QComboBox(page);
    m_language_combo->addItem(tr("English"), QStringLiteral("en"));
    m_language_combo->addItem(tr("Português (Brasil)"), QStringLiteral("pt_BR"));
    m_language_combo->addItem(tr("Español"), QStringLiteral("es"));
    QString currentLang = settings.value("UI/Language", "en").toString();
    int langIdx = m_language_combo->findData(currentLang);
    m_language_combo->setCurrentIndex(langIdx >= 0 ? langIdx : 0);
    installTooltip(m_language_combo, tr("Select the interface language. Changes apply on next restart."));
    langRow->addWidget(m_language_combo, 1);
    layout->addLayout(langRow);

    // Show restart warning when language changes
    connect(m_language_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, currentLang]() {
        QString newLang = m_language_combo->currentData().toString();
        if (newLang != currentLang) {
            m_footer_label->setText(tr("Language change requires restarting the application to take effect."));
        }
    });

    addSectionTitle(layout, tr("Behavior"));

    auto* startupRow = new QHBoxLayout();
    startupRow->addWidget(new QLabel(tr("On ROM open:"), page));
    m_startup_combo = new QComboBox(page);
    m_startup_combo->addItem(tr("Start immediately"), QStringLiteral("start"));
    m_startup_combo->addItem(tr("Show game info first"), QStringLiteral("info"));
    QString startupVal = settings.value("UI/OnRomOpen", "start").toString();
    int startIdx = m_startup_combo->findData(startupVal);
    m_startup_combo->setCurrentIndex(startIdx >= 0 ? startIdx : 0);
    installTooltip(m_startup_combo, tr("What happens when you double-click a ROM in the game list."));
    startupRow->addWidget(m_startup_combo, 1);
    layout->addLayout(startupRow);

    layout->addStretch();
    return page;
}

// ==================== LYNX CONTROLLER PAGE ====================

// ========================== LynxJoystickWidget ==========================

LynxJoystickWidget::LynxJoystickWidget(SDLInput* input, QWidget* parent)
    : QWidget(parent), m_input(input)
{
    setMinimumSize(340, 320);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);

    m_capture_timer = new QTimer(this);
    m_capture_timer->setInterval(50);
    connect(m_capture_timer, &QTimer::timeout, this, &LynxJoystickWidget::onCaptureTimerTick);

    buildZones();
}

void LynxJoystickWidget::buildZones()
{
    m_zones.clear();
    // Accurate Atari Lynx layout matching real hardware:
    // D-pad on the left, A/B face buttons top-right and bottom-right (flip mode),
    // L shoulder top edge, R shoulder bottom edge, Start middle-right
    m_zones = {
        // D-pad (left side)
        {{0.06, 0.14, 0.12, 0.14}, static_cast<int>(InputAction::LynxUp),            "UP"},
        {{0.06, 0.52, 0.12, 0.14}, static_cast<int>(InputAction::LynxDown),          "DOWN"},
        {{0.00, 0.33, 0.10, 0.14}, static_cast<int>(InputAction::LynxLeft),          "LEFT"},
        {{0.14, 0.33, 0.10, 0.14}, static_cast<int>(InputAction::LynxRight),         "RIGHT"},
        // Face buttons — top pair (normal orientation)
        {{0.74, 0.08, 0.11, 0.14}, static_cast<int>(InputAction::LynxB),             "B"},
        {{0.86, 0.08, 0.11, 0.14}, static_cast<int>(InputAction::LynxA),             "A"},
        // Shoulder buttons
        {{0.74, 0.82, 0.11, 0.10}, static_cast<int>(InputAction::LynxLeftShoulder),  "L"},
        {{0.86, 0.82, 0.11, 0.10}, static_cast<int>(InputAction::LynxRightShoulder), "R"},
        // Start (middle-right area)
        {{0.76, 0.45, 0.18, 0.10}, static_cast<int>(InputAction::LynxStart),         "START"},
    };
}

void LynxJoystickWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // === Lynx body — landscape handheld with ergonomic shape ===
    // Main body (wide dark rectangle)
    QRectF body(w * 0.04, h * 0.06, w * 0.92, h * 0.72);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(55, 55, 58));
    p.drawRoundedRect(body, 22, 22);

    // Left grip (wider on the left where d-pad is)
    QRectF leftGrip(w * 0.00, h * 0.12, w * 0.28, h * 0.60);
    p.setBrush(QColor(50, 50, 54));
    p.drawRoundedRect(leftGrip, 18, 18);

    // Right grip
    QRectF rightGrip(w * 0.70, h * 0.12, w * 0.30, h * 0.60);
    p.setBrush(QColor(50, 50, 54));
    p.drawRoundedRect(rightGrip, 18, 18);

    // Inner bezel (darker)
    QRectF innerBody(w * 0.08, h * 0.10, w * 0.84, h * 0.64);
    p.setBrush(QColor(38, 38, 42));
    p.drawRoundedRect(innerBody, 14, 14);

    // === Screen (center) ===
    QRectF screen(w * 0.30, h * 0.15, w * 0.38, h * 0.50);
    // Screen bezel
    p.setBrush(QColor(28, 28, 32));
    p.drawRoundedRect(screen.adjusted(-3, -3, 3, 3), 6, 6);
    // Screen itself (slightly green-tinted like the real Lynx LCD)
    QLinearGradient screenGrad(screen.topLeft(), screen.bottomLeft());
    screenGrad.setColorAt(0, QColor(15, 30, 15));
    screenGrad.setColorAt(1, QColor(10, 22, 10));
    p.setBrush(screenGrad);
    p.drawRoundedRect(screen, 3, 3);

    // Screen reflection highlight
    p.setBrush(QColor(255, 255, 255, 8));
    p.drawRoundedRect(screen.adjusted(4, 4, -4, -screen.height() * 0.6), 2, 2);

    // "ATARI LYNX" text on screen
    QFont titleFont = font();
    titleFont.setPixelSize(12);
    titleFont.setBold(true);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 2);
    p.setFont(titleFont);
    p.setPen(QColor(40, 90, 40, 180));
    p.drawText(screen, Qt::AlignCenter, QStringLiteral("ATARI  LYNX"));
    p.setPen(Qt::NoPen);

    // === D-pad (left side) ===
    QPointF dpadCenter(w * 0.12, h * 0.40);
    double dpadSize = qMin(w, h) * 0.15;
    // D-pad base circle
    p.setBrush(QColor(35, 35, 38));
    p.drawEllipse(dpadCenter, dpadSize * 1.1, dpadSize * 1.1);
    // Cross
    double armW = dpadSize * 0.38;
    double armL = dpadSize * 0.95;
    p.setBrush(QColor(65, 65, 70));
    p.drawRoundedRect(QRectF(dpadCenter.x() - armW / 2, dpadCenter.y() - armL, armW, armL * 2), 3, 3);
    p.drawRoundedRect(QRectF(dpadCenter.x() - armL, dpadCenter.y() - armW / 2, armL * 2, armW), 3, 3);
    // Center nub
    p.setBrush(QColor(50, 50, 55));
    p.drawEllipse(dpadCenter, armW * 0.5, armW * 0.5);

    // === Face buttons (right side) — top pair B, A ===
    double btnR = w * 0.028;

    // Top-right B button
    p.setBrush(QColor(60, 60, 65));
    p.drawEllipse(QPointF(w * 0.79, h * 0.18), btnR * 1.3, btnR * 1.3);
    p.setBrush(QColor(45, 45, 50));
    p.drawEllipse(QPointF(w * 0.79, h * 0.18), btnR, btnR);

    // Top-right A button
    p.setBrush(QColor(60, 60, 65));
    p.drawEllipse(QPointF(w * 0.91, h * 0.18), btnR * 1.3, btnR * 1.3);
    p.setBrush(QColor(45, 45, 50));
    p.drawEllipse(QPointF(w * 0.91, h * 0.18), btnR, btnR);

    // Bottom-right B button (flip mode — decorative, same binding)
    p.setBrush(QColor(60, 60, 65));
    p.drawEllipse(QPointF(w * 0.79, h * 0.62), btnR * 1.3, btnR * 1.3);
    p.setBrush(QColor(45, 45, 50));
    p.drawEllipse(QPointF(w * 0.79, h * 0.62), btnR, btnR);

    // Bottom-right A button (flip mode — decorative)
    p.setBrush(QColor(60, 60, 65));
    p.drawEllipse(QPointF(w * 0.91, h * 0.62), btnR * 1.3, btnR * 1.3);
    p.setBrush(QColor(45, 45, 50));
    p.drawEllipse(QPointF(w * 0.91, h * 0.62), btnR, btnR);

    // Button labels on the hardware (small text near buttons)
    QFont smallFont = font();
    smallFont.setPixelSize(9);
    smallFont.setBold(true);
    p.setFont(smallFont);
    p.setPen(QColor(120, 120, 130));
    p.drawText(QRectF(w * 0.76, h * 0.03, w * 0.06, h * 0.08), Qt::AlignCenter, "B");
    p.drawText(QRectF(w * 0.88, h * 0.03, w * 0.06, h * 0.08), Qt::AlignCenter, "A");
    p.drawText(QRectF(w * 0.76, h * 0.68, w * 0.06, h * 0.08), Qt::AlignCenter, "B");
    p.drawText(QRectF(w * 0.88, h * 0.68, w * 0.06, h * 0.08), Qt::AlignCenter, "A");
    p.setPen(Qt::NoPen);

    // === Start button (small, between the button pairs on the right) ===
    p.setBrush(QColor(55, 55, 58));
    QRectF startBtn(w * 0.80, h * 0.38, w * 0.10, h * 0.06);
    p.drawRoundedRect(startBtn, 3, 3);
    p.setPen(QColor(100, 100, 110));
    smallFont.setPixelSize(8);
    p.setFont(smallFont);
    p.drawText(startBtn, Qt::AlignCenter, "START");
    p.setPen(Qt::NoPen);

    // === Shoulder buttons area (bottom strip) ===
    QRectF shoulderStrip(w * 0.10, h * 0.78, w * 0.80, h * 0.16);
    p.setBrush(QColor(42, 42, 46));
    p.drawRoundedRect(shoulderStrip, 8, 8);

    // L and R shoulder button shapes
    p.setBrush(QColor(55, 55, 60));
    QRectF lBtn(w * 0.14, h * 0.81, w * 0.14, h * 0.10);
    QRectF rBtn(w * 0.72, h * 0.81, w * 0.14, h * 0.10);
    p.drawRoundedRect(lBtn, 5, 5);
    p.drawRoundedRect(rBtn, 5, 5);
    p.setPen(QColor(100, 100, 110));
    smallFont.setPixelSize(9);
    p.setFont(smallFont);
    p.drawText(lBtn, Qt::AlignCenter, "L");
    p.drawText(rBtn, Qt::AlignCenter, "R");
    p.setPen(Qt::NoPen);

    // === Power LED (small green dot near screen, left side) ===
    p.setBrush(QColor(0, 200, 0, 160));
    p.drawEllipse(QPointF(w * 0.28, h * 0.38), 3, 3);

    // === Draw clickable hot zones with bindings ===
    QFont labelFont = font();
    labelFont.setPixelSize(11);
    labelFont.setBold(true);
    p.setFont(labelFont);

    for (const auto& zone : m_zones) {
        QRectF r(zone.rect.x() * w, zone.rect.y() * h, zone.rect.width() * w, zone.rect.height() * h);

        bool isCapturing = (m_capture_action == zone.action);
        QColor bg = isCapturing ? QColor(0, 120, 255, 200) : QColor(0, 88, 208, 140);
        p.setBrush(bg);
        p.setPen(QPen(QColor(255, 255, 255, 100), 1));
        p.drawRoundedRect(r, 6, 6);

        p.setPen(Qt::white);
        QString text = zone.label;
        if (isCapturing) {
            text = tr("Press...");
        } else {
            InputBinding binding = m_input->getBinding(static_cast<InputAction>(zone.action));
            if (binding.isValid())
                text += "\n" + binding.toDisplayString();
        }
        p.drawText(r, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
}

void LynxJoystickWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_capture_action >= 0) return;

    int w = width(), h = height();
    QPointF pos = event->position();
    for (const auto& zone : m_zones) {
        QRectF r(zone.rect.x() * w, zone.rect.y() * h, zone.rect.width() * w, zone.rect.height() * h);
        if (r.contains(pos)) {
            startCapture(zone.action);
            return;
        }
    }
}

void LynxJoystickWidget::startCapture(int action)
{
    m_capture_action = action;
    m_capture_countdown = 100; // 5 seconds
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
    grabKeyboard();
    m_capture_timer->start();
    update();
}

void LynxJoystickWidget::stopCapture()
{
    m_capture_action = -1;
    m_capture_timer->stop();
    releaseKeyboard();
    setFocusPolicy(Qt::NoFocus);
    if (m_input) m_input->clearAllKeyStates();
    update();
}

void LynxJoystickWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_capture_action >= 0) {
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), true);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void LynxJoystickWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (m_capture_action >= 0) {
        if (m_input && !event->isAutoRepeat())
            m_input->setQtKeyState(event->key(), false);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void LynxJoystickWidget::focusOutEvent(QFocusEvent* event)
{
    if (m_capture_action >= 0)
        stopCapture();
    QWidget::focusOutEvent(event);
}

void LynxJoystickWidget::onCaptureTimerTick()
{
    if (m_capture_action < 0) { stopCapture(); return; }

    m_capture_countdown--;
    if (m_capture_countdown <= 0) { stopCapture(); return; }

    InputBinding binding = m_input->pollForBinding(true, true);
    if (binding.isValid()) {
        m_input->setBinding(static_cast<InputAction>(m_capture_action), binding);
        m_input->saveBindings();
        stopCapture();
        emit bindingChanged();
    }
}

QWidget* SettingsDialog::createLynxControllerPage()
{
    QSettings settings;

    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("Atari Lynx BIOS"));

    auto* biosRow = new QHBoxLayout();
    biosRow->addWidget(new QLabel(tr("Boot ROM file:"), page));
    m_lynx_bios_path_edit = new QLineEdit(page);
    m_lynx_bios_path_edit->setText(settings.value("Lynx/BiosPath", "").toString());
    m_lynx_bios_path_edit->setPlaceholderText(tr("Select a Lynx boot ROM (e.g. lynxboot.img or custom .mig)"));
    installTooltip(m_lynx_bios_path_edit, tr("If a Lynx BIOS file is configured, it will be loaded before the cartridge ROM."));
    biosRow->addWidget(m_lynx_bios_path_edit, 1);

    m_lynx_bios_browse_btn = new QPushButton(tr("Browse..."), page);
    installTooltip(m_lynx_bios_browse_btn, tr("Choose the Lynx boot ROM file manually."));
    biosRow->addWidget(m_lynx_bios_browse_btn);
    layout->addLayout(biosRow);

    connect(m_lynx_bios_browse_btn, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Select Lynx BIOS File"), QString(),
            tr("Lynx BIOS Files (*.img *.rom *.lnx *.mig);;All Files (*)"));
        if (!path.isEmpty())
            m_lynx_bios_path_edit->setText(path);
    });

    auto* infoLabel = new QLabel(tr(
        "Click a button on the Lynx below to rebind it. "
        "The Lynx has a D-pad, two face buttons (A/B), two shoulder buttons (L/R), and Start."), page);
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet(QStringLiteral("color: #aaa; margin-bottom: 4px;"));
    layout->addWidget(infoLabel);

    m_lynx_joystick_widget = new LynxJoystickWidget(m_input, page);
    layout->addWidget(m_lynx_joystick_widget, 1);

    addSectionTitle(layout, tr("Defaults"));

    auto* defaultsLabel = new QLabel(tr(
        "<i>Keyboard: Arrows = D-pad, Z = A, X = B, A = L Shoulder, S = R Shoulder, Enter = Start<br>"
        "Gamepad: D-pad/Stick = directions, A = A, B = B, LB = L Shoulder, RB = R Shoulder, Start = Start</i>"), page);
    defaultsLabel->setWordWrap(true);
    defaultsLabel->setStyleSheet(QStringLiteral("color: #888;"));
    layout->addWidget(defaultsLabel);

    auto* resetRow = new QHBoxLayout();
    resetRow->addStretch();
    auto* resetBtn = new QPushButton(tr("Reset Lynx Defaults"), page);
    installTooltip(resetBtn, tr("Restore Lynx controller bindings to their factory defaults."));
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        for (int i = static_cast<int>(InputAction::LynxUp); i < static_cast<int>(InputAction::Count); i++) {
            auto action = static_cast<InputAction>(i);
            m_input->setBinding(action, SDLInput::defaultBinding(action));
        }
        m_input->saveBindings();
        m_lynx_joystick_widget->update();
    });
    resetRow->addWidget(resetBtn);
    layout->addLayout(resetRow);

    layout->addStretch();
    return page;
}

// ==================== DISCORD PAGE ====================

QWidget* SettingsDialog::createDiscordPage()
{
    QSettings settings;
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 4, 8, 4);

    addSectionTitle(layout, tr("Discord Rich Presence"));

    auto* enableCheck = new QCheckBox(tr("Enable Discord Rich Presence"), page);
    enableCheck->setObjectName("discord_enable");
    enableCheck->setChecked(settings.value("Discord/Enabled", true).toBool());
    installTooltip(enableCheck, tr("Show what you are playing on your Discord profile."));
    layout->addWidget(enableCheck);

    auto* previewGroup = new QGroupBox(tr("Preview"), page);
    auto* previewLayout = new QVBoxLayout(previewGroup);
    auto* card = new QLabel(page);
    card->setFixedHeight(90);
    card->setStyleSheet("QLabel { background: #2b2d31; border-radius: 8px; padding: 10px; color: white; font-size: 12px; }");
    card->setText(
        "<b style='color:#00b0f4;'>Iris Emulator</b><br>"
        "<span style='color:#dbdee1;'>Playing: California Games</span><br>"
        "<span style='color:#b5bac1;'>Atari Lynx</span><br>"
        "<span style='color:#b5bac1;'>00:12:34 elapsed</span>");
    card->setTextFormat(Qt::RichText);
    previewLayout->addWidget(card);
    auto* hint = new QLabel(tr("<i style='color:#888;'>Shows: game name, console, time playing, status</i>"), page);
    hint->setWordWrap(true);
    previewLayout->addWidget(hint);
    layout->addWidget(previewGroup);

    auto* appIdRow = new QHBoxLayout();
    appIdRow->addWidget(new QLabel(tr("App ID:"), page));
    auto* appIdEdit = new QLineEdit(page);
    appIdEdit->setObjectName("discord_appid");
    appIdEdit->setText(settings.value("Discord/AppId", IRIS_DISCORD_APP_ID).toString());
    appIdEdit->setPlaceholderText(tr("Discord Application ID"));
    installTooltip(appIdEdit, tr("Your Discord Application ID. Leave default to use the built-in one."));
    appIdRow->addWidget(appIdEdit, 1);
    layout->addLayout(appIdRow);

    auto syncEnabled = [previewGroup, appIdEdit](bool on) {
        previewGroup->setEnabled(on);
        appIdEdit->setEnabled(on);
    };
    connect(enableCheck, &QCheckBox::toggled, this, syncEnabled);
    syncEnabled(enableCheck->isChecked());

    layout->addStretch();
    return page;
}

// ==================== SAVE ====================

void SettingsDialog::onAccepted()
{
    QSettings settings;

    // Video
    settings.setValue("Video/TVStandard", m_tv_standard_combo->currentData().toString());
    settings.setValue("Video/VSync", m_vsync_check->isChecked());
    settings.setValue("Video/Filter", m_filter_combo->currentData().toString());
    settings.setValue("Video/ScanlineIntensity", m_scanline_spin->value());
    if (m_lynx_filter_combo)
        settings.setValue("Video/LynxFilter", m_lynx_filter_combo->currentData().toString());
    if (m_ghosting_spin)
        settings.setValue("Video/LCDGhosting", m_ghosting_spin->value());
    if (auto* s = findChild<QSpinBox*>("lynx_lcd_intensity"))
        settings.setValue("Video/LCDGridIntensity", s->value());

    // Audio
    settings.setValue("Audio/Enabled", m_audio_enabled_check->isChecked());
    settings.setValue("Audio/Volume", m_volume_slider->value());
    settings.setValue("Audio/Device", m_audio_device_combo->currentText());
    settings.setValue("Audio/Latency", m_audio_latency_spin->value());

    // Directories
    QStringList dirs;
    for (int i = 0; i < m_dirs_list->count(); i++)
        dirs.append(m_dirs_list->item(i)->text());
    settings.setValue("GameList/Directories", dirs);
    settings.setValue("Covers/Directory", m_covers_dir_edit->text());

    // Lynx BIOS
    if (m_lynx_bios_path_edit)
        settings.setValue("Lynx/BiosPath", m_lynx_bios_path_edit->text());

    // Interface
    QString newTheme = m_theme_combo->currentData().toString();
    settings.setValue("UI/Theme", newTheme);
    settings.setValue("UI/Language", m_language_combo->currentData().toString());
    settings.setValue("UI/OnRomOpen", m_startup_combo->currentData().toString());
    Themes::UpdateApplicationTheme();

    // Discord
    if (auto* cb = findChild<QCheckBox*>("discord_enable"))
        settings.setValue("Discord/Enabled", cb->isChecked());
    if (auto* le = findChild<QLineEdit*>("discord_appid"))
        settings.setValue("Discord/AppId", le->text());

    accept();
}
