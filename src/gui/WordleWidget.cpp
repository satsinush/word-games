#ifdef WITH_GUI

#include "gui/WordleWidget.hpp"
#include "ui_WordleWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QTableWidgetItem>
#include <QVBoxLayout>
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
  // Always default to grey when a letter is set
  if (currentLetter != '\0' && currentLetter != ' ') {
    currentColor = 0;
    updateStyle();
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
      currentRowWidget(nullptr), gameInitialized(true) {
  ui->setupUi(this);

  // Initialize config
  config.maxDepth = 1;
  config.excludeUncommonWords = true;

  // Create guess list container
  guessListWidget = new QWidget(this);
  guessListLayout = new QVBoxLayout(guessListWidget);
  guessListLayout->setSpacing(2);
  guessListLayout->setContentsMargins(0, 0, 0, 0);
  guessListLayout->setAlignment(Qt::AlignHCenter); // Center the guess list
  guessListLayout->addStretch();

  // Wrap guess list in scroll area
  guessListScrollArea = new QScrollArea(this);
  guessListScrollArea->setWidget(guessListWidget);
  guessListScrollArea->setWidgetResizable(true);
  guessListScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  guessListScrollArea->setMaximumHeight(
      200); // Limit height to make it scrollable
  guessListScrollArea->setFrameShape(QFrame::NoFrame);

  // Add scroll area to main layout after input row and before Solve button
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
  if (mainLayout) {
    int hintBtnIndex = mainLayout->indexOf(ui->hintBtn);
    if (hintBtnIndex >= 0) {
      mainLayout->insertWidget(hintBtnIndex, guessListScrollArea);
    }
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

  // Add settings button next to New Game button
  QPushButton *settingsBtn = new QPushButton("⚙", this);
  settingsBtn->setToolTip("Solver Settings");
  settingsBtn->setMaximumWidth(40);
  connect(settingsBtn, &QPushButton::clicked, this, &WordleWidget::onSettings);

  // Find the top control layout and add settings button after New Game
  QHBoxLayout *topLayout =
      ui->newGameBtn->parentWidget()->findChild<QHBoxLayout *>(
          "topControlLayout");
  if (!topLayout) {
    topLayout =
        qobject_cast<QHBoxLayout *>(ui->newGameBtn->parentWidget()->layout());
  }
  if (topLayout) {
    int index = topLayout->indexOf(ui->newGameBtn);
    if (index >= 0) {
      topLayout->insertWidget(index + 1, settingsBtn);
    }
  }

  // Create result tables
  allResultsTable = new QTableWidget(this);
  allResultsTable->setColumnCount(5);
  allResultsTable->setHorizontalHeaderLabels(
      {"Rank", "Word", "Word Score", "ENT", "Probability %"});
  allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  allResultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  allResultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  allResultsTable->setAlternatingRowColors(true);
  connect(allResultsTable, &QTableWidget::cellClicked, this,
          &WordleWidget::onTableRowClicked);

  probableWordsTable = new QTableWidget(this);
  probableWordsTable->setColumnCount(5);
  probableWordsTable->setHorizontalHeaderLabels(
      {"Rank", "Word", "Word Score", "ENT", "Probability %"});
  probableWordsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  probableWordsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  probableWordsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  probableWordsTable->setAlternatingRowColors(true);
  connect(probableWordsTable, &QTableWidget::cellClicked, this,
          &WordleWidget::onTableRowClicked);

  // Add tables to tab widget
  QVBoxLayout *allResultsLayout = new QVBoxLayout();
  allResultsLayout->addWidget(allResultsTable);
  ui->resultsTabWidget->widget(0)->setLayout(allResultsLayout);

  QVBoxLayout *probableWordsLayout = new QVBoxLayout();
  probableWordsLayout->addWidget(probableWordsTable);
  ui->resultsTabWidget->widget(1)->setLayout(probableWordsLayout);

  // Store reference to config info label (we'll use the title)
  configInfoLabel = ui->titleLabel;

  // Initialize game immediately with default configuration
  initGame();
  updateConfigInfo();
}

WordleWidget::~WordleWidget() { delete ui; }

bool WordleWidget::showConfigDialog() {
  // Show current configuration
  QDialog dialog(this);
  dialog.setWindowTitle("Wordle Solver Configuration");
  dialog.setMinimumWidth(400);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);

  QLabel *infoLabel = new QLabel(
      "<b>Current Configuration:</b><br><br>"
      "• Max Search Depth: " +
          QString::number(config.maxDepth) +
          "<br>"
          "• Exclude Uncommon Words: " +
          QString(config.excludeUncommonWords ? "Yes" : "No") +
          "<br><br>"
          "<i>Note: Full configuration options (word length, etc.) will be "
          "available in a future update.</i>",
      &dialog);
  infoLabel->setWordWrap(true);
  layout->addWidget(infoLabel);

  QDialogButtonBox *buttonBox =
      new QDialogButtonBox(QDialogButtonBox::Ok, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  layout->addWidget(buttonBox);

  dialog.exec();
  return false; // No changes made
}

void WordleWidget::initGame() {
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

  // Clear result tables
  allResultsTable->setRowCount(0);
  probableWordsTable->setRowCount(0);

  // Reset tab texts to default
  ui->resultsTabWidget->setTabText(0, "All Suggestions");
  ui->resultsTabWidget->setTabText(1, "Possible Solutions");

  // Setup fresh current row
  setupCurrentRow();
  ui->inputField->clear();
  ui->inputField->setFocus();

  updateConfigInfo();
}

void WordleWidget::setUIEnabled(bool enabled) {
  ui->inputField->setVisible(enabled);
  ui->submitBtn->setVisible(enabled);
  ui->hintBtn->setVisible(enabled);
  guessListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
}

void WordleWidget::updateConfigInfo() {
  configInfoLabel->setText("Wordle Solver");
}

void WordleWidget::setupCurrentRow() {
  // Don't create letter boxes until word is submitted
  // Just create an empty widget placeholder
  currentRowWidget = new QWidget(this);
  QHBoxLayout *rowLayout = new QHBoxLayout(currentRowWidget);
  rowLayout->setSpacing(5);
  rowLayout->setContentsMargins(0, 5, 0, 5);

  // Initialize currentBoxes to nullptr
  for (int i = 0; i < 5; ++i) {
    currentBoxes[i] = nullptr;
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
  // Letter boxes are not shown until word is submitted
  // So we don't need to update them here
  Q_UNUSED(text);
}

void WordleWidget::onLetterBoxClicked() {
  // Cycle through colors when a box is clicked
  LetterBox *box = qobject_cast<LetterBox *>(sender());
  if (box && box->getLetter() != '\0') {
    box->setColor(box->getColor() + 1);
  }
}

void WordleWidget::onTableRowClicked(int row, int column) {
  Q_UNUSED(column);

  // Get the table that was clicked
  QTableWidget *table = qobject_cast<QTableWidget *>(sender());
  if (!table)
    return;

  // Get the word from column 1 (Word column)
  QTableWidgetItem *wordItem = table->item(row, 1);
  if (!wordItem)
    return;

  QString word = wordItem->text().toLower(); // Convert back to lowercase

  // Set the word in the input field
  ui->inputField->setText(word);

  // Auto-submit the word
  submitCurrentGuess();
}

void WordleWidget::newGame() { onNewGame(); }

void WordleWidget::onSubmit() { submitCurrentGuess(); }

void WordleWidget::submitCurrentGuess() {
  // Get word from input field
  QString inputText = ui->inputField->text().trimmed();
  if (inputText.length() != 5) {
    QMessageBox::information(this, "Incomplete Word",
                             "Please enter a 5-letter word!");
    return;
  }

  std::string word = inputText.toLower().toStdString();

  // Create feedback object with all letters set to grey by default
  Wordle::Feedback fb;
  fb.word = word;

  // Set all letters to grey initially
  for (int i = 0; i < 5; ++i) {
    fb.setGrey(i);
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
  initGame();
  updateConfigInfo();
}

void WordleWidget::onHint() { solveWordle(); }

void WordleWidget::onSettings() { showConfigDialog(); }

void WordleWidget::populateResultTable(
    QTableWidget *table, const std::vector<Wordle::WordGuess> &guesses,
    int maxRows, int startRank) {
  table->setRowCount(0);

  int numRows = std::min(maxRows, static_cast<int>(guesses.size()));
  table->setRowCount(numRows);

  for (int i = 0; i < numRows; ++i) {
    const auto &guess = guesses[i];

    // Rank
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(startRank + i));
    rankItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 0, rankItem);

    // Word - uppercase and monospace font
    QTableWidgetItem *wordItem = new QTableWidgetItem(
        QString::fromStdString(guess.word.wordString).toUpper());
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    wordItem->setFont(monoFont);
    wordItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 1, wordItem);

    // Word Score - 3 decimal places
    QTableWidgetItem *scoreItem =
        new QTableWidgetItem(QString::number(guess.word.score, 'f', 3));
    scoreItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 2, scoreItem);

    // ENT - 3 decimal places
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 3, entItem);

    // Probability - as percentage
    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 4, probItem);

    // Color highlighting based on probability
    if (guess.probability >= 1.0) {
      // Green for 100% probability
      for (int col = 0; col < 5; ++col) {
        table->item(i, col)->setBackground(
            QColor(144, 238, 144));                          // Light green
        table->item(i, col)->setForeground(QColor(0, 0, 0)); // Black text
      }
    } else if (guess.probability > 0.0) {
      // Yellow for possible words (probability > 0 but < 100%)
      for (int col = 0; col < 5; ++col) {
        table->item(i, col)->setBackground(
            QColor(255, 255, 153));                          // Light yellow
        table->item(i, col)->setForeground(QColor(0, 0, 0)); // Black text
      }
    }
  }
}

