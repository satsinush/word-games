#ifdef WITH_GUI

#include "gui/WordleWidget.hpp"
#include "ui_WordleWidget.h"

#include <QHBoxLayout>
#include <QMessageBox>
#include <QMouseEvent>
#include <algorithm>

// ============ LetterBox Implementation ============

LetterBox::LetterBox(QWidget *parent)
    : QLabel(parent), currentLetter('\0'), currentColor(0) {
  setFixedSize(60, 60);
  setAlignment(Qt::AlignCenter);
  QFont font;
  font.setPointSize(24);
  font.setBold(true);
  setFont(font);
  setCursor(Qt::PointingHandCursor);
  updateStyle();
}

void LetterBox::setLetter(char letter) {
  currentLetter = std::toupper(letter);
  setText(QString(currentLetter));
  // Default to grey when a letter is set
  if (currentLetter != '\0' && currentLetter != ' ' && currentColor < 0) {
    setColor(0);
  }
}

void LetterBox::setColor(int color) {
  currentColor = color % 3; // Cycle through 0, 1, 2
  updateStyle();
}

void LetterBox::clear() {
  currentLetter = '\0';
  currentColor = 0;
  setText("");
  updateStyle();
}

void LetterBox::updateStyle() {
  QString bgColor, textColor, border;

  switch (currentColor) {
  case 0: // Grey
    bgColor = "#787c7e";
    textColor = "white";
    border = "#787c7e";
    break;
  case 1: // Yellow
    bgColor = "#c9b458";
    textColor = "white";
    border = "#c9b458";
    break;
  case 2: // Green
    bgColor = "#6aaa64";
    textColor = "white";
    border = "#6aaa64";
    break;
  }

  // If no letter, show empty box with grey background
  if (currentLetter == '\0' || currentLetter == ' ') {
    bgColor = "white";
    textColor = "black";
    border = "#d3d6da";
  }

  setStyleSheet(QString("QLabel { "
                        "background-color: %1; "
                        "color: %2; "
                        "border: 2px solid %3; "
                        "border-radius: 4px; "
                        "}")
                    .arg(bgColor)
                    .arg(textColor)
                    .arg(border));
}

void LetterBox::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && currentLetter != '\0') {
    emit clicked();
  }
  QLabel::mousePressEvent(event);
}

// ============ GuessRow Implementation ============

GuessRow::GuessRow(const Wordle::Feedback &feedback, QWidget *parent)
    : QWidget(parent), isEditable(false) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(5);
  layout->setContentsMargins(0, 5, 0, 5);

  // Create 5 letter boxes
  for (int i = 0; i < 5; ++i) {
    boxes[i] = new LetterBox(this);
    boxes[i]->setLetter(feedback.word[i]);
    boxes[i]->setColor(feedback.getColor(i));
    connect(boxes[i], &LetterBox::clicked, this, &GuessRow::onLetterBoxClicked);
    layout->addWidget(boxes[i]);
  }

  // Add spacing
  layout->addSpacing(10);

  // Delete button
  deleteBtn = new QPushButton("✕", this);
  deleteBtn->setFixedSize(30, 30);
  deleteBtn->setToolTip("Delete this guess");
  deleteBtn->setStyleSheet("QPushButton { background-color: #dc3545; color: "
                           "white; border: none; border-radius: 4px; }");
  connect(deleteBtn, &QPushButton::clicked, this, &GuessRow::onDeleteClicked);
  layout->addWidget(deleteBtn);

  layout->addStretch();
}

Wordle::Feedback GuessRow::getFeedback() const {
  Wordle::Feedback fb;
  fb.word = "";
  for (int i = 0; i < 5; ++i) {
    fb.word += std::tolower(boxes[i]->getLetter());
    int color = boxes[i]->getColor();
    switch (color) {
    case 0:
      fb.setGrey(i);
      break;
    case 1:
      fb.setYellow(i);
      break;
    case 2:
      fb.setGreen(i);
      break;
    }
  }
  return fb;
}

