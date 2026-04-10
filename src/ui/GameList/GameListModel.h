#pragma once

#include <QtCore/QAbstractTableModel>
#include <QtGui/QPixmap>
#include <array>
#include <vector>
#include <unordered_map>

enum class ConsoleType
{
    Atari2600,
    AtariLynx,
    Unknown
};

struct GameListEntry
{
    QString path;
    QString title;
    QString fileName;
    qint64 fileSize = 0;
    QString mapperType;
    QPixmap coverPixmap;
    ConsoleType consoleType = ConsoleType::Atari2600;
};

class GameListModel final : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column : int
    {
        Column_Console,
        Column_Cover,
        Column_Title,
        Column_FileName,
        Column_Size,
        Column_Mapper,
        Column_Count
    };

    explicit GameListModel(QObject* parent = nullptr);
    ~GameListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refresh();
    void refreshCovers();

    bool getShowCoverTitles() const { return m_show_titles_for_covers; }
    void setShowCoverTitles(bool enabled);

    float getCoverScale() const { return m_cover_scale; }
    void setCoverScale(float scale);
    int getCoverArtWidth() const;
    int getCoverArtHeight() const;

    QString getEntryPath(int row) const;

Q_SIGNALS:
    void coverScaleChanged();

private:
    void scanDirectories();
    void scanDirectory(const QString& dir);
    QPixmap loadCover(const QString& romPath) const;
    QString guessMapper(qint64 size) const;

    std::vector<GameListEntry> m_entries;
    float m_cover_scale = 1.0f;
    bool m_show_titles_for_covers = false;
    QPixmap m_placeholder_pixmap;
    QPixmap m_atari2600_logo;
    QPixmap m_atarilynx_logo;
};
