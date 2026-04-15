#pragma once

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QMap>

class QProgressBar;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLineEdit;

// Downloads cover art from TheGamesDB / OpenVGDB / custom URL
// Matches by ROM filename, saves as SHA1.jpg or GameName.jpg
class CoverDownloadDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CoverDownloadDialog(const QStringList& romPaths, const QString& coversDir, QWidget* parent = nullptr);
    ~CoverDownloadDialog();

private Q_SLOTS:
    void onStartClicked();
    void onStopClicked();
    void onReplyFinished(QNetworkReply* reply);

private:
    void processNext();
    void downloadCover(const QString& romName, const QString& destPath);
    QString buildSearchUrl(const QString& gameName) const;
    QString sanitizeGameName(const QString& filename) const;
    void log(const QString& msg, bool error = false);

    QNetworkAccessManager* m_nam;
    QStringList  m_rom_paths;
    QString      m_covers_dir;
    int          m_current_index = 0;
    bool         m_running       = false;
    QNetworkReply* m_active_reply = nullptr;

    // Pending: romName -> destPath
    QList<QPair<QString,QString>> m_queue;

    // UI
    QListWidget*  m_log_list    = nullptr;
    QProgressBar* m_progress    = nullptr;
    QLabel*       m_status      = nullptr;
    QPushButton*  m_start_btn   = nullptr;
    QPushButton*  m_stop_btn    = nullptr;
    QLabel*       m_preview     = nullptr;
};
