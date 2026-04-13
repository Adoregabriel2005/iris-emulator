#include "GameListWidget.h"
#include "GameListModel.h"

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>

// --- GameListGridListView ---

GameListGridListView::GameListGridListView(QWidget* parent)
    : QListView(parent)
{
}

void GameListGridListView::wheelEvent(QWheelEvent* e)
{
    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->angleDelta().y() > 0)
            emit zoomIn();
        else
            emit zoomOut();
        e->accept();
        return;
    }
    QListView::wheelEvent(e);
}

// --- GameListSortModel ---

class GameListSortModel : public QSortFilterProxyModel
{
public:
    explicit GameListSortModel(QObject* parent = nullptr)
        : QSortFilterProxyModel(parent) {}

    void setFilterText(const QString& text)
    {
        m_filter_text = text.toLower();
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
    {
        if (m_filter_text.isEmpty())
            return true;

        const QModelIndex idx = sourceModel()->index(source_row, GameListModel::Column_Title, source_parent);
        const QString title = sourceModel()->data(idx, Qt::DisplayRole).toString().toLower();
        return title.contains(m_filter_text);
    }

private:
    QString m_filter_text;
};

// --- GameListWidget ---

GameListWidget::GameListWidget(QWidget* parent)
    : QWidget(parent)
{
}

GameListWidget::~GameListWidget()
{
}

void GameListWidget::initialize()
{
    m_ui.setupUi(this);

    m_model = new GameListModel(this);
    m_sort_model = new GameListSortModel(this);
    m_sort_model->setSourceModel(m_model);
    m_sort_model->setSortCaseSensitivity(Qt::CaseInsensitive);

    // Table view (game list)
    m_table_view = new QTableView(this);
    m_table_view->setModel(m_sort_model);
    m_table_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table_view->setAlternatingRowColors(true);
    m_table_view->setSortingEnabled(true);
    m_table_view->setShowGrid(false);
    m_table_view->verticalHeader()->hide();
    m_table_view->horizontalHeader()->setStretchLastSection(true);
    m_table_view->horizontalHeader()->setHighlightSections(false);
    m_table_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui.stack->addWidget(m_table_view);

    // Grid view (covers)
    m_list_view = new GameListGridListView(this);
    m_list_view->setModel(m_sort_model);
    m_list_view->setViewMode(QListView::IconMode);
    m_list_view->setResizeMode(QListView::Adjust);
    m_list_view->setUniformItemSizes(true);
    m_list_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_list_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_ui.stack->addWidget(m_list_view);

    // Default column widths for table
    m_table_view->setColumnWidth(GameListModel::Column_Console,   30);
    m_table_view->setColumnWidth(GameListModel::Column_Cover,      60);
    m_table_view->setColumnWidth(GameListModel::Column_Title,     300);
    m_table_view->setColumnWidth(GameListModel::Column_Publisher, 160);
    m_table_view->setColumnWidth(GameListModel::Column_Region,     90);
    m_table_view->setColumnWidth(GameListModel::Column_Size,       80);
    m_table_view->setColumnWidth(GameListModel::Column_Mapper,     80);
    m_table_view->sortByColumn(GameListModel::Column_Title, Qt::AscendingOrder);

    // Hide technical columns by default
    m_table_view->setColumnHidden(GameListModel::Column_FileName, true);
    m_table_view->setColumnHidden(GameListModel::Column_Size,     true);
    m_table_view->setColumnHidden(GameListModel::Column_Mapper,   true);

    // Allow toggling columns via header right-click
    m_table_view->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_table_view->horizontalHeader(), &QHeaderView::customContextMenuRequested,
        this, [this](const QPoint& pos) {
            QMenu menu;
            for (int col = 0; col < GameListModel::Column_Count; col++) {
                QString name = m_model->headerData(col, Qt::Horizontal, Qt::DisplayRole).toString();
                QAction* action = menu.addAction(name);
                action->setCheckable(true);
                action->setChecked(!m_table_view->isColumnHidden(col));
                connect(action, &QAction::toggled, [this, col](bool visible) {
                    m_table_view->setColumnHidden(col, !visible);
                });
            }
            menu.exec(m_table_view->horizontalHeader()->mapToGlobal(pos));
        });

    // Connect toolbar buttons
    connect(m_ui.viewGameList, &QToolButton::clicked, this, &GameListWidget::showGameList);
    connect(m_ui.viewGameGrid, &QToolButton::clicked, this, &GameListWidget::showGameGrid);
    connect(m_ui.viewGridTitles, &QToolButton::toggled, this, &GameListWidget::setShowCoverTitles);
    connect(m_ui.gridScale, &QSlider::valueChanged, this, [this](int value) {
        m_model->setCoverScale(static_cast<float>(value) / 100.0f);
    });
    connect(m_ui.searchText, &QLineEdit::textChanged, this, &GameListWidget::onSearchTextChanged);

