#ifdef WITH_GUI

#include "gui/WordleWidget.hpp"
#include "ui_WordleWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

// ============ LetterBox Implementation ============

LetterBox::LetterBox(QWidget *parent)
    : QLabel(parent), currentLetter('\0'), currentColor(0) {
  setFixedSize(40, 40);
  setAlignment(Qt::AlignCenter);
  QFont font;
  font.setPointSize(18);
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

  // Create letter boxes based on word length
  size_t wordLen = feedback.word.size();
  boxes.resize(wordLen);
  for (size_t i = 0; i < wordLen; ++i) {
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
  for (size_t i = 0; i < boxes.size(); ++i) {
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
  for (size_t i = 0; i < boxes.size(); ++i) {
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

WordleWidget::WordleWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::WordleWidget), currentRowWidget(nullptr) {
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
  // guessListScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  guessListScrollArea->setMaximumHeight(
      200); // Limit height to make it scrollable
  guessListScrollArea->setFrameShape(QFrame::NoFrame);

  // Add scroll area to main layout after input row and before Solve button
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
  if (mainLayout) {
    int solveBtnIndex = mainLayout->indexOf(ui->solveBtn);
    if (solveBtnIndex >= 0) {
      mainLayout->insertWidget(solveBtnIndex, guessListScrollArea);
    }
  }

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this, &WordleWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &WordleWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this, &WordleWidget::onHint);
  connect(ui->inputField, &QLineEdit::textChanged, this,
          &WordleWidget::onInputChanged);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &WordleWidget::onSubmit);

  // Set input field to max word length (default 5, configurable up to 32)
  ui->inputField->setMaxLength(config.wordLength);

  // Connect settings button
  ui->settingsBtn->setToolTip("Solver Settings");
  ui->settingsBtn->setMaximumWidth(40);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &WordleWidget::onSettings);

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

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Initialize game immediately with default configuration
  gameInitialized = true;
  initGame();
  updateConfigInfo();
}

WordleWidget::~WordleWidget() {
  // Base class destructor handles thread and dialog cleanup
  delete ui;
}

bool WordleWidget::showConfigDialog() {
  // Show configuration dialog
  QDialog dialog(this);
  dialog.setWindowTitle("Wordle Solver Configuration");
  dialog.setMinimumWidth(400);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

  // Word Length
  QSpinBox *wordLengthSpinner = new QSpinBox(&dialog);
  wordLengthSpinner->setRange(1, 32);
  wordLengthSpinner->setValue(config.wordLength);
  formLayout->addRow("Word Length:", wordLengthSpinner);

  // Max Depth
  QSpinBox *maxDepthSpinner = new QSpinBox(&dialog);
  maxDepthSpinner->setRange(0, 2);
  maxDepthSpinner->setValue(config.maxDepth);
  formLayout->addRow("Search Depth:", maxDepthSpinner);

  // Exclude Uncommon Words
  QCheckBox *excludeCheckbox = new QCheckBox(&dialog);
  excludeCheckbox->setChecked(config.excludeUncommonWords);
  formLayout->addRow("Exclude Uncommon Words:", excludeCheckbox);

  layout->addLayout(formLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    uint8_t oldWordLength = config.wordLength;

    config.wordLength = wordLengthSpinner->value();
    config.maxDepth = maxDepthSpinner->value();
    config.excludeUncommonWords = excludeCheckbox->isChecked();

    // Update input field max length
    ui->inputField->setMaxLength(config.wordLength);

    // Clear feedback history if word length changed
    if (oldWordLength != config.wordLength) {
      config.feedbackHistory.clear();

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

      // Setup fresh current row
      setupCurrentRow();
      ui->inputField->clear();
    }

    updateConfigInfo();
    return true;
  }
  return false;
}

void WordleWidget::initGame() {
  // Clear feedback history
  config.feedbackHistory.clear();

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
  ui->solveBtn->setVisible(enabled);
  guessListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
}

void WordleWidget::updateConfigInfo() {
  QString info =
      QString("<span style='color:#666; font-size:11pt;'>%1 letters | Search "
              "Depth: "
              "%2 | %3</span>")
          .arg(config.wordLength)
          .arg(config.maxDepth)
          .arg(config.excludeUncommonWords ? "Common words" : "All words");
  configInfoLabel->setText(info);
}

void WordleWidget::setupCurrentRow() {
  // Don't create letter boxes until word is submitted
  // Just create an empty widget placeholder
  currentRowWidget = new QWidget(this);
  QHBoxLayout *rowLayout = new QHBoxLayout(currentRowWidget);
  rowLayout->setSpacing(5);
  rowLayout->setContentsMargins(0, 5, 0, 5);

  // Clear currentBoxes
  currentBoxes.clear();

  rowLayout->addStretch();

  // Insert before the stretch at end of guess list
  guessListLayout->insertWidget(guessListLayout->count() - 1, currentRowWidget);
}

void WordleWidget::rebuildFeedbackHistory() {
  config.feedbackHistory.clear();
  for (GuessRow *row : guessRows) {
    config.feedbackHistory.push_back(row->getFeedback());
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
  if (inputText.length() != config.wordLength) {
    QMessageBox::information(
        this, "Incomplete Word",
        QString("Please enter a %1-letter word!").arg(config.wordLength));
    return;
  }

  std::string word = inputText.toLower().toStdString();

  // Create feedback object with all letters set to grey by default
  Wordle::Feedback fb;
  fb.word = word;

  // Set all letters to grey initially
  for (size_t i = 0; i < word.length(); ++i) {
    fb.setGrey(i);
  }

  config.feedbackHistory.push_back(fb);

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

void WordleWidget::onSettings() {
  showConfigDialog();
  updateConfigInfo();
}

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
  // Clean up any existing thread and dialog using base class methods
  cleanupSolverThread();
  cleanupProgressDialog();

  // Reset cancellation flag
  cancellationRequested.store(false, std::memory_order_release);

  // Create progress dialog using base class method
  createProgressDialog("Solving Wordle...", 0, 0);

  // Disable UI elements that could interfere with solving
  ui->inputField->setEnabled(false);
  ui->submitBtn->setEnabled(false);
  ui->solveBtn->setEnabled(false);
  ui->newGameBtn->setEnabled(false);
  ui->settingsBtn->setEnabled(false);
  ui->resultsTabWidget->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(config, &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &WordleWidget::onSolverFinished);
  solverThread->start();
}

void WordleWidget::onSolverFinished() {
  // Re-enable UI elements
  ui->inputField->setEnabled(true);
  ui->submitBtn->setEnabled(true);
  ui->solveBtn->setEnabled(true);
  ui->newGameBtn->setEnabled(true);
  ui->settingsBtn->setEnabled(true);
  ui->resultsTabWidget->setEnabled(true);

  // Use common handler - returns false if cancelled
  if (!handleSolverFinished()) {
    return;
  }

  try {
    Wordle::Result result =
        static_cast<SolverThread *>(solverThread)->getResult();

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
      // No suggestions found - only show message if not cancelled (which we
      // already handled)
      QMessageBox::information(this, "No Results", "No suggestions available");
    }
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Solver Error",
                          QString("Error running solver: %1").arg(e.what()));
  }

  // Clean up (non-blocking)
  cleanupProgressDialog();
  cleanupSolverThread();
}

#endif // WITH_GUI
