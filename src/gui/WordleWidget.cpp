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
    boxes[i]->setColor(static_cast<int>(feedback.getColor(i)));
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

  // Initialize config (match web frontend defaults)
  config.maxDepth = 1;
  config.autoDepth = true;
  config.excludeUncommonWords = true;
  config.maxGuesses = 6;

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
  connect(ui->addOnlyBtn, &QPushButton::clicked, this,
          &WordleWidget::onAddOnly);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &WordleWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this, &WordleWidget::onHint);
  connect(ui->inputField, &QLineEdit::textChanged, this,
          &WordleWidget::onInputChanged);
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &WordleWidget::onInputReturn);

  // Set input field to max word length (default 5, configurable up to 32)
  ui->inputField->setMaxLength(config.wordLength);

  // Connect settings button
  ui->settingsBtn->setToolTip("Solver Settings");
  ui->settingsBtn->setMaximumWidth(40);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &WordleWidget::onSettings);

  // Get result tables from UI and configure them
  allResultsTable = ui->allResultsTable;
  allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  allResultsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");
  connect(allResultsTable, &QTableWidget::cellClicked, this,
          &WordleWidget::onTableRowClicked);

  probableWordsTable = ui->probableWordsTable;
  probableWordsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  probableWordsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");
  connect(probableWordsTable, &QTableWidget::cellClicked, this,
          &WordleWidget::onTableRowClicked);

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
  QDialog dialog(this);
  dialog.setWindowTitle("Wordle Settings");
  dialog.setMinimumWidth(320);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

  QSpinBox *wordLengthSpinner = new QSpinBox(&dialog);
  wordLengthSpinner->setRange(1, 32);
  wordLengthSpinner->setValue(config.wordLength);
  formLayout->addRow("Word Length:", wordLengthSpinner);

  QCheckBox *autoDepthCheckBox = new QCheckBox(&dialog);
  autoDepthCheckBox->setChecked(config.autoDepth);
  autoDepthCheckBox->setToolTip(
      "Dynamically choose search depth based on available time.");
  formLayout->addRow("Auto Depth (Recommended):", autoDepthCheckBox);

  QSpinBox *maxDepthSpinner = new QSpinBox(&dialog);
  maxDepthSpinner->setRange(0, 2);
  maxDepthSpinner->setValue(config.maxDepth);
  maxDepthSpinner->setEnabled(!config.autoDepth);
  maxDepthSpinner->setToolTip(
      "0: Fastest, 1: Balanced, 2: Deep. Disabled when Auto Depth is on.");
  formLayout->addRow("Manual Search Depth:", maxDepthSpinner);

  connect(autoDepthCheckBox, &QCheckBox::toggled, maxDepthSpinner,
          [maxDepthSpinner](bool checked) { maxDepthSpinner->setEnabled(!checked); });

  QSpinBox *maxGuessesSpinner = new QSpinBox(&dialog);
  maxGuessesSpinner->setRange(1, 100);
  maxGuessesSpinner->setValue(static_cast<int>(config.maxGuesses));
  formLayout->addRow("Maximum Guesses Allowed:", maxGuessesSpinner);

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
    config.autoDepth = autoDepthCheckBox->isChecked();
    config.maxDepth = maxDepthSpinner->value();
    config.maxGuesses = static_cast<uint32_t>(maxGuessesSpinner->value());
    config.excludeUncommonWords = excludeCheckbox->isChecked();

    ui->inputField->setMaxLength(config.wordLength);

    if (oldWordLength != config.wordLength) {
      config.feedbackHistory.clear();

      for (GuessRow *row : guessRows) {
        guessListLayout->removeWidget(row);
        row->deleteLater();
      }
      guessRows.clear();

      if (currentRowWidget) {
        guessListLayout->removeWidget(currentRowWidget);
        currentRowWidget->deleteLater();
      }

      allResultsTable->setRowCount(0);
      probableWordsTable->setRowCount(0);

      setupCurrentRow();
      ui->inputField->clear();
    }

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

  // Reset tab texts to match frontend
  ui->resultsTabWidget->setTabText(0, "Suggested Guesses");
  ui->resultsTabWidget->setTabText(1, "Possible Words");

  // Setup fresh current row
  setupCurrentRow();
  ui->inputField->clear();
  ui->inputField->setFocus();

  updateConfigInfo();
}

void WordleWidget::setUIEnabled(bool enabled) {
  ui->inputField->setVisible(enabled);
  ui->submitBtn->setVisible(enabled);
  ui->addOnlyBtn->setVisible(enabled);
  ui->solveBtn->setVisible(enabled);
  guessListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
}

void WordleWidget::updateConfigInfo() {
  QString depthStr = config.autoDepth ? QStringLiteral("auto")
                                      : QString::number(config.maxDepth);
  QString info =
      QString("<span style='color:#666; font-size:11pt;'>%1 letters | Depth: "
              "%2 | Max guesses: %3 | %4</span>")
          .arg(config.wordLength)
          .arg(depthStr)
          .arg(config.maxGuesses)
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

  // Prefer cached results (avoids picking Probability/ENT/WNT cells)
  QString word;
  if (table == allResultsTable && row >= 0 &&
      row < static_cast<int>(lastAllResults.size())) {
    word = QString::fromStdString(lastAllResults[row].word.wordString);
  } else if (table == probableWordsTable && row >= 0 &&
             row < static_cast<int>(lastProbableResults.size())) {
    word = QString::fromStdString(lastProbableResults[row].word.wordString);
  } else {
    QTableWidgetItem *wordItem = table->item(row, 0); // Word column
    if (!wordItem)
      return;
    word = wordItem->text();
  }

  word = word.toLower();
  ui->inputField->setText(word);
  submitCurrentGuess();
}

