#ifdef WITH_GUI

#include "gui/MainWindow.hpp"
#include "gui/LetterBoxedWidget.hpp"
#include "gui/MastermindWidget.hpp"
#include "gui/SpellingBeeWidget.hpp"
#include "gui/WordleWidget.hpp"
#include "ui_MainWindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {

  ui->setupUi(this);

  // Set window title
  setWindowTitle("Puzzle++");

  // Create button group for sidebar (exclusive selection)
  sidebarButtonGroup = new QButtonGroup(this);
  sidebarButtonGroup->addButton(ui->btnWordle, 0);
  sidebarButtonGroup->addButton(ui->btnSpellingBee, 1);
  sidebarButtonGroup->addButton(ui->btnLetterBoxed, 2);
  sidebarButtonGroup->addButton(ui->btnMastermind, 3);
  sidebarButtonGroup->setExclusive(true);

  // Create and add game widgets to the stacked widget
  wordleWidget = new WordleWidget(this);
  spellingBeeWidget = new SpellingBeeWidget(this);
  letterBoxedWidget = new LetterBoxedWidget(this);
  mastermindWidget = new MastermindWidget(this);

  ui->stackedWidget->addWidget(wordleWidget);      // index 0
  ui->stackedWidget->addWidget(spellingBeeWidget); // index 1
  ui->stackedWidget->addWidget(letterBoxedWidget); // index 2
  ui->stackedWidget->addWidget(mastermindWidget);  // index 3

  // Set initial page to Wordle
  ui->stackedWidget->setCurrentIndex(0);
  ui->btnWordle->setChecked(true);

  // Connect signals and slots
  setupConnections();

  // Initialize status bar
  // statusBar()->showMessage("Welcome to Word Games Suite!");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupConnections() {
  // Sidebar navigation connections
  connect(ui->btnWordle, &QPushButton::clicked, this,
          &MainWindow::onWordleSelected);
  connect(ui->btnSpellingBee, &QPushButton::clicked, this,
          &MainWindow::onSpellingBeeSelected);
  connect(ui->btnLetterBoxed, &QPushButton::clicked, this,
          &MainWindow::onLetterBoxedSelected);
  connect(ui->btnMastermind, &QPushButton::clicked, this,
          &MainWindow::onMastermindSelected);

  // Menu action connections
  connect(ui->actionNew_Game, &QAction::triggered, this,
          &MainWindow::onMenuNewGame);
  connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onMenuExit);
  connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onMenuAbout);
}

void MainWindow::switchToPage(int pageIndex) {
  ui->stackedWidget->setCurrentIndex(pageIndex);

  // Update button checked states
  if (auto *btn = sidebarButtonGroup->button(pageIndex)) {
    btn->setChecked(true);
  }
}

// Sidebar navigation slot implementations
void MainWindow::onWordleSelected() { switchToPage(0); }

void MainWindow::onSpellingBeeSelected() { switchToPage(1); }

void MainWindow::onLetterBoxedSelected() { switchToPage(2); }

void MainWindow::onMastermindSelected() { switchToPage(3); }

// Menu action slot implementations
void MainWindow::onMenuNewGame() {
  // Get the current widget and call its newGame() slot
  int currentIndex = ui->stackedWidget->currentIndex();

  switch (currentIndex) {
  case 0:
    wordleWidget->newGame();
    break;
  case 1:
    spellingBeeWidget->newGame();
    break;
  case 2:
    letterBoxedWidget->newGame();
    break;
  case 3:
    mastermindWidget->newGame();
    break;
  }
}

void MainWindow::onMenuExit() { close(); }

void MainWindow::onMenuAbout() {
  QMessageBox::about(
      this, "About Puzzle++",
      "<h3>Puzzle++</h3>"
      "<p>A collection of word-based puzzle games and solvers.</p>"
      "<ul>"
      "<li><b>Wordle:</b> Entropy-based solver to find optimal guesses</li>"
      "<li><b>Spelling Bee:</b> Find words using given letters</li>"
      "<li><b>Letter Boxed:</b> Create word chains to use all letters</li>"
      "<li><b>Mastermind:</b> Code-breaking pattern solver</li>"
      "</ul>"
      "<p>Built with Qt 6 and C++17</p>");
}

#endif // WITH_GUI