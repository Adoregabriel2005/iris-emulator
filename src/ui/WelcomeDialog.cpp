#include "WelcomeDialog.h"
#include "SettingsDialog.h"
#include "SDLInput.h"

#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QStackedWidget>
#include <QListWidget>
#include <QFrame>
#include <QPixmap>
#include <QFont>
#include <QScrollArea>

// ─── Static helpers ───────────────────────────────────────────────────────────

bool WelcomeDialog::isFirstRun()
{
    return !QSettings().value("App/Configured", false).toBool();
}

bool WelcomeDialog::shouldShow()
{
    // Show if first run OR if user hasn't disabled the welcome dialog
    if (isFirstRun()) return true;
    return QSettings().value("App/ShowWelcomeOnStart", true).toBool();
}

void WelcomeDialog::markConfigured()
{
    QSettings().setValue("App/Configured", true);
}

// ─── Constructor ──────────────────────────────────────────────────────────────

WelcomeDialog::WelcomeDialog(SDLInput* input, bool firstRun, QWidget* parent)
    : QDialog(parent), m_firstRun(firstRun)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setModal(true);

    if (firstRun) {
        setWindowTitle(tr("Íris Emulator — Setup Assistant"));
        setMinimumSize(740, 520);
        resize(760, 540);
        buildSetupWizard(input);
    } else {
        setWindowTitle(tr("Welcome back — Íris Emulator"));
        setMinimumSize(480, 300);
        resize(500, 320);
        buildReturningGreeting();
    }
}

// ─── Returning user (simple greeting) ────────────────────────────────────────

void WelcomeDialog::buildReturningGreeting()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(14);

    // Logo + title row
    auto* headerRow = new QHBoxLayout();
    auto* logo = new QLabel(this);
    QPixmap px(":/iris_icon.svg");
    if (!px.isNull())
        logo->setPixmap(px.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logo->setFixedSize(72, 72);
    logo->setAlignment(Qt::AlignCenter);
    headerRow->addWidget(logo);
    headerRow->addSpacing(12);

    auto* textCol = new QVBoxLayout();
    auto* title = new QLabel(tr("<h2 style='margin:0;'>Welcome back!</h2>"), this);
    title->setTextFormat(Qt::RichText);
    auto* sub = new QLabel(
        tr("<span style='color:#aaa;font-size:13px;'>"
           "Íris Emulator — Atari 2600 · Lynx · Jaguar<br>"
           "Everything is configured and ready to go.</span>"), this);
    sub->setTextFormat(Qt::RichText);
    sub->setWordWrap(true);
    textCol->addWidget(title);
    textCol->addSpacing(4);
    textCol->addWidget(sub);
    textCol->addStretch();
    headerRow->addLayout(textCol, 1);
    root->addLayout(headerRow);

    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setStyleSheet("color:rgba(255,255,255,25);");
    root->addWidget(sep);

    auto* hint = new QLabel(
        tr("<span style='color:#888;font-size:12px;'>"
           "Double-click any game in the list to start playing.<br>"
           "Use the toolbar gear button to access Settings, save states, and more."
           "</span>"), this);
    hint->setTextFormat(Qt::RichText);
    hint->setWordWrap(true);
    root->addWidget(hint);

    root->addStretch();

    // "Don't show again" + Close
    m_dontShowCheck = new QCheckBox(tr("Don't show this dialog on startup"), this);
    m_dontShowCheck->setChecked(!QSettings().value("App/ShowWelcomeOnStart", true).toBool());
    root->addWidget(m_dontShowCheck);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* closeBtn = new QPushButton(tr("Let's Play!"), this);
    closeBtn->setDefault(true);
    closeBtn->setMinimumWidth(110);
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        QSettings().setValue("App/ShowWelcomeOnStart", !m_dontShowCheck->isChecked());
        accept();
    });
}

// ─── First-run setup wizard ───────────────────────────────────────────────────

// Helper: creates a styled page title label
static QLabel* makePageTitle(const QString& text, QWidget* parent)
{
    auto* lbl = new QLabel(text, parent);
    QFont f = lbl->font();
    f.setPointSize(f.pointSize() + 3);
    f.setBold(true);
    lbl->setFont(f);
    return lbl;
}

