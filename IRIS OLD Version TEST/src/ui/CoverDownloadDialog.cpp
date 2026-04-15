#include "CoverDownloadDialog.h"

#include <QBoxLayout>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QListWidget>
#include <QNetworkRequest>
#include <QProgressBar>
#include <QPushButton>
#include <QPixmap>
#include <QScrollBar>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QCryptographicHash>
#include <QSettings>

// Uses TheGamesDB public API (no key needed for basic search)
// Falls back to a direct image URL pattern
static const char* TGDB_SEARCH_URL = "https://api.thegamesdb.net/v1/Games/ByGameName";
static const char* TGDB_IMG_BASE   = "https://cdn.thegamesdb.net/images/original/";

CoverDownloadDialog::CoverDownloadDialog(const QStringList& romPaths, const QString& coversDir, QWidget* parent)
    : QDialog(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_rom_paths(romPaths)
    , m_covers_dir(coversDir)
{
    setWindowTitle(tr("Download Cover Art"));
    setMinimumSize(640, 480);
    resize(700, 520);

    auto* layout = new QVBoxLayout(this);

    // Header
    auto* headerLabel = new QLabel(
        tr("<b>Cover Art Downloader</b><br>"
           "<span style='color:#aaa;font-size:11px;'>"
           "Downloads cover images for your ROMs from TheGamesDB.<br>"
           "Covers are saved to your configured covers folder.</span>"), this);
    headerLabel->setWordWrap(true);
    layout->addWidget(headerLabel);

    // Covers dir display
    auto* dirRow = new QHBoxLayout();
    dirRow->addWidget(new QLabel(tr("Saving to:"), this));
    auto* dirLabel = new QLabel(m_covers_dir.isEmpty() ? tr("(not configured — set in Settings → Directories)") : m_covers_dir, this);
    dirLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    dirRow->addWidget(dirLabel, 1);
    layout->addLayout(dirRow);

    // ROM count
    auto* countLabel = new QLabel(tr("%1 ROMs found in your library.").arg(romPaths.size()), this);
    countLabel->setStyleSheet("color: #aaa; font-size: 11px;");
    layout->addWidget(countLabel);

    // Progress
    m_progress = new QProgressBar(this);
    m_progress->setRange(0, qMax(1, romPaths.size()));
    m_progress->setValue(0);
    m_progress->setTextVisible(true);
    layout->addWidget(m_progress);

    m_status = new QLabel(tr("Ready. Click Start to begin downloading."), this);
    m_status->setStyleSheet("color: #ccc; font-size: 11px;");
    layout->addWidget(m_status);

    // Log + preview side by side
    auto* midRow = new QHBoxLayout();

    m_log_list = new QListWidget(this);
    m_log_list->setStyleSheet("font-size: 11px;");
    midRow->addWidget(m_log_list, 1);

    m_preview = new QLabel(this);
    m_preview->setFixedSize(160, 200);
    m_preview->setAlignment(Qt::AlignCenter);
    m_preview->setStyleSheet("background: #1a1a2e; border: 1px solid #333; border-radius: 6px;");
    m_preview->setText(tr("Preview"));
    midRow->addWidget(m_preview);

    layout->addLayout(midRow, 1);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    m_start_btn = new QPushButton(QIcon::fromTheme("download-2-line"), tr("Start Download"), this);
    m_stop_btn  = new QPushButton(QIcon::fromTheme("stop-circle-line"), tr("Stop"), this);
    m_stop_btn->setEnabled(false);
    auto* closeBtn = new QPushButton(tr("Close"), this);

    btnRow->addWidget(m_start_btn);
    btnRow->addWidget(m_stop_btn);
    btnRow->addStretch();
    btnRow->addWidget(closeBtn);
    layout->addLayout(btnRow);

    connect(m_start_btn, &QPushButton::clicked, this, &CoverDownloadDialog::onStartClicked);
    connect(m_stop_btn,  &QPushButton::clicked, this, &CoverDownloadDialog::onStopClicked);
    connect(closeBtn,    &QPushButton::clicked, this, &QDialog::accept);
    connect(m_nam, &QNetworkAccessManager::finished, this, &CoverDownloadDialog::onReplyFinished);

    // Disable start if no covers dir
    if (m_covers_dir.isEmpty()) {
        m_start_btn->setEnabled(false);
        m_start_btn->setToolTip(tr("Configure a covers directory in Settings → Directories first."));
    }
}

CoverDownloadDialog::~CoverDownloadDialog()
{
    onStopClicked();
}

void CoverDownloadDialog::onStartClicked()
{
    if (m_covers_dir.isEmpty()) return;

    m_queue.clear();
    QDir dir(m_covers_dir);
    dir.mkpath(".");

    for (const QString& path : m_rom_paths) {
        QFileInfo fi(path);
        QString gameName = sanitizeGameName(fi.completeBaseName());
        // Destination: covers_dir/GameName.jpg
        QString dest = m_covers_dir + "/" + fi.completeBaseName() + ".jpg";
        if (QFile::exists(dest)) {
            log(tr("✓ Already exists: %1").arg(fi.completeBaseName()));
            continue;
        }
        m_queue.append({gameName, dest});
    }

    if (m_queue.isEmpty()) {
        log(tr("All covers already downloaded!"));
        return;
    }

    m_current_index = 0;
    m_running = true;
    m_start_btn->setEnabled(false);
    m_stop_btn->setEnabled(true);
    m_progress->setMaximum(m_queue.size());
    m_progress->setValue(0);
    processNext();
}

void CoverDownloadDialog::onStopClicked()
{
    m_running = false;
    if (m_active_reply) {
        m_active_reply->abort();
        m_active_reply = nullptr;
    }
    m_start_btn->setEnabled(!m_covers_dir.isEmpty());
    m_stop_btn->setEnabled(false);
    m_status->setText(tr("Stopped."));
}

void CoverDownloadDialog::processNext()
{
    if (!m_running || m_current_index >= m_queue.size()) {
        m_running = false;
        m_start_btn->setEnabled(!m_covers_dir.isEmpty());
        m_stop_btn->setEnabled(false);
        m_status->setText(tr("Done! %1 covers downloaded.").arg(m_current_index));
        return;
    }

    const auto& [gameName, destPath] = m_queue[m_current_index];
    m_status->setText(tr("Searching: %1").arg(gameName));
    m_progress->setValue(m_current_index);
    downloadCover(gameName, destPath);
}

void CoverDownloadDialog::downloadCover(const QString& gameName, const QString& destPath)
{
    // Use TheGamesDB search — free tier, no API key needed for basic queries
    // Format: https://api.thegamesdb.net/v1/Games/ByGameName?name=Pitfall!&fields=boxart
    QUrl url(TGDB_SEARCH_URL);
    QUrlQuery query;
    query.addQueryItem("name", gameName);
    query.addQueryItem("fields", "boxart");
    query.addQueryItem("filter[platform]", "22"); // 22 = Atari 2600, 61 = Atari Lynx
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "IrisEmulator/1.0");
    req.setAttribute(QNetworkRequest::User, QVariant::fromValue(QPair<QString,QString>(gameName, destPath)));

    m_active_reply = m_nam->get(req);
}

