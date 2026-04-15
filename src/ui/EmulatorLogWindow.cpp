#include "EmulatorLogWindow.h"
#include <QApplication>
#include <QMutexLocker>
#include <QMetaObject>
#include <QDateTime>

EmulatorLogWindow* EmulatorLogWindow::s_instance   = nullptr;
QtMessageHandler   EmulatorLogWindow::s_prevHandler = nullptr;

EmulatorLogWindow* EmulatorLogWindow::instance()
{
    if (!s_instance)
        s_instance = new EmulatorLogWindow(nullptr);
    return s_instance;
}

void EmulatorLogWindow::installHandler()
{
    instance(); // ensure created
    s_prevHandler = qInstallMessageHandler(qtMessageHandler);
}

void EmulatorLogWindow::qtMessageHandler(QtMsgType type,
    const QMessageLogContext& ctx, const QString& msg)
{
    // Forward to previous handler (stderr) as well
    if (s_prevHandler)
        s_prevHandler(type, ctx, msg);

    if (s_instance)
        s_instance->appendMessage(type, msg);
}

EmulatorLogWindow::EmulatorLogWindow(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    setWindowTitle(tr("Emulator Log — Íris"));
    resize(820, 480);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(m_maxLines);
    QFont f("Courier New", 9);
    f.setStyleHint(QFont::Monospace);
    m_log->setFont(f);
    m_log->setStyleSheet("QPlainTextEdit { background:#0d0d0d; color:#e0e0e0; }");
    layout->addWidget(m_log, 1);

    // Controls
    auto* ctrlRow = new QHBoxLayout();
    m_pause      = new QCheckBox(tr("Pause"), this);
    m_autoscroll = new QCheckBox(tr("Auto-scroll"), this);
    m_autoscroll->setChecked(true);

    auto* clearBtn = new QPushButton(tr("Clear"), this);
    auto* saveBtn  = new QPushButton(tr("Save…"), this);

    ctrlRow->addWidget(m_pause);
    ctrlRow->addWidget(m_autoscroll);
    ctrlRow->addStretch();
    ctrlRow->addWidget(clearBtn);
    ctrlRow->addWidget(saveBtn);
    layout->addLayout(ctrlRow);

    connect(clearBtn, &QPushButton::clicked, m_log, &QPlainTextEdit::clear);
    connect(saveBtn,  &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getSaveFileName(this, tr("Save Log"),
            QDateTime::currentDateTime().toString("'iris_log_'yyyyMMdd_HHmmss'.txt'"),
            tr("Text files (*.txt);;All files (*)"));
        if (path.isEmpty()) return;
        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
            QTextStream(&f) << m_log->toPlainText();
    });
}

void EmulatorLogWindow::appendMessage(QtMsgType type, const QString& msg)
{
    if (m_pause && m_pause->isChecked()) return;

    // Build colored HTML line
    QString color;
    QString prefix;
    switch (type) {
    case QtDebugMsg:    color = "#a0a0a0"; prefix = "[D] "; break;
    case QtInfoMsg:     color = "#80d0ff"; prefix = "[I] "; break;
    case QtWarningMsg:  color = "#ffcc00"; prefix = "[W] "; break;
    case QtCriticalMsg: color = "#ff6060"; prefix = "[C] "; break;
    case QtFatalMsg:    color = "#ff2020"; prefix = "[F] "; break;
    }

    // Must append from GUI thread
    QMetaObject::invokeMethod(this, [this, color, prefix, msg]() {
        QMutexLocker lk(&m_mutex);
        QString line = QString("<span style='color:%1'>%2%3</span>")
            .arg(color, prefix, msg.toHtmlEscaped());
        m_log->appendHtml(line);
        if (m_autoscroll && m_autoscroll->isChecked())
            m_log->verticalScrollBar()->setValue(
                m_log->verticalScrollBar()->maximum());
    }, Qt::QueuedConnection);
}
