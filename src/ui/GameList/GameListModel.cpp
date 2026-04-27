#include "GameListModel.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>
#include <algorithm>

static const int COVER_BASE_WIDTH = 120;
static const int COVER_BASE_HEIGHT = 160;

GameListModel::GameListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    // Create a styled placeholder cover
    m_placeholder_pixmap = QPixmap(COVER_BASE_WIDTH, COVER_BASE_HEIGHT);
    m_placeholder_pixmap.fill(QColor(30, 30, 35));
    QPainter painter(&m_placeholder_pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    // Dark gradient background
    QLinearGradient grad(0, 0, 0, COVER_BASE_HEIGHT);
    grad.setColorAt(0, QColor(45, 45, 55));
    grad.setColorAt(1, QColor(20, 20, 25));
    painter.fillRect(m_placeholder_pixmap.rect(), grad);
    // Border
    painter.setPen(QPen(QColor(60, 60, 70), 1));
    painter.drawRect(m_placeholder_pixmap.rect().adjusted(0, 0, -1, -1));
    // Icon-like joystick
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(55, 55, 65));
    painter.drawEllipse(QPoint(COVER_BASE_WIDTH / 2, COVER_BASE_HEIGHT / 2 - 10), 24, 24);
    // Text
    painter.setPen(QColor(100, 100, 120));
    QFont f = painter.font();
    f.setPixelSize(11);
    painter.setFont(f);
    painter.drawText(m_placeholder_pixmap.rect().adjusted(0, 20, 0, 0), Qt::AlignHCenter | Qt::AlignBottom,
        QStringLiteral("No Cover"));

    // Load console logo SVGs as 20x20 pixmaps
    auto loadSvgPixmap = [](const QString& path, int size) -> QPixmap {
        QSvgRenderer renderer(path);
        if (!renderer.isValid())
            return {};
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        renderer.render(&p);
        return pix;
    };
    m_atari2600_logo = loadSvgPixmap(QStringLiteral(":/icons/white/svg/console-atari2600.svg"), 20);
    m_atarilynx_logo = loadSvgPixmap(QStringLiteral(":/icons/white/svg/console-atarilynx.svg"), 20);
    m_atarijaguar_logo = loadSvgPixmap(QStringLiteral(":/icons/white/svg/console-atarijaguar.svg"), 20);
}

GameListModel::~GameListModel()
{
}

int GameListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

int GameListModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : Column_Count;
}

QVariant GameListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_entries.size()))
        return {};

    const GameListEntry& entry = m_entries[index.row()];

    if (role == Qt::DisplayRole)
    {
        switch (index.column())
        {
            case Column_Title:
                return entry.title;
            case Column_FileName:
                return entry.fileName;
            case Column_Size:
            {
                if (entry.fileSize >= 1024)
                    return QStringLiteral("%1 KB").arg(entry.fileSize / 1024);
                return QStringLiteral("%1 B").arg(entry.fileSize);
            }
            case Column_Mapper:
                return entry.mapperType;
            case Column_Console:
                if (entry.consoleType == ConsoleType::AtariLynx)    return QStringLiteral("Lynx");
                if (entry.consoleType == ConsoleType::AtariJaguar)   return QStringLiteral("Jaguar");
                return QStringLiteral("2600");
            default:
                return {};
        }
    }
    else if (role == Qt::DecorationRole)
    {
        if (index.column() == Column_Cover)
        {
            if (!entry.coverPixmap.isNull())
                return entry.coverPixmap.scaled(getCoverArtWidth(), getCoverArtHeight(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            return m_placeholder_pixmap.scaled(getCoverArtWidth(), getCoverArtHeight(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (index.column() == Column_Console)
        {
            if (entry.consoleType == ConsoleType::AtariLynx)   return m_atarilynx_logo;
            if (entry.consoleType == ConsoleType::AtariJaguar) return m_atarijaguar_logo;
            return m_atari2600_logo;
        }
    }
    else if (role == Qt::ToolTipRole)
    {
        return entry.path;
    }
    else if (role == Qt::UserRole)
    {
        return entry.path;
    }

    return {};
}

QVariant GameListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};

    switch (section)
    {
        case Column_Console:
            return tr("Console");
        case Column_Cover:
            return tr("Cover");
        case Column_Title:
            return tr("Title");
        case Column_FileName:
            return tr("File Name");
        case Column_Size:
            return tr("Size");
        case Column_Mapper:
            return tr("Mapper");
        default:
            return {};
    }
}

void GameListModel::refresh()
{
    beginResetModel();
    m_entries.clear();
    scanDirectories();
    endResetModel();
}

void GameListModel::refreshCovers()
{
    for (auto& entry : m_entries)
    {
        entry.coverPixmap = loadCover(entry.path);
    }
    emit dataChanged(index(0, Column_Cover), index(static_cast<int>(m_entries.size()) - 1, Column_Cover));
}

void GameListModel::setShowCoverTitles(bool enabled)
{
    m_show_titles_for_covers = enabled;
}

void GameListModel::setCoverScale(float scale)
{
    m_cover_scale = std::clamp(scale, 0.1f, 2.0f);
    emit coverScaleChanged();
}

int GameListModel::getCoverArtWidth() const
{
    return static_cast<int>(static_cast<float>(COVER_BASE_WIDTH) * m_cover_scale);
}

int GameListModel::getCoverArtHeight() const
{
    return static_cast<int>(static_cast<float>(COVER_BASE_HEIGHT) * m_cover_scale);
}

QString GameListModel::getEntryPath(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_entries.size()))
        return {};
    return m_entries[row].path;
}