void CoverDownloadDialog::onReplyFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    if (reply != m_active_reply) return;
    m_active_reply = nullptr;

    if (!m_running) return;

    auto pair = reply->request().attribute(QNetworkRequest::User).value<QPair<QString,QString>>();
    QString gameName = pair.first;
    QString destPath = pair.second;

    if (reply->error() != QNetworkReply::NoError) {
        log(tr("✗ Network error for: %1").arg(gameName), true);
        m_current_index++;
        QTimer::singleShot(300, this, &CoverDownloadDialog::processNext);
        return;
    }

    QByteArray data = reply->readAll();

    // Check if this is a JSON search response or a direct image
    if (reply->header(QNetworkRequest::ContentTypeHeader).toString().contains("json")) {
        // Parse TheGamesDB response to get image URL
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();
        QJsonArray games = root.value("data").toObject().value("games").toArray();

        if (games.isEmpty()) {
            log(tr("✗ Not found: %1").arg(gameName));
            m_current_index++;
            QTimer::singleShot(300, this, &CoverDownloadDialog::processNext);
            return;
        }

        // Get first result's boxart
        QJsonObject game = games[0].toObject();
        int gameId = game.value("id").toInt();

        // Now fetch the boxart image URL
        QUrl imgUrl(QString("https://api.thegamesdb.net/v1/Games/Images?games_id=%1&filter[type]=boxart").arg(gameId));
        QNetworkRequest imgReq(imgUrl);
        imgReq.setHeader(QNetworkRequest::UserAgentHeader, "IrisEmulator/1.0");
        imgReq.setAttribute(QNetworkRequest::User, QVariant::fromValue(QPair<QString,QString>(gameName, destPath)));
        m_active_reply = m_nam->get(imgReq);
        return;
    }

    // Check if this is an image metadata response
    if (data.startsWith("{")) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonObject root = doc.object();
        QJsonObject images = root.value("data").toObject().value("images").toObject();

        // Get first image filename
        QString imgFile;
        for (auto it = images.begin(); it != images.end(); ++it) {
            QJsonArray arr = it.value().toArray();
            if (!arr.isEmpty()) {
                imgFile = arr[0].toObject().value("filename").toString();
                break;
            }
        }

        if (imgFile.isEmpty()) {
            log(tr("✗ No boxart for: %1").arg(gameName));
            m_current_index++;
            QTimer::singleShot(300, this, &CoverDownloadDialog::processNext);
            return;
        }

        // Download the actual image
        QUrl imgUrl(QString(TGDB_IMG_BASE) + imgFile);
        QNetworkRequest imgReq(imgUrl);
        imgReq.setHeader(QNetworkRequest::UserAgentHeader, "IrisEmulator/1.0");
        imgReq.setAttribute(QNetworkRequest::User, QVariant::fromValue(QPair<QString,QString>(gameName, destPath)));
        m_active_reply = m_nam->get(imgReq);
        return;
    }

    // It's the actual image data — save it
    QFile f(destPath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();

        // Show preview
        QPixmap pix;
        if (pix.loadFromData(data)) {
            m_preview->setPixmap(pix.scaled(160, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }

        log(tr("✓ Downloaded: %1").arg(gameName));
    } else {
        log(tr("✗ Failed to save: %1").arg(gameName), true);
    }

    m_current_index++;
    QTimer::singleShot(300, this, &CoverDownloadDialog::processNext); // small delay to avoid rate limiting
}

QString CoverDownloadDialog::sanitizeGameName(const QString& filename) const
{
    // Remove common suffixes like "(1983)", "[fixed]", "(PAL)", etc.
    QString name = filename;
    name.remove(QRegularExpression(R"(\s*\([^)]*\))"));
    name.remove(QRegularExpression(R"(\s*\[[^\]]*\])"));
    name.remove(QRegularExpression(R"(\s*~$)"));
    return name.trimmed();
}

void CoverDownloadDialog::log(const QString& msg, bool error)
{
    auto* item = new QListWidgetItem(msg, m_log_list);
    if (error)
        item->setForeground(QColor(220, 80, 80));
    else if (msg.startsWith("✓"))
        item->setForeground(QColor(80, 200, 80));
    else
        item->setForeground(QColor(180, 180, 180));

    m_log_list->scrollToBottom();
}
