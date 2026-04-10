#pragma once

#include "ui_GameListWidget.h"

#include <QtWidgets/QListView>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QTableView>
#include <optional>

class GameListModel;
class GameListSortModel;

class GameListGridListView : public QListView
{
    Q_OBJECT

public:
    GameListGridListView(QWidget* parent = nullptr);

Q_SIGNALS:
    void zoomOut();
    void zoomIn();

protected:
    void wheelEvent(QWheelEvent* e);
};

class GameListWidget : public QWidget
{
    Q_OBJECT

public:
    GameListWidget(QWidget* parent = nullptr);
    ~GameListWidget();

    GameListModel* getModel() const { return m_model; }

    void initialize();

    void refresh(bool invalidate_cache);

    bool isShowingGameList() const;
    bool isShowingGameGrid() const;
    bool getShowGridCoverTitles() const;

    std::optional<QString> getSelectedEntry() const;

Q_SIGNALS:
    void refreshProgress(const QString& status, int current, int total);
    void refreshComplete();
    void selectionChanged();
    void entryActivated();
    void entryContextMenuRequested(const QPoint& point);
    void addGameDirectoryRequested();
    void layoutChange();

private Q_SLOTS:
    void onSelectionModelCurrentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onTableViewItemActivated(const QModelIndex& index);
    void onTableViewContextMenuRequested(const QPoint& point);
    void onListViewItemActivated(const QModelIndex& index);
    void onListViewContextMenuRequested(const QPoint& point);
    void onCoverScaleChanged();
    void onSearchTextChanged(const QString& text);

public Q_SLOTS:
    void showGameList();
    void showGameGrid();
    void setShowCoverTitles(bool enabled);
    void gridZoomIn();
    void gridZoomOut();
    void gridIntScale(int int_scale);
    void refreshGridCovers();

private:
    void listZoom(float delta);
    void updateToolbar();

    Ui::GameListWidget m_ui;

    GameListModel* m_model = nullptr;
    GameListSortModel* m_sort_model = nullptr;
    QTableView* m_table_view = nullptr;
    GameListGridListView* m_list_view = nullptr;
};
