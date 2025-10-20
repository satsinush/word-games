#ifdef WITH_GUI

#include "gui/SpellingBeeWidget.hpp"
#include "ui_SpellingBeeWidget.h"

#include <QMessageBox>
#include <algorithm>
#include <random>
#include <set>

SpellingBeeWidget::SpellingBeeWidget(const std::vector<Utils::Word> &words,
                                     QWidget *parent)
    : QWidget(parent), ui(new Ui::SpellingBeeWidget), wordVec(words) {
  ui->setupUi(this);

  // Initialize config
  config.allLetters.fill('\0');
  config.validLettersMap.fill(false);

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onNewGame);
  connect(ui->shuffleBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onShuffle);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &SpellingBeeWidget::onSubmit);

  // Set initial state
  ui->lettersLabel->setText("Enter 7 letters above");
  ui->outputArea->append("Enter 7 unique letters (e.g., 'abcdefg')");
  ui->outputArea->append(
      "The first letter will be the required center letter.");
}

SpellingBeeWidget::~SpellingBeeWidget() { delete ui; }

void SpellingBeeWidget::newGame() { onNewGame(); }

void SpellingBeeWidget::onSubmit() {
  QString lettersInput = ui->inputField->text().trimmed().toLower();
  if (lettersInput.isEmpty()) {
    QMessageBox::information(this, "Input Required", "Please enter 7 letters!");
    return;
  }

  // Remove spaces and validate
  std::string letters = lettersInput.toStdString();
  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());

  if (letters.size() != 7) {
    QMessageBox::warning(this, "Invalid Input",
                         "Must provide exactly 7 letters!");
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
  for (size_t i = 0; i < 7; ++i) {
    config.allLetters[i] = letters[i];
  }

  config.validLettersMap.fill(false);
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  // Display letters
  QString lettersDisplay;
  for (size_t i = 0; i < 7; ++i) {
    lettersDisplay += QString("%1 ").arg(QChar(config.allLetters[i]).toUpper());
  }
  ui->lettersLabel->setText(lettersDisplay.trimmed());

  // Solve
  solutions = SpellingBee::runSpellingBeeSolver(wordVec, config);

  // Display results
  ui->outputArea->clear();
  ui->outputArea->append(QString("Center letter: %1 (must be used)")
                             .arg(QChar(config.allLetters[0]).toUpper()));
  ui->outputArea->append(
      QString("\nFound %1 valid words:\n").arg(solutions.size()));

  // Show top 100 solutions
  int limit = std::min(100, static_cast<int>(solutions.size()));
  int lastUniqueLetters = 0;

  for (int i = limit - 1; i >= 0; --i) {
    const auto &word = solutions[i];
    if (word.uniqueLetters != lastUniqueLetters) {
      ui->outputArea->append(
          QString("\n--- %1 unique letters ---").arg(word.uniqueLetters));
      lastUniqueLetters = word.uniqueLetters;
    }
    ui->outputArea->append(QString::fromStdString(word.wordString));
  }

  ui->inputField->clear();
}

void SpellingBeeWidget::onNewGame() {
  ui->outputArea->clear();
  ui->inputField->clear();
  ui->lettersLabel->setText("Enter 7 letters above");
  ui->scoreLabel->setText("");
  solutions.clear();
}

void SpellingBeeWidget::onShuffle() {
  if (config.allLetters[0] == '\0') {
    QMessageBox::information(this, "No Puzzle",
                             "Enter letters first to create a puzzle!");
    return;
  }

  // Shuffle the display order of letters (keeping first letter fixed as center)
  std::array<char, 7> shuffled = config.allLetters;
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(shuffled.begin() + 1, shuffled.end(), g);

  QString lettersDisplay;
  for (size_t i = 0; i < 7; ++i) {
    lettersDisplay += QString("%1 ").arg(QChar(shuffled[i]).toUpper());
  }
  ui->lettersLabel->setText(lettersDisplay.trimmed());
}

#endif // WITH_GUI