void GuessRow::setEditable(bool editable) {
  isEditable = editable;
  for (int i = 0; i < 5; ++i) {
    boxes[i]->setCursor(editable ? Qt::PointingHandCursor : Qt::ArrowCursor);
  }
}

void GuessRow::onLetterBoxClicked() {
  if (isEditable) {
    LetterBox *box = qobject_cast<LetterBox *>(sender());
    if (box && box->getLetter() != '\0') {
      box->setColor(box->getColor() + 1);
      emit editRequested();
    }
  }
}

void GuessRow::onDeleteClicked() { emit deleteRequested(); }

// ============ WordleWidget Implementation ============

WordleWidget::WordleWidget(const std::vector<Utils::Word> &words,
                           QWidget *parent)
    : QWidget(parent), ui(new Ui::WordleWidget), wordVec(words),
      currentRowWidget(nullptr) {
  ui->setupUi(this);

  // Initialize config
  config.maxDepth = 1;
  config.excludeUncommonWords = true;

  // Create guess list container
  guessListWidget = new QWidget(this);
  guessListLayout = new QVBoxLayout(guessListWidget);
  guessListLayout->setSpacing(2);
  guessListLayout->setContentsMargins(0, 0, 0, 0);
  guessListLayout->addStretch();

  // Add guess list to main layout
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
  if (mainLayout) {
    // Insert after title, before output area
    mainLayout->insertWidget(1, guessListWidget);
  }

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this, &WordleWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &WordleWidget::onNewGame);
  connect(ui->hintBtn, &QPushButton::clicked, this, &WordleWidget::onHint);
  connect(ui->inputField, &QLineEdit::textChanged, this,
          &WordleWidget::onInputChanged);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &WordleWidget::onSubmit);

  // Set input field to max 5 characters
  ui->inputField->setMaxLength(5);

  // Hide output area or make it smaller for solver results only
  ui->outputArea->setMaximumHeight(150);
  ui->outputArea->clear();
  ui->outputArea->append("Enter guesses to build feedback history");

  newGame();
}

WordleWidget::~WordleWidget() { delete ui; }

void WordleWidget::setupCurrentRow() {
  // Create a new row widget with 5 letter boxes
  currentRowWidget = new QWidget(this);
  QHBoxLayout *rowLayout = new QHBoxLayout(currentRowWidget);
  rowLayout->setSpacing(5);
  rowLayout->setContentsMargins(0, 5, 0, 5);

  for (int i = 0; i < 5; ++i) {
    currentBoxes[i] = new LetterBox(currentRowWidget);
    connect(currentBoxes[i], &LetterBox::clicked, this,
            &WordleWidget::onLetterBoxClicked);
    rowLayout->addWidget(currentBoxes[i]);
  }

  rowLayout->addStretch();

  // Insert before the stretch at end of guess list
  guessListLayout->insertWidget(guessListLayout->count() - 1, currentRowWidget);
}

void WordleWidget::rebuildFeedbackHistory() {
  feedbackHistory.clear();
  for (GuessRow *row : guessRows) {
    feedbackHistory.push_back(row->getFeedback());
  }
}

void WordleWidget::onGuessDeleted() {
  GuessRow *row = qobject_cast<GuessRow *>(sender());
  if (row) {
    // Remove from UI
    guessListLayout->removeWidget(row);
    row->deleteLater();

    // Remove from list
    auto it = std::find(guessRows.begin(), guessRows.end(), row);
    if (it != guessRows.end()) {
      guessRows.erase(it);
    }

    // Rebuild feedback history
    rebuildFeedbackHistory();
  }
}

void WordleWidget::onInputChanged(const QString &text) {
  // Update the letter boxes as user types
  QString upperText = text.toUpper();
  for (int i = 0; i < 5; ++i) {
    if (i < upperText.length()) {
      currentBoxes[i]->setLetter(upperText[i].toLatin1());
      // Set to grey (0) by default when letter is added
      if (currentBoxes[i]->getColor() != 0 &&
          currentBoxes[i]->getColor() != 1 &&
          currentBoxes[i]->getColor() != 2) {
        currentBoxes[i]->setColor(0);
      }
    } else {
      currentBoxes[i]->clear();
    }
  }
}

