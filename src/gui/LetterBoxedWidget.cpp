#ifdef WITH_GUI

#include "gui/LetterBoxedWidget.hpp"
#include "ui_LetterBoxedWidget.h"

#include <QMessageBox>
#include <algorithm>
#include <random>
#include <set>

LetterBoxedWidget::LetterBoxedWidget(const std::vector<Utils::Word> &words,
                                     QWidget *parent)
    : QWidget(parent), ui(new Ui::LetterBoxedWidget), wordVec(words) {
  ui->setupUi(this);

  // Initialize config
  config.allLetters.fill('*');

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onSolve);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &LetterBoxedWidget::onSubmit);

  // Set initial state
  ui->boxLabel->setText("Enter 12 letters above");
  ui->outputArea->append("Enter 12 unique letters (e.g., 'abcdefghijkl')");
  ui->outputArea->append("Letters will be arranged as:");
  ui->outputArea->append("  Top: 1-3, Right: 4-6, Bottom: 7-9, Left: 10-12");
}

LetterBoxedWidget::~LetterBoxedWidget() { delete ui; }

void LetterBoxedWidget::newGame() { onNewGame(); }

void LetterBoxedWidget::onSubmit() {
  QString lettersInput = ui->inputField->text().trimmed().toLower();
  if (lettersInput.isEmpty()) {
    QMessageBox::information(this, "Input Required",
                             "Please enter 12 letters!");
    return;
  }

  // Remove spaces and validate
  std::string letters = lettersInput.toStdString();
  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());

  if (letters.size() != 12) {
    QMessageBox::warning(this, "Invalid Input",
                         "Must provide exactly 12 letters!");
    return;
  }

  // Check for duplicates
  std::set<char> seen;
  for (char c : letters) {
    if (!isalpha(static_cast<unsigned char>(c))) {
      QMessageBox::warning(this, "Invalid Input",
                           "All characters must be letters!");
      return;
    }
    if (seen.count(c)) {
      QMessageBox::warning(this, "Invalid Input",
                           "Duplicate letters not allowed!");
      return;
    }
    seen.insert(c);
  }

  // Set up config
  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = letters[i];
  }

  // Set up letter mappings
  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  config.uniquePuzzleLetters.reset();
  for (int i = 0; i < 12; ++i)
    config.uniquePuzzleLetters.set(i);

  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  // Display the box
  auto up = [](char c) { return QChar(c).toUpper(); };
  QString boxDisplay =
      QString("Top: %1 %2 %3\nLeft: %4 %5 %6  |  Right: %7 %8 %9\nBottom: %10 "
              "%11 %12")
          .arg(up(config.allLetters[0]))
          .arg(up(config.allLetters[1]))
          .arg(up(config.allLetters[2]))
          .arg(up(config.allLetters[11]))
          .arg(up(config.allLetters[10]))
          .arg(up(config.allLetters[9]))
          .arg(up(config.allLetters[3]))
          .arg(up(config.allLetters[4]))
          .arg(up(config.allLetters[5]))
          .arg(up(config.allLetters[8]))
          .arg(up(config.allLetters[7]))
          .arg(up(config.allLetters[6]));

  ui->boxLabel->setText(boxDisplay);

  ui->inputField->clear();
  ui->outputArea->append(
      "Letters configured! Click 'Solve' to find solutions.");
}

void LetterBoxedWidget::onNewGame() {
  ui->outputArea->clear();
  ui->inputField->clear();
  ui->boxLabel->setText("Enter 12 letters above");
  config.allLetters.fill('*');
  solutions.clear();
}

void LetterBoxedWidget::onSolve() {
  if (config.allLetters[0] == '*') {
    QMessageBox::information(this, "No Puzzle",
                             "Enter 12 letters first to create a puzzle!");
    return;
  }

  ui->outputArea->append("\n--- Finding solutions ---");

  // Configure for fast solving (matching CLI preset 2)
  config.maxDepth = 2;
  config.minWordLength = 4;
  config.minUniqueLetters = 3;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = true;

  solutions = LetterBoxed::runLetterBoxedSolver(config, wordVec);

  ui->outputArea->append(
      QString("\nFound %1 solutions!").arg(solutions.size()));

  // Show top 100 solutions
  int limit = std::min(100, static_cast<int>(solutions.size()));
  if (limit > 0) {
    ui->outputArea->append("\n=== Solutions ===");
    for (int i = 0; i < limit; ++i) {
      ui->outputArea->append(QString("%1. %2 (%3 words)")
                                 .arg(i + 1)
                                 .arg(QString::fromStdString(solutions[i].text))
                                 .arg(solutions[i].wordCount));
    }
  }
}

#endif // WITH_GUI
