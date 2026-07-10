#ifdef WITH_GUI

#include "gui/HangmanWidget.hpp"
#include "ui_HangmanWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QSpinBox>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>
#include <cctype>
#include <unordered_set>

// ============ HangmanWidget Implementation ============

HangmanWidget::HangmanWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::HangmanWidget), patternInput(nullptr),
      excludedLettersInput(nullptr) {
  ui->setupUi(this);

  // Initialize config
  config.maxDepth = 1;
  config.excludeUncommonWords = true;
  config.wordPatterns = {{"____"}}; // Default: single 4-letter word

  // Setup the input widgets
  setupInputs();

  // Connect signals
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &HangmanWidget::onNewGame);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &HangmanWidget::onSettings);
  connect(ui->solveBtn, &QPushButton::clicked, this, &HangmanWidget::onSolve);
  connect(ui->letterSuggestionsTable, &QTableWidget::cellClicked, this,
          &HangmanWidget::onTableRowClicked);

  // Initialize game
  initGame();
}

HangmanWidget::~HangmanWidget() {
  cleanupSolverThread();
  delete ui;
}

void HangmanWidget::setupInputs() {
  // Create input section widget
  QWidget *inputWidget = new QWidget(this);
  QHBoxLayout *inputLayout = new QHBoxLayout(inputWidget);
  inputLayout->setContentsMargins(10, 5, 10, 5);

  // Pattern input
  QLabel *patternLabel = new QLabel("Pattern:", inputWidget);
  patternLabel->setToolTip(
      "Enter word patterns separated by spaces.\nUse _ for unknown "
      "letters.\nExample: _A__ _A_ _____ (4-letter word with A, 3-letter word "
      "with A, 5-letter word)");
  inputLayout->addWidget(patternLabel);

  patternInput = new QLineEdit(inputWidget);
  patternInput->setPlaceholderText("e.g., _A__ _A_ _____");
  patternInput->setText("____"); // Default pattern
  patternInput->setFont(QFont("Courier", 12));
  patternInput->setMinimumWidth(200);
  inputLayout->addWidget(patternInput);

  // Auto-update when pattern changes
  connect(patternInput, &QLineEdit::textChanged, this,
          &HangmanWidget::onPatternChanged);

  inputLayout->addSpacing(20);

  // Excluded letters input
  QLabel *excludedLabel = new QLabel("Not in word:", inputWidget);
  excludedLabel->setToolTip(
      "Enter letters that are NOT in the word (no spaces needed).\n"
      "Example: RSTLNE");
  inputLayout->addWidget(excludedLabel);

  excludedLettersInput = new QLineEdit(inputWidget);
  excludedLettersInput->setPlaceholderText("e.g., RSTLNE");
  excludedLettersInput->setFont(QFont("Courier", 12));
  excludedLettersInput->setMinimumWidth(150);
  inputLayout->addWidget(excludedLettersInput);

  // Auto-update when excluded letters change
  connect(excludedLettersInput, &QLineEdit::textChanged, this,
          &HangmanWidget::onExcludedLettersChanged);

  inputLayout->addStretch();

  // Insert input widget after instruction label
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
  if (mainLayout) {
    int instructionIndex = mainLayout->indexOf(ui->instructionLabel);
    if (instructionIndex >= 0) {
      mainLayout->insertWidget(instructionIndex + 1, inputWidget);
    } else {
      mainLayout->insertWidget(4, inputWidget);
    }
  }
}

void HangmanWidget::rebuildFeedbackFromInputs() {
  config.feedbackHistory.clear();

  // Collect all revealed letters from patterns for display purposes only
  // NOTE: We do NOT add revealed letters to feedbackHistory because:
  // 1. Pattern matching already handles revealed letters via matchesPattern()
  // 2. Adding them as "isInWord=true" feedback would incorrectly require
  //    EVERY word to contain ALL revealed letters (e.g., "AIR FLYIN?" would
  //    fail because "AIR" doesn't contain F, L, Y, N from the second word)
  std::unordered_set<char> revealedLetters;
  for (const auto &pattern : config.wordPatterns) {
    for (const auto &[pos, letter] : pattern.getRevealedLetters()) {
      revealedLetters.insert(letter);
    }
  }

  // Add feedback for excluded letters (they are NOT in the word)
  if (excludedLettersInput) {
    QString excluded = excludedLettersInput->text().toUpper();
    std::unordered_set<char> addedExcluded;
    for (QChar qc : excluded) {
      char c = qc.toLower().toLatin1();
      if (std::isalpha(static_cast<unsigned char>(c))) {
        // Don't add if already in revealed letters or already added
        if (revealedLetters.count(c) == 0 && addedExcluded.count(c) == 0) {
          Hangman::Feedback fb;
          fb.letter = c;
          fb.isInWord = false;
          config.feedbackHistory.push_back(fb);
          addedExcluded.insert(c);
        }
      }
    }
  }

  // Update the guessed label
  QString guessedStr;
  int count = 0;

  // Show revealed letters
  for (char c : revealedLetters) {
    if (!guessedStr.isEmpty())
      guessedStr += " ";
    guessedStr += QString("+%1").arg(QChar(std::toupper(c)));
    count++;
  }

  // Show excluded letters
  if (excludedLettersInput) {
    QString excluded = excludedLettersInput->text().toUpper();
    std::unordered_set<char> shown;
    for (QChar qc : excluded) {
      char c = qc.toLower().toLatin1();
      if (std::isalpha(static_cast<unsigned char>(c)) &&
          revealedLetters.count(c) == 0 && shown.count(c) == 0) {
        if (!guessedStr.isEmpty())
          guessedStr += " ";
        guessedStr += QString("-%1").arg(QChar(std::toupper(c)));
        count++;
        shown.insert(c);
      }
    }
  }

  if (count == 0) {
    ui->guessedLabel->setText("Known Letters: (none)");
  } else {
    ui->guessedLabel->setText(
        QString("Known Letters (%1): %2").arg(count).arg(guessedStr));
  }
}

