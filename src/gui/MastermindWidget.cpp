#ifdef WITH_GUI

#include "gui/MastermindWidget.hpp"
#include "ui_MastermindWidget.h"

#include <QMessageBox>
#include <sstream>

MastermindWidget::MastermindWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::MastermindWidget) {
  ui->setupUi(this);

  // Initialize default config
  config.numPegs = 4;
  config.numColors = 6;
  config.allowDuplicates = true;
  config.maxDepth = 1;

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &MastermindWidget::onNewGame);
  connect(ui->hintBtn, &QPushButton::clicked, this, &MastermindWidget::onHint);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &MastermindWidget::onSubmit);

  newGame();
}

MastermindWidget::~MastermindWidget() { delete ui; }

void MastermindWidget::newGame() { onNewGame(); }

void MastermindWidget::onSubmit() {
  QString input = ui->inputField->text().trimmed();
  if (input.isEmpty()) {
    QMessageBox::information(this, "Input Required",
                             "Please enter guess and feedback!");
    return;
  }

  try {
    // Parse input format: "1 2 3 4|2 1"
    std::string inputStr = input.toStdString();
    size_t pipePos = inputStr.find('|');
    if (pipePos == std::string::npos) {
      throw std::invalid_argument("Missing '|' separator. Use: 1 2 3 4|2 1");
    }

    std::string patternStr = inputStr.substr(0, pipePos);
    std::string feedbackStr = inputStr.substr(pipePos + 1);

    // Parse pattern
    std::istringstream patternIss(patternStr);
    Mastermind::Pattern pattern;
    pattern.numPegs = 0;
    std::string token;
    while (patternIss >> token && pattern.numPegs < Mastermind::MAX_PEGS) {
      int color = std::stoi(token);
      if (color < 0 || color > static_cast<int>(config.numColors)) {
        throw std::invalid_argument("Color out of range");
      }
      pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
      pattern.numPegs++;
    }

    if (pattern.numPegs != config.numPegs) {
      throw std::invalid_argument("Wrong number of pegs");
    }

    // Parse feedback
    std::istringstream feedbackIss(feedbackStr);
    int correctPos, correctCol;
    if (!(feedbackIss >> correctPos >> correctCol)) {
      throw std::invalid_argument("Invalid feedback format");
    }

    Mastermind::Feedback feedback;
    feedback.guess = pattern;
    feedback.correctPosition = static_cast<uint8_t>(correctPos);
    feedback.correctColor = static_cast<uint8_t>(correctCol);

    feedbackHistory.push_back(feedback);

    ui->outputArea->append(QString("Added: %1 with feedback %2 %3")
                               .arg(QString::fromStdString(pattern.toString()))
                               .arg(correctPos)
                               .arg(correctCol));

    ui->attemptsLabel->setText(
        QString("Attempts: %1").arg(feedbackHistory.size()));
    ui->inputField->clear();

    // Check if solved
    if (correctPos == static_cast<int>(config.numPegs)) {
      ui->outputArea->append("\n🎉 Congratulations! You solved it!");
    }
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Invalid Input",
                         QString("Error: %1\n\nFormat: 1 2 3 4|2 "
                                 "1\n(pattern|correct_pos correct_color)")
                             .arg(e.what()));
  }
}

void MastermindWidget::onNewGame() { initGame(); }

void MastermindWidget::onHint() { solveMastermind(); }

void MastermindWidget::initGame() {
  feedbackHistory.clear();
  ui->outputArea->clear();
  ui->inputField->clear();
  ui->attemptsLabel->setText("Attempts: 0");

  // Generate all possible patterns
  allPatterns = Mastermind::generateAllPatterns(config);

  ui->outputArea->append("New Mastermind game started!");
  ui->outputArea->append(QString("\nSettings: %1 pegs, %2 colors")
                             .arg(config.numPegs)
                             .arg(config.numColors));
  ui->outputArea->append(
      QString("Total possible patterns: %1\n").arg(allPatterns.size()));
  ui->outputArea->append("Enter your guesses and feedback:");
  ui->outputArea->append(
      "Format: 1 2 3 4|2 1 (pattern|correct_pos correct_color)\n");
}

void MastermindWidget::solveMastermind() {
  ui->outputArea->append("\n--- Calculating best guesses ---");

  try {
    Mastermind::Result result =
        Mastermind::runMastermindSolver(allPatterns, feedbackHistory, config);

    ui->outputArea->append(QString("\nPossible patterns remaining: %1\n")
                               .arg(result.totalPossiblePatterns));

    if (!result.sortedGuesses.empty()) {
      ui->outputArea->append("=== Top 10 Suggestions ===");

      int limit = std::min(10, static_cast<int>(result.sortedGuesses.size()));
      for (int i = 0; i < limit; ++i) {
        const auto &guess = result.sortedGuesses[i];
        ui->outputArea->append(
            QString("%1. %2 (ENT: %3)")
                .arg(i + 1)
                .arg(QString::fromStdString(guess.pattern.toString()))
                .arg(guess.ent, 0, 'f', 2));
      }
    }
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Solver Error",
                          QString("Error running solver: %1").arg(e.what()));
  }
}

#endif // WITH_GUI