    // Table view signals
    connect(m_table_view, &QTableView::activated, this, &GameListWidget::onTableViewItemActivated);
    connect(m_table_view, &QTableView::customContextMenuRequested, this, &GameListWidget::onTableViewContextMenuRequested);
    connect(m_table_view->selectionModel(), &QItemSelectionModel::currentChanged,
        this, &GameListWidget::onSelectionModelCurrentChanged);

    // Grid view signals
    connect(m_list_view, &QListView::activated, this, &GameListWidget::onListViewItemActivated);
    connect(m_list_view, &QListView::customContextMenuRequested, this, &GameListWidget::onListViewContextMenuRequested);
    connect(m_list_view, &GameListGridListView::zoomIn, this, &GameListWidget::gridZoomIn);
    connect(m_list_view, &GameListGridListView::zoomOut, this, &GameListWidget::gridZoomOut);

    // Initial settings
    QSettings settings;
    const bool show_grid = settings.value("UI/GameListGridView", false).toBool();
    const float cover_scale = settings.value("UI/CoverScale", 1.0f).toFloat();
    m_ui.gridScale->setValue(static_cast<int>(cover_scale * 100.0f));
    m_model->setCoverScale(cover_scale);

    if (show_grid)
        showGameGrid();
    else
        showGameList();

    // Initial scan
    refresh(false);
}

void GameListWidget::refresh(bool invalidate_cache)
{
    Q_UNUSED(invalidate_cache);
    m_model->refresh();
}

bool GameListWidget::isShowingGameList() const
{
    return m_ui.stack->currentWidget() == m_table_view;
}

bool GameListWidget::isShowingGameGrid() const
{
    return m_ui.stack->currentWidget() == m_list_view;
}

bool GameListWidget::getShowGridCoverTitles() const
{
    return m_model ? m_model->getShowCoverTitles() : false;
}

std::optional<QString> GameListWidget::getSelectedEntry() const
{
    QModelIndex index;
    if (isShowingGameList())
        index = m_table_view->currentIndex();
    else
        index = m_list_view->currentIndex();

    if (!index.isValid())
        return std::nullopt;

    const QModelIndex source_index = m_sort_model->mapToSource(index);
    return m_model->getEntryPath(source_index.row());
}

void GameListWidget::showGameList()
{
    m_ui.stack->setCurrentWidget(m_table_view);
    m_ui.viewGameList->setChecked(true);
    m_ui.viewGameGrid->setChecked(false);
    updateToolbar();
    emit layoutChange();
}

void GameListWidget::showGameGrid()
{
    m_ui.stack->setCurrentWidget(m_list_view);
    m_ui.viewGameList->setChecked(false);
    m_ui.viewGameGrid->setChecked(true);
    updateToolbar();
    emit layoutChange();
}

void GameListWidget::setShowCoverTitles(bool enabled)
{
    if (m_model)
        m_model->setShowCoverTitles(enabled);
    m_ui.viewGridTitles->setChecked(enabled);
    emit layoutChange();
}

void GameListWidget::gridZoomIn()
{
    listZoom(0.1f);
}

void GameListWidget::gridZoomOut()
{
    listZoom(-0.1f);
}

void GameListWidget::gridIntScale(int int_scale)
{
    m_model->setCoverScale(static_cast<float>(int_scale) / 100.0f);
    m_ui.gridScale->setValue(int_scale);
}

void GameListWidget::refreshGridCovers()
{
    if (m_model)
        m_model->refreshCovers();
}

void GameListWidget::listZoom(float delta)
{
    if (!m_model)
        return;
    const float new_scale = std::clamp(m_model->getCoverScale() + delta, 0.1f, 2.0f);
    m_model->setCoverScale(new_scale);
    m_ui.gridScale->setValue(static_cast<int>(new_scale * 100.0f));
    QSettings settings;
    settings.setValue("UI/CoverScale", new_scale);
}

void GameListWidget::updateToolbar()
{
    const bool is_grid = isShowingGameGrid();
    m_ui.viewGridTitles->setEnabled(is_grid);
    m_ui.gridScale->setEnabled(is_grid);
}

// --- Slots ---

void GameListWidget::onSelectionModelCurrentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);
    Q_UNUSED(current);
    emit selectionChanged();
}

void GameListWidget::onTableViewItemActivated(const QModelIndex& index)
{
    Q_UNUSED(index);
    emit entryActivated();
}

void GameListWidget::onTableViewContextMenuRequested(const QPoint& point)
{
    emit entryContextMenuRequested(m_table_view->mapToGlobal(point));
}

void GameListWidget::onListViewItemActivated(const QModelIndex& index)
{
    Q_UNUSED(index);
    emit entryActivated();
}

void GameListWidget::onListViewContextMenuRequested(const QPoint& point)
{
    emit entryContextMenuRequested(m_list_view->mapToGlobal(point));
}

void GameListWidget::onCoverScaleChanged()
{
    // handled by slider
}

void GameListWidget::onSearchTextChanged(const QString& text)
{
    if (m_sort_model)
        m_sort_model->setFilterText(text);
}