void WordleWidget::solveWordle() {
  try {
    Wordle::Result result =
        Wordle::runWordleSolver(wordVec, feedbackHistory, config);

    if (!result.sortedGuesses.empty()) {
      // Populate all results table (show ALL results)
      populateResultTable(allResultsTable, result.sortedGuesses,
                          result.sortedGuesses.size(), 1);

      // Filter and populate probable words table, tracking original ranks
      std::vector<Wordle::WordGuess> probableWords;
      std::vector<int> originalRanks;

      for (size_t i = 0; i < result.sortedGuesses.size(); ++i) {
        const auto &guess = result.sortedGuesses[i];
        if (guess.probability > 0.0) {
          probableWords.push_back(guess);
          originalRanks.push_back(i + 1); // Track original rank
        }
      }

      if (!probableWords.empty()) {
        // Manually populate probable words table with original ranks
        probableWordsTable->setRowCount(0);

        int numRows = std::min(50, static_cast<int>(probableWords.size()));
        probableWordsTable->setRowCount(numRows);

        for (int i = 0; i < numRows; ++i) {
          const auto &guess = probableWords[i];
          int originalRank = originalRanks[i];

          // Rank - use original rank from all results
          QTableWidgetItem *rankItem =
              new QTableWidgetItem(QString::number(originalRank));
          rankItem->setTextAlignment(Qt::AlignCenter);
          probableWordsTable->setItem(i, 0, rankItem);

          // Word - uppercase and monospace font
          QTableWidgetItem *wordItem = new QTableWidgetItem(
              QString::fromStdString(guess.word.wordString).toUpper());
          QFont monoFont("Consolas", 10);
          monoFont.setBold(true);
          wordItem->setFont(monoFont);
          wordItem->setTextAlignment(Qt::AlignCenter);
          probableWordsTable->setItem(i, 1, wordItem);

          // Word Score - 3 decimal places
          QTableWidgetItem *scoreItem =
              new QTableWidgetItem(QString::number(guess.word.score, 'f', 3));
          scoreItem->setTextAlignment(Qt::AlignCenter);
          probableWordsTable->setItem(i, 2, scoreItem);

          // ENT - 3 decimal places
          QTableWidgetItem *entItem =
              new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
          entItem->setTextAlignment(Qt::AlignCenter);
          probableWordsTable->setItem(i, 3, entItem);

          // Probability - as percentage
          QTableWidgetItem *probItem = new QTableWidgetItem(
              QString::number(guess.probability * 100.0, 'f', 2) + "%");
          probItem->setTextAlignment(Qt::AlignCenter);
          probableWordsTable->setItem(i, 4, probItem);

          // Color highlighting based on probability
          if (guess.probability >= 1.0) {
            // Green for 100% probability
            for (int col = 0; col < 5; ++col) {
              probableWordsTable->item(i, col)->setBackground(
                  QColor(144, 238, 144)); // Light green
              probableWordsTable->item(i, col)->setForeground(QColor(0, 0, 0));
            }
          } else if (guess.probability > 0.0) {
            // Yellow for possible words (probability > 0 but < 100%)
            for (int col = 0; col < 5; ++col) {
              probableWordsTable->item(i, col)->setBackground(
                  QColor(255, 255, 153)); // Light yellow
              probableWordsTable->item(i, col)->setForeground(QColor(0, 0, 0));
            }
          }
        }

        ui->resultsTabWidget->setTabText(
            1, QString("Possible Solutions (%1)").arg(probableWords.size()));
      } else {
        probableWordsTable->setRowCount(0);
        ui->resultsTabWidget->setTabText(1, "Possible Solutions (0)");
      }

      ui->resultsTabWidget->setTabText(
          0, QString("All Suggestions (%1)").arg(result.sortedGuesses.size()));

    } else {
      allResultsTable->setRowCount(0);
      probableWordsTable->setRowCount(0);
      QMessageBox::information(this, "No Results", "No suggestions available");
    }
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Solver Error",
                          QString("Error running solver: %1").arg(e.what()));
  }
}

#endif // WITH_GUI