bool HangmanWidget::showConfigDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Hangman Settings");

  QFormLayout *formLayout = new QFormLayout(&dialog);

  // Max depth
  QSpinBox *depthSpinBox = new QSpinBox(&dialog);
  depthSpinBox->setRange(0, 2);
  depthSpinBox->setValue(config.maxDepth);
  depthSpinBox->setToolTip(
      "Search depth for ENT calculation (0-2). Higher = slower but smarter.");
  formLayout->addRow("Search Depth:", depthSpinBox);

  // Exclude uncommon words
  QCheckBox *excludeUncommonCheckBox = new QCheckBox(&dialog);
  excludeUncommonCheckBox->setChecked(config.excludeUncommonWords);
  excludeUncommonCheckBox->setToolTip(
      "Only use common Scrabble words in suggestions.");
  formLayout->addRow("Exclude Uncommon Words:", excludeUncommonCheckBox);

  // Dialog buttons
  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
  formLayout->addRow(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

  if (dialog.exec() == QDialog::Accepted) {
    config.maxDepth = static_cast<uint8_t>(depthSpinBox->value());
    config.excludeUncommonWords = excludeUncommonCheckBox->isChecked();
    return true;
  }
  return false;
}

void HangmanWidget::initGame() {
  config.feedbackHistory.clear();
  lastResult = Hangman::Result();

  // Clear excluded letters input
  if (excludedLettersInput) {
    excludedLettersInput->clear();
  }

  // Clear tables
  ui->letterSuggestionsTable->setRowCount(0);
  ui->possibleWordsTable->setRowCount(0);

  // Update guessed label
  ui->guessedLabel->setText("Known Letters: (none)");

  updateConfigInfo();
  gameInitialized = true;
}

void HangmanWidget::setUIEnabled(bool enabled) {
  ui->newGameBtn->setEnabled(enabled);
  ui->settingsBtn->setEnabled(enabled);
  ui->solveBtn->setEnabled(enabled);
  if (patternInput)
    patternInput->setEnabled(enabled);
  if (excludedLettersInput)
    excludedLettersInput->setEnabled(enabled);
}

void HangmanWidget::updateConfigInfo() {
  QString patternStr =
      QString::fromStdString(Hangman::patternsToString(config.wordPatterns));
  QString info =
      QString("Pattern: %1 | Depth: %2").arg(patternStr).arg(config.maxDepth);
  ui->configInfoLabel->setText(info);
}

void HangmanWidget::onPatternChanged() {
  if (!patternInput)
    return;

  QString patternText = patternInput->text().trimmed();
  if (patternText.isEmpty()) {
    return; // Silently ignore empty patterns
  }

  // Parse the pattern
  std::vector<Hangman::WordPattern> newPatterns =
      Hangman::parsePatternString(patternText.toStdString());

  if (newPatterns.empty()) {
    return; // Silently ignore invalid patterns
  }

  // Validate patterns (only _ and letters allowed)
  for (const auto &pattern : newPatterns) {
    for (char c : pattern.pattern) {
      if (c != '_' && !std::isalpha(static_cast<unsigned char>(c))) {
        return; // Silently ignore invalid characters
      }
    }
  }

  config.wordPatterns = newPatterns;
  rebuildFeedbackFromInputs();
  updateConfigInfo();
}

void HangmanWidget::onExcludedLettersChanged() { rebuildFeedbackFromInputs(); }

void HangmanWidget::onSolve() {
  // Sync feedback from inputs before solving
  rebuildFeedbackFromInputs();
  solveHangman();
}