void WordleWidget::onLetterBoxClicked() {
  // Cycle through colors when a box is clicked
  LetterBox *box = qobject_cast<LetterBox *>(sender());
  if (box && box->getLetter() != '\0') {
    box->setColor(box->getColor() + 1);
  }
}

void WordleWidget::newGame() { onNewGame(); }

void WordleWidget::onSubmit() { submitCurrentGuess(); }

void WordleWidget::submitCurrentGuess() {
  // Build word from current boxes
  std::string word;
  for (int i = 0; i < 5; ++i) {
    char letter = currentBoxes[i]->getLetter();
    if (letter == '\0') {
      QMessageBox::information(this, "Incomplete Word",
                               "Please enter a 5-letter word!");
      return;
    }
    word += std::tolower(letter);
  }

  // Create feedback object and set colors
  Wordle::Feedback fb;
  fb.word = word;

  // Set feedback colors based on box colors
  for (int i = 0; i < 5; ++i) {
    int color = currentBoxes[i]->getColor();
    switch (color) {
    case 0: // Grey
      fb.setGrey(i);
      break;
    case 1: // Yellow
      fb.setYellow(i);
      break;
    case 2: // Green
      fb.setGreen(i);
      break;
    }
  }

  feedbackHistory.push_back(fb);

  // Remove current row from layout
  guessListLayout->removeWidget(currentRowWidget);
  currentRowWidget->deleteLater();

  // Create a GuessRow for this feedback
  GuessRow *guessRow = new GuessRow(fb, guessListWidget);
  guessRow->setEditable(true);
  connect(guessRow, &GuessRow::deleteRequested, this,
          &WordleWidget::onGuessDeleted);
  connect(guessRow, &GuessRow::editRequested, this,
          [this]() { rebuildFeedbackHistory(); });

  guessRows.push_back(guessRow);

  // Add to layout before stretch
  guessListLayout->insertWidget(guessListLayout->count() - 1, guessRow);

  // Clear input and create new row
  ui->inputField->clear();
  setupCurrentRow();
}

void WordleWidget::onNewGame() {
  // Clear feedback history
  feedbackHistory.clear();

  // Delete all GuessRow widgets
  for (GuessRow *row : guessRows) {
    guessListLayout->removeWidget(row);
    row->deleteLater();
  }
  guessRows.clear();

  // Remove current row if it exists
  if (currentRowWidget) {
    guessListLayout->removeWidget(currentRowWidget);
    currentRowWidget->deleteLater();
  }

  // Clear output
  ui->outputArea->clear();

  // Setup fresh current row
  setupCurrentRow();
  ui->inputField->clear();
  ui->inputField->setFocus();

  ui->outputArea->append("New Wordle game started!");
  ui->outputArea->append("\n1. Type a 5-letter word");
  ui->outputArea->append("2. Click on letters to cycle colors:");
  ui->outputArea->append("   Grey → Yellow → Green");
  ui->outputArea->append("3. Press Submit or Enter\n");
}

void WordleWidget::onHint() { solveWordle(); }

void WordleWidget::solveWordle() {
  ui->outputArea->append("\n--- Calculating best guesses ---");

  try {
    Wordle::Result result =
        Wordle::runWordleSolver(wordVec, feedbackHistory, config);

    ui->outputArea->append(QString("\nPossible words remaining: %1\n")
                               .arg(result.totalPossibleWords));

    if (!result.sortedGuesses.empty()) {
      ui->outputArea->append("=== Top 10 Suggestions ===");

      int limit = std::min(10, static_cast<int>(result.sortedGuesses.size()));
      for (int i = 0; i < limit; ++i) {
        const auto &guess = result.sortedGuesses[i];
        ui->outputArea->append(
            QString("%1. %2 (Score: %3, ENT: %4)")
                .arg(i + 1)
                .arg(QString::fromStdString(guess.word.wordString))
                .arg(guess.word.score, 0, 'f', 2)
                .arg(guess.ent, 0, 'f', 2));
      }
    }
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Solver Error",
                          QString("Error running solver: %1").arg(e.what()));
  }
}

#endif // WITH_GUI
