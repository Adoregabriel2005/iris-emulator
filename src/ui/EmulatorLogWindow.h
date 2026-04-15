#pragma once
#include <QDialog>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QScrollBar>
#include <QtMessageHandler>

// Singleton log window — instale o handler Qt e todos os qDebug/qWarning
// aparecem aqui em tempo real, mesmo quando o app crasha logo depois.
class EmulatorLogWindow : public QDialog
{
    Q_OBJECT
public:
    static EmulatorLogWindow* instance();
    static void installHandler();   // chama uma vez no main()

    // Chamado pelo handler Qt — thread-safe
    void appendMessage(QtMsgType type, const QString& msg);

private:
    explicit EmulatorLogWindow(QWidget* parent = nullptr);

    QPlainTextEdit* m_log   = nullptr;
    QCheckBox*      m_pause = nullptr;
    QCheckBox*      m_autoscroll = nullptr;
    int             m_maxLines   = 4000;
    QMutex          m_mutex;

    static EmulatorLogWindow* s_instance;
    static QtMessageHandler   s_prevHandler;
    static void qtMessageHandler(QtMsgType, const QMessageLogContext&, const QString&);
};