void HangmanWidget::onTableRowClicked(int row, int column) {
  (void)column;
  if (row < 0 || row >= static_cast<int>(lastResult.sortedGuesses.size()))
    return;

  // Get the letter from the clicked row and add it to excluded letters
  char letter = lastResult.sortedGuesses[row].letter;

  if (excludedLettersInput) {
    QString current = excludedLettersInput->text().toUpper();
    QChar letterChar = QChar(std::toupper(letter));

    // Only add if not already present
    if (!current.contains(letterChar)) {
      excludedLettersInput->setText(current + letterChar);
    }
  }
}

void HangmanWidget::newGame() { onNewGame(); }

void HangmanWidget::onNewGame() { initGame(); }

void HangmanWidget::onSettings() {
  if (showConfigDialog()) {
    initGame();
    updateConfigInfo();
  }
}

void HangmanWidget::populateResults(int maxRows) {
  // Populate letter suggestions table
  ui->letterSuggestionsTable->setRowCount(0);
  int letterCount =
      std::min(maxRows, static_cast<int>(lastResult.sortedGuesses.size()));
  ui->letterSuggestionsTable->setRowCount(letterCount);

  for (int i = 0; i < letterCount; ++i) {
    const auto &guess = lastResult.sortedGuesses[i];

    // Rank
    QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(i + 1));
    rankItem->setTextAlignment(Qt::AlignCenter);
    ui->letterSuggestionsTable->setItem(i, 0, rankItem);

    // Letter
    QTableWidgetItem *letterItem =
        new QTableWidgetItem(QString(QChar(std::toupper(guess.letter))));
    letterItem->setTextAlignment(Qt::AlignCenter);
    QFont font = letterItem->font();
    font.setBold(true);
    letterItem->setFont(font);
    ui->letterSuggestionsTable->setItem(i, 1, letterItem);

    // ENT Score
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    ui->letterSuggestionsTable->setItem(i, 2, entItem);

    // Probability
    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 1) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    ui->letterSuggestionsTable->setItem(i, 3, probItem);
  }

  // Evenly space columns
  ui->letterSuggestionsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  // Populate possible words table
  ui->possibleWordsTable->setRowCount(0);
  int wordCount =
      std::min(maxRows, static_cast<int>(lastResult.possibleWords.size()));
  ui->possibleWordsTable->setRowCount(wordCount);

  for (int i = 0; i < wordCount; ++i) {
    const auto &word = lastResult.possibleWords[i];

    // Number
    QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(i + 1));
    numItem->setTextAlignment(Qt::AlignCenter);
    ui->possibleWordsTable->setItem(i, 0, numItem);

    // Word
    QString wordStr = QString::fromStdString(word.wordString).toUpper();
    QTableWidgetItem *wordItem = new QTableWidgetItem(wordStr);
    wordItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->possibleWordsTable->setItem(i, 1, wordItem);

    // Score
    QTableWidgetItem *scoreItem =
        new QTableWidgetItem(QString::number(word.score, 'f', 1));
    scoreItem->setTextAlignment(Qt::AlignCenter);
    ui->possibleWordsTable->setItem(i, 2, scoreItem);
  }

  // Evenly space columns
  ui->possibleWordsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  // Update title to show count
  ui->resultsTabWidget->setTabText(
      0,
      QString("Letter Suggestions (%1)").arg(lastResult.sortedGuesses.size()));
  ui->resultsTabWidget->setTabText(
      1, QString("Possible Words (%1)").arg(lastResult.totalPossibleWords));
}

void HangmanWidget::solveHangman() {
  if (isSolverRunning())
    return;

  setUIEnabled(false);

  // Create worker thread
  cleanupSolverThread();
  solverThread = new QThread();

  // Copy config for thread
  Hangman::Config threadConfig = config;

  // Create progress dialog
  createProgressDialog("Calculating best letters...");

  // Get pointer to atomic flag
  std::atomic<bool> *cancelFlag = &cancellationRequested;

  // Move work to thread
  QObject *context = new QObject();
  context->moveToThread(solverThread);

  connect(solverThread, &QThread::started, context,
          [this, threadConfig, cancelFlag]() {
            lastResult = Hangman::runHangmanSolver(threadConfig, cancelFlag);
            // Quit the thread's event loop so it can finish
            if (QThread::currentThread())
              QThread::currentThread()->quit();
            QMetaObject::invokeMethod(this, "onSolverFinished",
                                      Qt::QueuedConnection);
          });

  connect(solverThread, &QThread::finished, context, &QObject::deleteLater);

  solverThread->start();
}

void HangmanWidget::onSolverFinished() {
  // Clean up the progress dialog first
  cleanupProgressDialog();

  // Clean up the solver thread
  cleanupSolverThread();

  // Check if cancelled
  if (cancellationRequested.load(std::memory_order_acquire)) {
    setUIEnabled(true);
    return;
  }

  setUIEnabled(true);

  if (lastResult.totalPossibleWords == 0) {
    QMessageBox::information(this, "No Matches",
                             "No words match the given constraints.");
    return;
  }

  populateResults();
}

#endif // WITH_GUI
