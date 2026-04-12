#pragma once
#include <QDialog>

class SDLInput;
class QStackedWidget;
class QLabel;
class QPushButton;
class QListWidget;

class WelcomeDialog : public QDialog
{
    Q_OBJECT
public:
    // firstRun=true  → full setup wizard (PCSX2 style, multi-page)
    // firstRun=false → returning user greeting (single page, quick dismiss)
    explicit WelcomeDialog(SDLInput* input, bool firstRun, QWidget* parent = nullptr);

    static bool isFirstRun();
    static bool shouldShow();   // false if user ticked "don't show again"
    static void markConfigured();

private:
    void buildSetupWizard(SDLInput* input);
    void buildReturningGreeting();
    void goToPage(int index);
    void updateNav();

    QStackedWidget* m_pages   = nullptr;
    QListWidget*    m_sidebar = nullptr;
    QPushButton*    m_back    = nullptr;
    QPushButton*    m_next    = nullptr;
    QPushButton*    m_finish  = nullptr;
    int             m_currentPage = 0;
    bool            m_firstRun    = true;

    // Captured widgets needed at finish time
    class QLineEdit* m_romEdit  = nullptr;
    class QLineEdit* m_lynxEdit = nullptr;
    class QCheckBox* m_dontShowCheck = nullptr;
};
