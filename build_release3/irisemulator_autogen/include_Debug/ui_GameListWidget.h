/********************************************************************************
** Form generated from reading UI file 'GameListWidget.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GAMELISTWIDGET_H
#define UI_GAMELISTWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GameListWidget
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    QHBoxLayout *horizontalLayout;
    QToolButton *viewGameList;
    QToolButton *viewGameGrid;
    QToolButton *viewGridTitles;
    QSlider *gridScale;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_2;
    QLineEdit *searchText;
    QStackedWidget *stack;

    void setupUi(QWidget *GameListWidget)
    {
        if (GameListWidget->objectName().isEmpty())
            GameListWidget->setObjectName("GameListWidget");
        GameListWidget->resize(758, 619);
        verticalLayout = new QVBoxLayout(GameListWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName("verticalLayout");
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        horizontalLayout_3->setContentsMargins(3, 3, 3, 3);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(3);
        horizontalLayout->setObjectName("horizontalLayout");
        viewGameList = new QToolButton(GameListWidget);
        viewGameList->setObjectName("viewGameList");
        viewGameList->setMinimumSize(QSize(32, 0));
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("list-check")));
        viewGameList->setIcon(icon);
        viewGameList->setCheckable(true);
        viewGameList->setAutoRaise(true);

        horizontalLayout->addWidget(viewGameList);

        viewGameGrid = new QToolButton(GameListWidget);
        viewGameGrid->setObjectName("viewGameGrid");
        viewGameGrid->setMinimumSize(QSize(32, 0));
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("function-line")));
        viewGameGrid->setIcon(icon1);
        viewGameGrid->setCheckable(true);
        viewGameGrid->setAutoRaise(true);

        horizontalLayout->addWidget(viewGameGrid);

        viewGridTitles = new QToolButton(GameListWidget);
        viewGridTitles->setObjectName("viewGridTitles");
        viewGridTitles->setMinimumSize(QSize(32, 0));
        QIcon icon2(QIcon::fromTheme(QString::fromUtf8("price-tag-3-line")));
        viewGridTitles->setIcon(icon2);
        viewGridTitles->setCheckable(true);
        viewGridTitles->setAutoRaise(true);

        horizontalLayout->addWidget(viewGridTitles);

        gridScale = new QSlider(GameListWidget);
        gridScale->setObjectName("gridScale");
        gridScale->setMinimumSize(QSize(125, 0));
        gridScale->setMaximumSize(QSize(125, 16777215));
        gridScale->setMinimum(10);
        gridScale->setMaximum(200);
        gridScale->setOrientation(Qt::Orientation::Horizontal);

        horizontalLayout->addWidget(gridScale);


        horizontalLayout_3->addLayout(horizontalLayout);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        searchText = new QLineEdit(GameListWidget);
        searchText->setObjectName("searchText");
        searchText->setMinimumSize(QSize(150, 0));
        searchText->setClearButtonEnabled(true);

        horizontalLayout_2->addWidget(searchText);


        horizontalLayout_3->addLayout(horizontalLayout_2);


        verticalLayout->addLayout(horizontalLayout_3);

        stack = new QStackedWidget(GameListWidget);
        stack->setObjectName("stack");

        verticalLayout->addWidget(stack);


        retranslateUi(GameListWidget);

        QMetaObject::connectSlotsByName(GameListWidget);
    } // setupUi

    void retranslateUi(QWidget *GameListWidget)
    {
#if QT_CONFIG(tooltip)
        viewGameList->setToolTip(QCoreApplication::translate("GameListWidget", "Game List", nullptr));
#endif // QT_CONFIG(tooltip)
        viewGameList->setText(QCoreApplication::translate("GameListWidget", "Game List", nullptr));
#if QT_CONFIG(tooltip)
        viewGameGrid->setToolTip(QCoreApplication::translate("GameListWidget", "Game Grid", nullptr));
#endif // QT_CONFIG(tooltip)
        viewGameGrid->setText(QCoreApplication::translate("GameListWidget", "Game Grid", nullptr));
#if QT_CONFIG(tooltip)
        viewGridTitles->setToolTip(QCoreApplication::translate("GameListWidget", "Show Titles", nullptr));
#endif // QT_CONFIG(tooltip)
        viewGridTitles->setText(QCoreApplication::translate("GameListWidget", "Show Titles", nullptr));
        searchText->setPlaceholderText(QCoreApplication::translate("GameListWidget", "Search...", nullptr));
        (void)GameListWidget;
    } // retranslateUi

};

namespace Ui {
    class GameListWidget: public Ui_GameListWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GAMELISTWIDGET_H