void GameListModel::scanDirectories()
{
    QSettings settings;
    QStringList dirs = settings.value("GameList/Directories").toStringList();

    for (const QString& dir : dirs)
    {
        scanDirectory(dir);
    }

    // Sort by title
    std::sort(m_entries.begin(), m_entries.end(),
        [](const GameListEntry& a, const GameListEntry& b) { return a.title.toLower() < b.title.toLower(); });
}

void GameListModel::scanDirectory(const QString& dir)
{
    static const QStringList romExtensions = {"*.bin", "*.a26", "*.rom", "*.lnx", "*.lyx", "*.j64", "*.jag"};

    QDirIterator it(dir, romExtensions, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next();
        QFileInfo fi = it.fileInfo();

        GameListEntry entry;
        entry.path = fi.absoluteFilePath();
        entry.title = fi.completeBaseName();
        entry.fileName = fi.fileName();
        entry.fileSize = fi.size();

        // Detect console type by extension
        const QString suffix = fi.suffix().toLower();
        if (suffix == "lnx" || suffix == "lyx")
        {
            entry.consoleType = ConsoleType::AtariLynx;
            entry.mapperType = QStringLiteral("Lynx");
        }
        else if (suffix == "j64" || suffix == "jag")
        {
            entry.consoleType = ConsoleType::AtariJaguar;
            entry.mapperType = QStringLiteral("Jaguar");
        }
        else
        {
            entry.consoleType = ConsoleType::Atari2600;
            entry.mapperType = guessMapper(fi.size());
        }
        entry.coverPixmap = loadCover(entry.path);

        m_entries.push_back(std::move(entry));
    }
}

QPixmap GameListModel::loadCover(const QString& romPath) const
{
    // Look for covers in the .ataricovers directory at the workspace root
    // Try: .ataricovers/<baseName>.png, .ataricovers/<baseName>.jpg
    QFileInfo romInfo(romPath);
    QString baseName = romInfo.completeBaseName();

    // Search in common cover locations
    QStringList coverDirs;

    // Check settings for cover directory
    QSettings settings;
    QString coverDir = settings.value("Covers/Directory").toString();
    if (!coverDir.isEmpty())
        coverDirs << coverDir;

    // Default cover locations relative to the app
    QString appDir = QCoreApplication::applicationDirPath();
    coverDirs << appDir + "/covers";

    // Check "covers" subfolder next to the ROM
    coverDirs << romInfo.absolutePath() + "/covers";

    // Also check .ataricovers in parent directories of the ROM
    QString romDir = romInfo.absolutePath();
    QDir d(romDir);
    for (int i = 0; i < 5; i++)
    {
        if (QDir(d.absolutePath() + "/.ataricovers").exists())
            coverDirs << d.absolutePath() + "/.ataricovers";
        if (QDir(d.absolutePath() + "/covers").exists())
            coverDirs << d.absolutePath() + "/covers";
        if (!d.cdUp())
            break;
    }

    for (const QString& dir : coverDirs)
    {
        for (const QString& ext : {".png", ".jpg", ".jpeg", ".webp"})
        {
            QString coverPath = dir + "/" + baseName + ext;
            if (QFile::exists(coverPath))
            {
                QPixmap pix(coverPath);
                if (!pix.isNull())
                    return pix;
            }
        }
    }

    return {};
}

QString GameListModel::guessMapper(qint64 size) const
{
    if (size <= 4096)
        return QStringLiteral("None (2K/4K)");
    else if (size <= 8192)
        return QStringLiteral("F8 (8K)");
    else if (size <= 16384)
        return QStringLiteral("F6 (16K)");
    else if (size <= 32768)
        return QStringLiteral("F4 (32K)");
    else
        return QStringLiteral("Unknown");
}
