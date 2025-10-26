#ifdef WITH_GUI

#pragma once

#include "utils/wordUtils.hpp"
#include <QButtonGroup>
#include <QMainWindow>
#include <vector>

// Forward declarations
namespace Ui {
class MainWindow;
}

class WordleWidget;
class SpellingBeeWidget;
class LetterBoxedWidget;
class MastermindWidget;
class DungleonWidget;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  // Sidebar navigation slots
  void onWordleSelected();
  void onSpellingBeeSelected();
  void onLetterBoxedSelected();
  void onMastermindSelected();
  void onDungleonSelected();

  // Menu actions
  void onMenuNewGame();
  void onMenuExit();
  void onMenuAbout();

private:
  // A pointer to the class that holds our UI elements
  Ui::MainWindow *ui;

  // Button group for exclusive selection in sidebar
  QButtonGroup *sidebarButtonGroup;

  // Game widgets
  WordleWidget *wordleWidget;
  SpellingBeeWidget *spellingBeeWidget;
  LetterBoxedWidget *letterBoxedWidget;
  MastermindWidget *mastermindWidget;
  DungleonWidget *dungleonWidget;

  // Helper methods
  void setupConnections();
  void switchToPage(int pageIndex);
};

#endif // WITH_GUI