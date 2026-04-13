#include "GameListModel.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtSvg/QSvgRenderer>
#include <algorithm>

static const int COVER_BASE_WIDTH  = 120;
static const int COVER_BASE_HEIGHT = 160;

// ─── No-Intro filename parsing ────────────────────────────────────────────────
// No-Intro format: "Title (Region) (Publisher) (flags...)"
// e.g. "Pitfall! (USA) (Activision)"
//      "Tempest 2000 (USA, Europe) (Atari Corporation)"

static QStringList extractParens(const QString& name)
{
    QStringList tokens;
    int i = 0;
    while (i < name.size()) {
        int open  = name.indexOf('(', i);
        if (open < 0) break;
        int close = name.indexOf(')', open);
        if (close < 0) break;
        tokens << name.mid(open + 1, close - open - 1).trimmed();
        i = close + 1;
    }
    return tokens;
}

static const QStringList kRegionKeywords = {
    "World","USA","Europe","Japan","Brazil","Australia",
    "France","Germany","Spain","Italy","Netherlands",
    "Korea","China","Sweden","Canada","UK"
};

static bool looksLikeRegion(const QString& t)
{
    for (const QString& r : kRegionKeywords)
        if (t.contains(r, Qt::CaseInsensitive)) return true;
    return false;
}

static bool looksLikeFlag(const QString& t)
{
    static const QStringList flags = {
        "Rev","Beta","Proto","Demo","Sample","Unl","En","Fr","De",
        "Es","It","Nl","Pt","Sv","No","Da","Fi","Pl","Ru",
        "Hack","Homebrew","Aftermarket","Pirate","BIOS"
    };
    for (const QString& f : flags)
        if (t.startsWith(f, Qt::CaseInsensitive)) return true;
    if (QRegularExpression("^[vV]?[0-9.]+$").match(t).hasMatch()) return true;
    return false;
}

QString GameListModel::cleanTitle(const QString& baseName)
{
    QString r = baseName;
    int first = r.indexOf('(');
    if (first > 0) r = r.left(first);
    return r.trimmed();
}

QString GameListModel::parseRegion(const QString& baseName)
{
    for (const QString& t : extractParens(baseName))
        if (looksLikeRegion(t)) return t;
    return {};
}

// Publisher database — loaded from publishers.txt next to the exe.
// Format: one entry per line: "Game Title=Publisher"
// Falls back to No-Intro parenthesis parsing.
static QHash<QString, QString>& publisherDb()
{
    static QHash<QString, QString> db;
    static bool loaded = false;
    if (!loaded) {
        loaded = true;
        QString path = QCoreApplication::applicationDirPath() + "/publishers.txt";
        QFile f(path);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.isEmpty() || line.startsWith('#')) continue;
                int eq = line.indexOf('=');
                if (eq > 0)
                    db.insert(line.left(eq).trimmed().toLower(),
                              line.mid(eq + 1).trimmed());
            }
        }
    }
    return db;
}