// Helper: horizontal separator
static QFrame* makeSep(QWidget* parent)
{
    auto* f = new QFrame(parent);
    f->setFrameShape(QFrame::HLine);
    f->setFrameShadow(QFrame::Sunken);
    f->setStyleSheet("color:rgba(255,255,255,25);");
    return f;
}

void WelcomeDialog::buildSetupWizard(SDLInput* input)
{
    // ── Root layout: sidebar | content ───────────────────────────────────────
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* body = new QHBoxLayout();
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);

    // ── Sidebar ───────────────────────────────────────────────────────────────
    auto* sidebar = new QWidget(this);
    sidebar->setFixedWidth(200);
    sidebar->setStyleSheet("background:#1a1a2e;");
    auto* sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(0, 0, 0, 0);
    sideLayout->setSpacing(0);

    // Logo in sidebar
    auto* sideLogoLbl = new QLabel(sidebar);
    QPixmap px(":/iris_icon.svg");
    if (!px.isNull())
        sideLogoLbl->setPixmap(px.scaled(56, 56, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    sideLogoLbl->setAlignment(Qt::AlignCenter);
    sideLogoLbl->setContentsMargins(0, 20, 0, 4);
    sideLayout->addWidget(sideLogoLbl);

    auto* sideTitle = new QLabel(tr("<b style='color:white;font-size:14px;'>Íris Emulator</b>"), sidebar);
    sideTitle->setTextFormat(Qt::RichText);
    sideTitle->setAlignment(Qt::AlignCenter);
    sideTitle->setContentsMargins(0, 0, 0, 16);
    sideLayout->addWidget(sideTitle);

    auto* sideSep = new QFrame(sidebar);
    sideSep->setFrameShape(QFrame::HLine);
    sideSep->setStyleSheet("color:rgba(255,255,255,20);");
    sideLayout->addWidget(sideSep);

    // Step list (PCSX2 style)
    m_sidebar = new QListWidget(sidebar);
    m_sidebar->setFocusPolicy(Qt::NoFocus);
    m_sidebar->setStyleSheet(
        "QListWidget { background:transparent; border:none; color:#ccc; font-size:12px; }"
        "QListWidget::item { padding:10px 16px; border-left:3px solid transparent; }"
        "QListWidget::item:selected { background:rgba(0,88,208,120); border-left:3px solid #0058d0; color:white; }"
        "QListWidget::item:hover:!selected { background:rgba(255,255,255,10); }");
    m_sidebar->setSelectionMode(QAbstractItemView::SingleSelection);
    m_sidebar->addItem(tr("  Welcome"));
    m_sidebar->addItem(tr("  ROM Folder"));
    m_sidebar->addItem(tr("  BIOS Files"));
    m_sidebar->addItem(tr("  Controls"));
    m_sidebar->addItem(tr("  Finish"));
    m_sidebar->setCurrentRow(0);
    // Sidebar is read-only — clicking doesn't navigate
    m_sidebar->setEnabled(false);
    sideLayout->addWidget(m_sidebar, 1);
    sideLayout->addStretch();

    auto* sideVer = new QLabel(tr("<span style='color:#555;font-size:10px;'>v1.17</span>"), sidebar);
    sideVer->setTextFormat(Qt::RichText);
    sideVer->setAlignment(Qt::AlignCenter);
    sideVer->setContentsMargins(0, 0, 0, 10);
    sideLayout->addWidget(sideVer);

    body->addWidget(sidebar);

    // ── Right: stacked pages ──────────────────────────────────────────────────
    auto* rightWidget = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(28, 24, 28, 16);
    rightLayout->setSpacing(0);

    m_pages = new QStackedWidget(rightWidget);

    // ── Page 0: Welcome ───────────────────────────────────────────────────────
    {
        auto* page = new QWidget();
        auto* lay  = new QVBoxLayout(page);
        lay->setSpacing(12);
        lay->addWidget(makePageTitle(tr("Welcome to Íris Emulator"), page));
        lay->addWidget(makeSep(page));
        auto* txt = new QLabel(tr(
            "<p style='font-size:13px;'>This assistant will guide you through the initial setup "
            "so you can start playing your Atari games right away.</p>"
            "<p style='font-size:13px;'>It will only take a minute.</p>"
            "<p style='color:#aaa;font-size:12px;'>"
            "Íris supports <b>Atari 2600</b>, <b>Atari Lynx</b>, and <b>Atari Jaguar</b>.<br>"
            "Built with C++17 · Qt 6 · SDL2 · Open Source (GPL-3.0).<br>"
            "Developed by <b>Gorigamia</b> &amp; <b>Aleister93MarkV</b>."
            "</p>"
            "<p style='color:#666;font-size:11px;'>"
            "ROMs and BIOS files are <b>not included</b> and cannot be provided.<br>"
            "You must own the original hardware to legally use ROM files."
            "</p>"), page);
        txt->setTextFormat(Qt::RichText);
        txt->setWordWrap(true);
        lay->addWidget(txt);
        lay->addStretch();
        m_pages->addWidget(page);
    }

    // ── Page 1: ROM Folder ────────────────────────────────────────────────────
    {
        auto* page = new QWidget();
        auto* lay  = new QVBoxLayout(page);
        lay->setSpacing(10);
        lay->addWidget(makePageTitle(tr("ROM Folder"), page));
        lay->addWidget(makeSep(page));
        auto* hint = new QLabel(tr(
            "<p style='font-size:13px;'>Select the folder where your ROM files are stored.<br>"
            "Íris will scan it automatically and build your game library.</p>"
            "<p style='color:#888;font-size:11px;'>"
            "Supported formats:<br>"
            "&nbsp;· <code>.bin .a26 .rom</code> — Atari 2600<br>"
            "&nbsp;· <code>.lnx .lyx</code> — Atari Lynx<br>"
            "&nbsp;· <code>.j64 .jag</code> — Atari Jaguar"
            "</p>"), page);
        hint->setTextFormat(Qt::RichText);
        hint->setWordWrap(true);
        lay->addWidget(hint);

        auto* row = new QHBoxLayout();
        m_romEdit = new QLineEdit(page);
        m_romEdit->setPlaceholderText(tr("Select a folder containing your ROMs..."));
        QSettings s;
        QStringList existing = s.value("GameList/Directories").toStringList();
        if (!existing.isEmpty()) m_romEdit->setText(existing.first());
        row->addWidget(m_romEdit, 1);
        auto* browse = new QPushButton(tr("Browse..."), page);
        connect(browse, &QPushButton::clicked, this, [this]() {
            QString dir = QFileDialog::getExistingDirectory(this, tr("Select ROM Folder"));
            if (!dir.isEmpty()) m_romEdit->setText(dir);
        });
        row->addWidget(browse);
        lay->addLayout(row);

        auto* note = new QLabel(tr(
            "<span style='color:#888;font-size:11px;'>"
            "You can add more folders later in Settings → Directories.</span>"), page);
        note->setTextFormat(Qt::RichText);
        lay->addWidget(note);
        lay->addStretch();
        m_pages->addWidget(page);
    }

    // ── Page 2: BIOS Files ────────────────────────────────────────────────────
    {
        auto* page = new QWidget();
        auto* lay  = new QVBoxLayout(page);
        lay->setSpacing(10);
        lay->addWidget(makePageTitle(tr("BIOS Files (Optional)"), page));
        lay->addWidget(makeSep(page));
        auto* hint = new QLabel(tr(
            "<p style='font-size:13px;'>BIOS files are <b>not required</b> for most games.<br>"
            "You can skip this step and configure them later in Settings.</p>"
            "<p style='color:#888;font-size:12px;'>"
            "The <b>Atari Lynx Boot ROM</b> (<code>lynxboot.img</code>) is only needed "
            "for some commercial Lynx games. Without it, homebrew and most games still work.<br><br>"
            "We <b>cannot provide</b> BIOS files — they are copyrighted by Atari.<br>"
            "You must dump them from your own hardware."
            "</p>"), page);
        hint->setTextFormat(Qt::RichText);
        hint->setWordWrap(true);
        lay->addWidget(hint);

        auto* lynxRow = new QHBoxLayout();
        lynxRow->addWidget(new QLabel(tr("Lynx Boot ROM:"), page));
        m_lynxEdit = new QLineEdit(page);
        m_lynxEdit->setPlaceholderText(tr("lynxboot.img — optional, can be set later"));
        m_lynxEdit->setText(QSettings().value("Lynx/BiosPath", "").toString());
        lynxRow->addWidget(m_lynxEdit, 1);
        auto* lynxBrowse = new QPushButton(tr("Browse..."), page);
        connect(lynxBrowse, &QPushButton::clicked, this, [this]() {
            QString p = QFileDialog::getOpenFileName(this, tr("Select Lynx Boot ROM"), {},
                tr("Lynx BIOS (*.img *.rom *.bin);;All Files (*)"));
            if (!p.isEmpty()) m_lynxEdit->setText(p);
        });
        lynxRow->addWidget(lynxBrowse);
        lay->addLayout(lynxRow);
        lay->addStretch();
        m_pages->addWidget(page);
    }

    // ── Page 3: Controls ─────────────────────────────────────────────────────
    {
        auto* page = new QWidget();
        auto* lay  = new QVBoxLayout(page);
        lay->setSpacing(10);
        lay->addWidget(makePageTitle(tr("Controls"), page));
        lay->addWidget(makeSep(page));
        auto* hint = new QLabel(tr(
            "<p style='font-size:13px;'>Íris works with keyboard and any USB/Bluetooth gamepad "
            "(Xbox, PlayStation, Switch Pro, generic).</p>"
            "<p style='color:#aaa;font-size:12px;'><b>Default keyboard bindings:</b></p>"), page);
        hint->setTextFormat(Qt::RichText);
        hint->setWordWrap(true);
        lay->addWidget(hint);

        auto* table = new QLabel(tr(
            "<table style='font-size:12px;color:#ccc;border-collapse:collapse;' cellpadding='5'>"
            "<tr><th align='left' style='color:#5b9bd5;'>Action</th>"
                "<th align='left' style='color:#5b9bd5;'>2600</th>"
                "<th align='left' style='color:#5b9bd5;'>Lynx</th>"
                "<th align='left' style='color:#5b9bd5;'>Jaguar</th></tr>"
            "<tr><td>Directions</td><td>Arrow keys</td><td>Arrow keys</td><td>Arrow keys</td></tr>"
            "<tr><td>Button A / Fire</td><td>Z</td><td>Z</td><td>Z</td></tr>"
            "<tr><td>Button B</td><td>—</td><td>X</td><td>X</td></tr>"
            "<tr><td>Select / Option</td><td>Tab</td><td>—</td><td>A</td></tr>"
            "<tr><td>Reset / Start / Pause</td><td>Backspace</td><td>Enter</td><td>Enter</td></tr>"
            "<tr><td>Quick Save</td><td colspan='3'>F5</td></tr>"
            "<tr><td>Quick Load</td><td colspan='3'>F7</td></tr>"
            "<tr><td>Pause overlay</td><td colspan='3'>Escape</td></tr>"
            "</table>"), page);
        table->setTextFormat(Qt::RichText);
        lay->addWidget(table);

        auto* note = new QLabel(tr(
            "<span style='color:#888;font-size:11px;'>"
            "All bindings are fully remappable in Settings → Controls.</span>"), page);
        note->setTextFormat(Qt::RichText);
        lay->addWidget(note);

        auto* openCtrlBtn = new QPushButton(tr("Open Controller Settings..."), page);
        connect(openCtrlBtn, &QPushButton::clicked, this, [input, this]() {
            SettingsDialog dlg(input, this);
            dlg.exec();
        });
        lay->addWidget(openCtrlBtn, 0, Qt::AlignLeft);
        lay->addStretch();
        m_pages->addWidget(page);
    }

    // ── Page 4: Finish ────────────────────────────────────────────────────────
    {
        auto* page = new QWidget();
        auto* lay  = new QVBoxLayout(page);
        lay->setSpacing(12);
        lay->addWidget(makePageTitle(tr("Setup Complete!"), page));
        lay->addWidget(makeSep(page));
        auto* txt = new QLabel(tr(
            "<p style='font-size:13px;'>Íris is ready to use.<br>"
            "Double-click any game in the list to start playing.</p>"
            "<p style='color:#aaa;font-size:12px;'>"
            "Quick tips:<br>"
            "&nbsp;· <b>Escape</b> while playing opens the pause overlay menu<br>"
            "&nbsp;· <b>F5</b> = quick save &nbsp;·&nbsp; <b>F7</b> = quick load<br>"
            "&nbsp;· Drag &amp; drop a ROM file onto the window to launch it instantly<br>"
            "&nbsp;· The gear button in the toolbar gives access to all settings"
            "</p>"), page);
        txt->setTextFormat(Qt::RichText);
        txt->setWordWrap(true);
        lay->addWidget(txt);
        lay->addStretch();

        m_dontShowCheck = new QCheckBox(tr("Don't show this assistant on next startup"), page);
        m_dontShowCheck->setChecked(false);
        lay->addWidget(m_dontShowCheck);
        m_pages->addWidget(page);
    }

    rightLayout->addWidget(m_pages, 1);

    // ── Navigation bar ────────────────────────────────────────────────────────
    auto* navSep = new QFrame(rightWidget);
    navSep->setFrameShape(QFrame::HLine);
    navSep->setFrameShadow(QFrame::Sunken);
    navSep->setStyleSheet("color:rgba(255,255,255,25);");
    rightLayout->addWidget(navSep);
    rightLayout->addSpacing(10);

    auto* navRow = new QHBoxLayout();
    navRow->setSpacing(8);
    m_back   = new QPushButton(tr("← Back"), rightWidget);
    m_next   = new QPushButton(tr("Next →"), rightWidget);
    m_finish = new QPushButton(tr("Start Using Íris!"), rightWidget);
    m_next->setDefault(true);
    m_finish->setDefault(true);
    m_finish->hide();
    m_back->setEnabled(false);
    m_back->setMinimumWidth(90);
    m_next->setMinimumWidth(90);
    m_finish->setMinimumWidth(140);

    navRow->addStretch();
    navRow->addWidget(m_back);
    navRow->addWidget(m_next);
    navRow->addWidget(m_finish);
    rightLayout->addLayout(navRow);

    body->addWidget(rightWidget, 1);
    root->addLayout(body, 1);

    // ── Navigation logic ──────────────────────────────────────────────────────
    connect(m_back, &QPushButton::clicked, this, [this]() {
        goToPage(m_currentPage - 1);
    });
    connect(m_next, &QPushButton::clicked, this, [this]() {
        goToPage(m_currentPage + 1);
    });
    connect(m_finish, &QPushButton::clicked, this, [this]() {
        // Save everything
        QSettings s;
        if (m_romEdit) {
            QString dir = m_romEdit->text().trimmed();
            if (!dir.isEmpty()) {
                QStringList dirs = s.value("GameList/Directories").toStringList();
                if (!dirs.contains(dir)) dirs.prepend(dir);
                s.setValue("GameList/Directories", dirs);
            }
        }
        if (m_lynxEdit && !m_lynxEdit->text().trimmed().isEmpty())
            s.setValue("Lynx/BiosPath", m_lynxEdit->text().trimmed());
        if (m_dontShowCheck)
            s.setValue("App/ShowWelcomeOnStart", !m_dontShowCheck->isChecked());
        markConfigured();
        accept();
    });

    goToPage(0);
}

void WelcomeDialog::goToPage(int index)
{
    const int total = m_pages ? m_pages->count() : 0;
    if (index < 0 || index >= total) return;
    m_currentPage = index;
    m_pages->setCurrentIndex(index);
    if (m_sidebar) m_sidebar->setCurrentRow(index);
    updateNav();
}

void WelcomeDialog::updateNav()
{
    const int total = m_pages ? m_pages->count() : 0;
    const bool isLast = (m_currentPage == total - 1);
    if (m_back)   m_back->setEnabled(m_currentPage > 0);
    if (m_next)   { m_next->setVisible(!isLast); m_next->setDefault(!isLast); }
    if (m_finish) { m_finish->setVisible(isLast); m_finish->setDefault(isLast); }
}