void WordleWidget::newGame() { onNewGame(); }

void WordleWidget::onSubmit() {
  if (submitCurrentGuess()) {
    solveWordle();
  }
}

void WordleWidget::onAddOnly() { submitCurrentGuess(); }

void WordleWidget::onInputReturn() {
  // Match frontend: Enter adds without solving; empty input + history solves
  const QString text = ui->inputField->text().trimmed();
  if (text.length() == config.wordLength) {
    onAddOnly();
  } else if (text.isEmpty() && !config.feedbackHistory.empty()) {
    solveWordle();
  }
}

bool WordleWidget::submitCurrentGuess() {
  // Get word from input field
  QString inputText = ui->inputField->text().trimmed();
  if (inputText.length() != config.wordLength) {
    QMessageBox::information(
        this, "Incomplete Word",
        QString("Please enter a %1-letter word!").arg(config.wordLength));
    return false;
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
  return true;
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

void WordleWidget::populateResults(int maxRows) {
  // Match frontend columns: Word | Probability | ENT | WNT
  auto setupHeaders = [](QTableWidget *table) {
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Word"), QStringLiteral("Probability"),
         QStringLiteral("ENT"), QStringLiteral("WNT")});
  };
  setupHeaders(allResultsTable);
  setupHeaders(probableWordsTable);

  const auto &all = lastAllResults;
  int allRows = std::min(maxRows, static_cast<int>(all.size()));
  allResultsTable->setRowCount(allRows);
  for (int i = 0; i < allRows; ++i) {
    allResultsTable->setRowHeight(i, 24);
    const auto &guess = all[i];

    QTableWidgetItem *wordItem = new QTableWidgetItem(
        QString::fromStdString(guess.word.wordString).toUpper());
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    wordItem->setFont(monoFont);
    wordItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 0, wordItem);

    QTableWidgetItem *probItem =
        new QTableWidgetItem(formatProbabilityPercent(guess.probability));
    probItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 1, probItem);

    QTableWidgetItem *entItem =
        new QTableWidgetItem(formatRoundedNum(guess.ent));
    entItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 2, entItem);

    QTableWidgetItem *wntItem =
        new QTableWidgetItem(formatRoundedNum(guess.wnt));
    wntItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 3, wntItem);

    applyProbabilityRowColors(allResultsTable, i, 4, guess.probability);
  }

  std::vector<Wordle::WordGuess> probable;
  probable.reserve(all.size());
  for (const auto &g : all) {
    if (g.probability > 0.0)
      probable.push_back(g);
  }

  int probRows = std::min(maxRows, static_cast<int>(probable.size()));
  probableWordsTable->setRowCount(probRows);
  for (int r = 0; r < probRows; ++r) {
    probableWordsTable->setRowHeight(r, 24);
    const auto &guess = probable[r];

    QTableWidgetItem *wordItem = new QTableWidgetItem(
        QString::fromStdString(guess.word.wordString).toUpper());
    QFont monoFont2("Consolas", 10);
    monoFont2.setBold(true);
    wordItem->setFont(monoFont2);
    wordItem->setTextAlignment(Qt::AlignCenter);
    probableWordsTable->setItem(r, 0, wordItem);

    QTableWidgetItem *probItem =
        new QTableWidgetItem(formatProbabilityPercent(guess.probability));
    probItem->setTextAlignment(Qt::AlignCenter);
    probableWordsTable->setItem(r, 1, probItem);

    QTableWidgetItem *entItem =
        new QTableWidgetItem(formatRoundedNum(guess.ent));
    entItem->setTextAlignment(Qt::AlignCenter);
    probableWordsTable->setItem(r, 2, entItem);

    QTableWidgetItem *wntItem =
        new QTableWidgetItem(formatRoundedNum(guess.wnt));
    wntItem->setTextAlignment(Qt::AlignCenter);
    probableWordsTable->setItem(r, 3, wntItem);

    applyProbabilityRowColors(probableWordsTable, r, 4, guess.probability);
  }

  lastProbableResults = std::move(probable);

  // Tab titles match frontend naming
  ui->resultsTabWidget->setTabText(
      0, QString("Suggested Guesses (%1)").arg(allResultsTable->rowCount()));
  ui->resultsTabWidget->setTabText(
      1, QString("Possible Words (%1)").arg(probableWordsTable->rowCount()));
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
  ui->addOnlyBtn->setEnabled(false);
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
  ui->addOnlyBtn->setEnabled(true);
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
      // Cache and populate both tables
      lastAllResults = result.sortedGuesses;
      populateResults(1000);

      // Update tab labels
      ui->resultsTabWidget->setTabText(
          0, QString("All Suggestions (%1)").arg(lastAllResults.size()));
      // Count probable entries
      int probableCount = 0;
      for (const auto &g : lastAllResults)
        if (g.probability > 0.0)
          ++probableCount;
      ui->resultsTabWidget->setTabText(
          1, QString("Possible Solutions (%1)").arg(probableCount));
    } else {
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