QString GameListModel::parsePublisher(const QString& baseName, ConsoleType console)
{
    Q_UNUSED(console);
    // 1. Check database by clean title
    QString title = cleanTitle(baseName).toLower();
    auto& db = publisherDb();
    if (db.contains(title)) return db.value(title);

    // 2. Fall back to No-Intro parenthesis parsing
    QStringList tokens = extractParens(baseName);
    bool skippedRegion = false;
    for (const QString& t : tokens) {
        if (!skippedRegion && looksLikeRegion(t)) { skippedRegion = true; continue; }
        if (looksLikeFlag(t) || looksLikeRegion(t)) continue;
        return t;
    }
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────

GameListModel::GameListModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_placeholder_pixmap = QPixmap(COVER_BASE_WIDTH, COVER_BASE_HEIGHT);
    m_placeholder_pixmap.fill(QColor(30, 30, 35));
    QPainter painter(&m_placeholder_pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient grad(0, 0, 0, COVER_BASE_HEIGHT);
    grad.setColorAt(0, QColor(45, 45, 55));
    grad.setColorAt(1, QColor(20, 20, 25));
    painter.fillRect(m_placeholder_pixmap.rect(), grad);
    painter.setPen(QPen(QColor(60, 60, 70), 1));
    painter.drawRect(m_placeholder_pixmap.rect().adjusted(0, 0, -1, -1));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(55, 55, 65));
    painter.drawEllipse(QPoint(COVER_BASE_WIDTH / 2, COVER_BASE_HEIGHT / 2 - 10), 24, 24);
    painter.setPen(QColor(100, 100, 120));
    QFont f = painter.font(); f.setPixelSize(11); painter.setFont(f);
    painter.drawText(m_placeholder_pixmap.rect().adjusted(0, 20, 0, 0),
        Qt::AlignHCenter | Qt::AlignBottom, QStringLiteral("No Cover"));

    auto loadSvg = [](const QString& path, int size) -> QPixmap {
        QSvgRenderer r(path);
        if (!r.isValid()) return {};
        QPixmap pix(size, size); pix.fill(Qt::transparent);
        QPainter p(&pix); r.render(&p); return pix;
    };
    m_atari2600_logo   = loadSvg(QStringLiteral(":/icons/white/svg/console-atari2600.svg"),   20);
    m_atarilynx_logo   = loadSvg(QStringLiteral(":/icons/white/svg/console-atarilynx.svg"),   20);
    m_atarijaguar_logo = loadSvg(QStringLiteral(":/icons/white/svg/console-atarijaguar.svg"), 20);
}

GameListModel::~GameListModel() {}

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

    const GameListEntry& e = m_entries[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case Column_Title:     return e.title;
            case Column_Publisher: return e.publisher;
            case Column_Region:    return e.region;
            case Column_FileName:  return e.fileName;
            case Column_Size:
                return e.fileSize >= 1024
                    ? QStringLiteral("%1 KB").arg(e.fileSize / 1024)
                    : QStringLiteral("%1 B").arg(e.fileSize);
            case Column_Mapper:  return e.mapperType;
            case Column_Console:
                if (e.consoleType == ConsoleType::AtariLynx)   return QStringLiteral("Lynx");
                if (e.consoleType == ConsoleType::AtariJaguar)  return QStringLiteral("Jaguar");
                return QStringLiteral("2600");
            default: return {};
        }
    }
    else if (role == Qt::DecorationRole) {
        if (index.column() == Column_Cover) {
            const QPixmap& src = e.coverPixmap.isNull() ? m_placeholder_pixmap : e.coverPixmap;
            return src.scaled(getCoverArtWidth(), getCoverArtHeight(),
                              Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        if (index.column() == Column_Console) {
            if (e.consoleType == ConsoleType::AtariLynx)   return m_atarilynx_logo;
            if (e.consoleType == ConsoleType::AtariJaguar)  return m_atarijaguar_logo;
            return m_atari2600_logo;
        }
    }
    else if (role == Qt::ToolTipRole) return e.path;
    else if (role == Qt::UserRole)    return e.path;

    return {};
}

QVariant GameListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
        case Column_Console:   return tr("Console");
        case Column_Cover:     return tr("Cover");
        case Column_Title:     return tr("Title");
        case Column_Publisher: return tr("Publisher");
        case Column_Region:    return tr("Region");
        case Column_FileName:  return tr("File Name");
        case Column_Size:      return tr("Size");
        case Column_Mapper:    return tr("Mapper");
        default:               return {};
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
        entry.coverPixmap = loadCover(entry.path);
    emit dataChanged(index(0, Column_Cover),
                     index(static_cast<int>(m_entries.size()) - 1, Column_Cover));
}

void GameListModel::setShowCoverTitles(bool enabled) { m_show_titles_for_covers = enabled; }

