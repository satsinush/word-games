#ifdef WITH_GUI

#include "gui/MainWindow.hpp"
#include "gui/DungleonWidget.hpp"
#include "gui/HangmanWidget.hpp"
#include "gui/LetterBoxedWidget.hpp"
#include "gui/MastermindWidget.hpp"
#include "gui/SpellingBeeWidget.hpp"
#include "gui/WordleWidget.hpp"
#include "ui_MainWindow.h"
#include "utils/utils.hpp"

#include <QIcon>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSize>
#include <QSvgRenderer>

namespace {
QIcon loadGameIcon(const std::string &resourceFile, int size = 28,
                   bool pixelated = false) {
  const QString path =
      QString::fromStdString(Utils::getResourceFile(resourceFile));
  if (path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
    QPixmap pm(path);
    if (pm.isNull())
      return {};
    return QIcon(pm.scaled(size, size, Qt::KeepAspectRatio,
                           pixelated ? Qt::FastTransformation
                                     : Qt::SmoothTransformation));
  }

  QSvgRenderer renderer(path);
  if (!renderer.isValid())
    return {};
  QPixmap pm(size, size);
  pm.fill(Qt::transparent);
  QPainter painter(&pm);
  renderer.render(&painter);
  return QIcon(pm);
}

void applySidebarIcon(QPushButton *btn, const std::string &resourceFile,
                      bool pixelated = false) {
  if (!btn)
    return;
  btn->setIcon(loadGameIcon(resourceFile, 28, pixelated));
  btn->setIconSize(QSize(28, 28));
}
} // namespace

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
  sidebarButtonGroup->addButton(ui->btnDungleon, 4);
  sidebarButtonGroup->addButton(ui->btnHangman, 5);
  sidebarButtonGroup->setExclusive(true);

  applySidebarIcon(ui->btnWordle, "wordle-icon.svg");
  applySidebarIcon(ui->btnSpellingBee, "spelling-bee-icon.svg");
  applySidebarIcon(ui->btnLetterBoxed, "letter-boxed-icon.svg");
  applySidebarIcon(ui->btnMastermind, "mastermind-icon.svg");
  applySidebarIcon(ui->btnDungleon, "dungleon-icon.png", /*pixelated=*/true);
  applySidebarIcon(ui->btnHangman, "hangman-icon.svg");

  // Create and add game widgets to the stacked widget
  wordleWidget = new WordleWidget(this);
  spellingBeeWidget = new SpellingBeeWidget(this);
  letterBoxedWidget = new LetterBoxedWidget(this);
  mastermindWidget = new MastermindWidget(this);
  dungleonWidget = new DungleonWidget(this);
  hangmanWidget = new HangmanWidget(this);

  ui->stackedWidget->addWidget(wordleWidget);      // index 0
  ui->stackedWidget->addWidget(spellingBeeWidget); // index 1
  ui->stackedWidget->addWidget(letterBoxedWidget); // index 2
  ui->stackedWidget->addWidget(mastermindWidget);  // index 3
  ui->stackedWidget->addWidget(dungleonWidget);    // index 4
  ui->stackedWidget->addWidget(hangmanWidget);     // index 5

  // Set initial page to Wordle
  ui->stackedWidget->setCurrentIndex(0);
  ui->btnWordle->setChecked(true);

  // Connect signals and slots
  setupConnections();
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
  connect(ui->btnDungleon, &QPushButton::clicked, this,
          &MainWindow::onDungleonSelected);
  connect(ui->btnHangman, &QPushButton::clicked, this,
          &MainWindow::onHangmanSelected);

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

void MainWindow::onDungleonSelected() { switchToPage(4); }

void MainWindow::onHangmanSelected() { switchToPage(5); }

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
  case 4:
    dungleonWidget->newGame();
    break;
  case 5:
    hangmanWidget->newGame();
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
      "<li><b>Hangman:</b> Optimal letter guessing strategy</li>"
      "</ul>"
      "<p>Built with Qt 6 and C++17</p>");
}

#endif // WITH_GUI