void GameListModel::setCoverScale(float scale)
{
    m_cover_scale = std::clamp(scale, 0.1f, 2.0f);
    emit coverScaleChanged();
}

int GameListModel::getCoverArtWidth()  const { return static_cast<int>(COVER_BASE_WIDTH  * m_cover_scale); }
int GameListModel::getCoverArtHeight() const { return static_cast<int>(COVER_BASE_HEIGHT * m_cover_scale); }

QString GameListModel::getEntryPath(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_entries.size())) return {};
    return m_entries[row].path;
}

void GameListModel::scanDirectories()
{
    QSettings settings;
    QStringList dirs = settings.value("GameList/Directories").toStringList();
    for (const QString& dir : dirs)
        scanDirectory(dir);
    std::sort(m_entries.begin(), m_entries.end(),
        [](const GameListEntry& a, const GameListEntry& b) {
            return a.title.toLower() < b.title.toLower();
        });
}

void GameListModel::scanDirectory(const QString& dir)
{
    static const QStringList exts = {
        "*.bin","*.a26","*.rom","*.lnx","*.lyx","*.j64","*.jag"
    };

    QDirIterator it(dir, exts, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();

        GameListEntry entry;
        entry.path      = fi.absoluteFilePath();
        entry.fileName  = fi.fileName();
        entry.fileSize  = fi.size();

        const QString suffix = fi.suffix().toLower();
        if (suffix == "lnx" || suffix == "lyx") {
            entry.consoleType = ConsoleType::AtariLynx;
            entry.mapperType  = QStringLiteral("Lynx");
        } else if (suffix == "j64" || suffix == "jag") {
            entry.consoleType = ConsoleType::AtariJaguar;
            entry.mapperType  = QStringLiteral("Jaguar");
        } else {
            entry.consoleType = ConsoleType::Atari2600;
            entry.mapperType  = guessMapper(fi.size());
        }

        entry.title     = cleanTitle(fi.completeBaseName());
        entry.region    = parseRegion(fi.completeBaseName());
        entry.publisher = parsePublisher(fi.completeBaseName(), entry.consoleType);
        entry.coverPixmap = loadCover(entry.path);

        m_entries.push_back(std::move(entry));
    }
}

QPixmap GameListModel::loadCover(const QString& romPath) const
{
    QFileInfo romInfo(romPath);
    QString baseName = romInfo.completeBaseName();

    QStringList coverDirs;
    QSettings settings;
    QString coverDir = settings.value("Covers/Directory").toString();
    if (!coverDir.isEmpty()) coverDirs << coverDir;

    QString appDir = QCoreApplication::applicationDirPath();
    coverDirs << appDir + "/covers";
    coverDirs << romInfo.absolutePath() + "/covers";

    QDir d(romInfo.absolutePath());
    for (int i = 0; i < 5; i++) {
        if (QDir(d.absolutePath() + "/.ataricovers").exists())
            coverDirs << d.absolutePath() + "/.ataricovers";
        if (QDir(d.absolutePath() + "/covers").exists())
            coverDirs << d.absolutePath() + "/covers";
        if (!d.cdUp()) break;
    }

    for (const QString& dir : coverDirs) {
        for (const QString& ext : {".png", ".jpg", ".jpeg", ".webp"}) {
            QString p = dir + "/" + baseName + ext;
            if (QFile::exists(p)) {
                QPixmap pix(p);
                if (!pix.isNull()) return pix;
            }
        }
    }
    return {};
}

QString GameListModel::guessMapper(qint64 size) const
{
    if (size <= 4096)  return QStringLiteral("None (2K/4K)");
    if (size <= 8192)  return QStringLiteral("F8 (8K)");
    if (size <= 16384) return QStringLiteral("F6 (16K)");
    if (size <= 32768) return QStringLiteral("F4 (32K)");
    return QStringLiteral("Unknown");
}
