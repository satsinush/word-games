#ifdef WITH_GUI

#include "gui/MastermindWidget.hpp"
#include "ui_MastermindWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>
#include <sstream>

namespace {
QPushButton *makeStepButton(const QString &text, QWidget *parent) {
  auto *btn = new QPushButton(text, parent);
  btn->setFixedSize(22, 22);
  btn->setCursor(Qt::PointingHandCursor);
  btn->setFocusPolicy(Qt::NoFocus);
  btn->setStyleSheet(
      "QPushButton {"
      "  background-color: transparent;"
      "  color: #ffffff;"
      "  border: 1px solid #888888;"
      "  border-radius: 3px;"
      "  font-size: 12px;"
      "  font-weight: bold;"
      "  padding: 0;"
      "}"
      "QPushButton:hover { background-color: rgba(255,255,255,0.15); "
      "border-color: #ffffff; }"
      "QPushButton:pressed { background-color: rgba(255,255,255,0.25); }");
  return btn;
}

QLabel *makeCountLabel(QWidget *parent) {
  auto *label = new QLabel(QStringLiteral("0"), parent);
  QFont font = label->font();
  font.setBold(true);
  label->setFont(font);
  label->setMinimumWidth(14);
  label->setAlignment(Qt::AlignCenter);
  label->setStyleSheet(
      "QLabel { color: #ffffff; background: transparent; border: none; }");
  return label;
}

QLabel *makeCaption(const QString &text, QWidget *parent) {
  auto *label = new QLabel(text, parent);
  label->setStyleSheet(
      "QLabel { color: #ffffff; background: transparent; border: none; }");
  return label;
}
} // namespace

FeedbackRow::FeedbackRow(int index, const QString &pattern, int colors,
                         int positions, int maxPegs, QWidget *parent)
    : QWidget(parent), rowIndex(index), correctColors(colors),
      correctPositions(positions), maxPegs(maxPegs) {
  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(6, 2, 6, 2);
  layout->setSpacing(4);

  patternLabel = new QLabel(pattern, this);
  QFont monoFont(QStringLiteral("Consolas"), 10);
  monoFont.setBold(true);
  patternLabel->setFont(monoFont);
  patternLabel->setMinimumWidth(70);
  patternLabel->setStyleSheet(
      "QLabel { color: #ffffff; background: transparent; border: none; }");

  // Black pegs = correct position
  auto *positionsCaption = makeCaption(QStringLiteral("Pos"), this);
  auto *positionsMinus = makeStepButton(QStringLiteral("-"), this);
  positionsValueLabel = makeCountLabel(this);
  auto *positionsPlus = makeStepButton(QStringLiteral("+"), this);

  // White pegs = correct color
  auto *colorsCaption = makeCaption(QStringLiteral("Color"), this);
  auto *colorsMinus = makeStepButton(QStringLiteral("-"), this);
  colorsValueLabel = makeCountLabel(this);
  auto *colorsPlus = makeStepButton(QStringLiteral("+"), this);

  connect(positionsMinus, &QPushButton::clicked, this,
          [this]() { adjustPositions(-1); });
  connect(positionsPlus, &QPushButton::clicked, this,
          [this]() { adjustPositions(1); });
  connect(colorsMinus, &QPushButton::clicked, this,
          [this]() { adjustColors(-1); });
  connect(colorsPlus, &QPushButton::clicked, this,
          [this]() { adjustColors(1); });

  deleteButton = new QPushButton(QStringLiteral("✕"), this);
  deleteButton->setFixedSize(22, 22);
  deleteButton->setToolTip(QStringLiteral("Remove guess"));
  deleteButton->setCursor(Qt::PointingHandCursor);
  deleteButton->setFocusPolicy(Qt::NoFocus);
  deleteButton->setStyleSheet(
      "QPushButton {"
      "  background-color: transparent;"
      "  color: #ff8a80;"
      "  border: 1px solid #ef5350;"
      "  border-radius: 3px;"
      "  font-size: 11px;"
      "  font-weight: bold;"
      "  padding: 0;"
      "}"
      "QPushButton:hover { background-color: rgba(239,83,80,0.25); }");
  connect(deleteButton, &QPushButton::clicked, this,
          [this]() { emit deleteRequested(rowIndex); });

  layout->addWidget(patternLabel);
  layout->addStretch(1);
  layout->addWidget(positionsCaption);
  layout->addWidget(positionsMinus);
  layout->addWidget(positionsValueLabel);
  layout->addWidget(positionsPlus);
  layout->addSpacing(8);
  layout->addWidget(colorsCaption);
  layout->addWidget(colorsMinus);
  layout->addWidget(colorsValueLabel);
  layout->addWidget(colorsPlus);
  layout->addSpacing(6);
  layout->addWidget(deleteButton);

  setStyleSheet("FeedbackRow { background: transparent; border: none; }");
  refreshValueLabels();
}

void FeedbackRow::refreshValueLabels() {
  positionsValueLabel->setText(QString::number(correctPositions));
  colorsValueLabel->setText(QString::number(correctColors));
}

void FeedbackRow::adjustPositions(int delta) {
  const int newVal =
      std::clamp(correctPositions + delta, 0, maxPegs);
  if (newVal == correctPositions)
    return;
  correctPositions = newVal;
  if (correctColors > maxPegs - correctPositions)
    correctColors = maxPegs - correctPositions;
  refreshValueLabels();
  emit feedbackChanged(rowIndex);
}

void FeedbackRow::adjustColors(int delta) {
  const int newVal = std::clamp(correctColors + delta, 0, maxPegs);
  if (newVal == correctColors)
    return;
  correctColors = newVal;
  if (correctPositions > maxPegs - correctColors)
    correctPositions = maxPegs - correctColors;
  refreshValueLabels();
  emit feedbackChanged(rowIndex);
}

void FeedbackRow::updateFeedback(int colors, int positions) {
  correctColors = colors;
  correctPositions = positions;
  refreshValueLabels();
}

MastermindWidget::MastermindWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::MastermindWidget) {
  ui->setupUi(this);

  // Initialize config defaults (match web frontend)
  config.numPegs = 4;
  config.colorChars = "RGBCMY"; // Default to 6 colors represented as letters
  config.allowDuplicates = true;
  config.autoDepth = true;
  config.maxDepth = 0;
  config.maxGuesses = 10;

  // Setup feedback list container (scroll area from UI)
  feedbackListScrollArea = ui->feedbackListScrollArea;
  feedbackListContainer = new QWidget();
  feedbackListLayout = new QVBoxLayout(feedbackListContainer);
  feedbackListLayout->setAlignment(Qt::AlignTop);
  feedbackListLayout->setContentsMargins(0, 0, 0, 0);
  feedbackListLayout->setSpacing(2);
  feedbackListScrollArea->setWidget(feedbackListContainer);
  feedbackListScrollArea->setWidgetResizable(true);

  // Get result tables from UI and configure them
  allResultsTable = ui->allResultsTable;
  allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  allResultsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");

  possibleResultsTable = ui->possibleResultsTable;
  possibleResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  possibleResultsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSubmit);
  connect(ui->addOnlyBtn, &QPushButton::clicked, this,
          &MastermindWidget::onAddOnly);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &MastermindWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSolve);
  connect(ui->patternField, &QLineEdit::returnPressed, this,
          &MastermindWidget::onInputReturn);

  connect(allResultsTable, &QTableWidget::cellClicked, this,
          &MastermindWidget::onTableRowClicked);
  connect(possibleResultsTable, &QTableWidget::cellClicked, this,
          &MastermindWidget::onTableRowClicked);

  // Connect settings button
  ui->settingsBtn->setToolTip("Game Settings");
  ui->settingsBtn->setMaximumWidth(40);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSettings);

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Initialize game immediately with default configuration
  gameInitialized = true;
  initGame();
  updateConfigInfo();
}

MastermindWidget::~MastermindWidget() {
  // Base class destructor handles thread and dialog cleanup
  delete ui;
}

void MastermindWidget::newGame() { onNewGame(); }

bool MastermindWidget::showConfigDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Mastermind Settings");
  dialog.setMinimumWidth(320);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

  QFormLayout *formLayout = new QFormLayout();

  QSpinBox *numPegsSpinBox = new QSpinBox(&dialog);
  numPegsSpinBox->setMinimum(1);
  numPegsSpinBox->setMaximum(8);
  numPegsSpinBox->setValue(config.numPegs);
  formLayout->addRow("Number of Pegs:", numPegsSpinBox);

  QLineEdit *colorCharsLineEdit = new QLineEdit(&dialog);
  colorCharsLineEdit->setText(QString::fromStdString(config.colorChars));
  colorCharsLineEdit->setPlaceholderText("e.g., rgbcmyk or 012345");
  formLayout->addRow("Color Characters:", colorCharsLineEdit);

  QCheckBox *allowDuplicatesCheckBox = new QCheckBox(&dialog);
  allowDuplicatesCheckBox->setChecked(config.allowDuplicates);
  formLayout->addRow("Allow Duplicates:", allowDuplicatesCheckBox);

  QCheckBox *autoDepthCheckBox = new QCheckBox(&dialog);
  autoDepthCheckBox->setChecked(config.autoDepth);
  autoDepthCheckBox->setToolTip(
      "Dynamically choose search depth based on available time.");
  formLayout->addRow("Auto Depth (Recommended):", autoDepthCheckBox);

  QSpinBox *maxDepthSpinBox = new QSpinBox(&dialog);
  maxDepthSpinBox->setMinimum(0);
  maxDepthSpinBox->setMaximum(2);
  maxDepthSpinBox->setValue(config.maxDepth);
  maxDepthSpinBox->setEnabled(!config.autoDepth);
  maxDepthSpinBox->setToolTip(
      "0: Fastest, 1: Balanced, 2: Deep. Disabled when Auto Depth is on.");
  formLayout->addRow("Manual Search Depth:", maxDepthSpinBox);

  connect(autoDepthCheckBox, &QCheckBox::toggled, maxDepthSpinBox,
          [maxDepthSpinBox](bool checked) { maxDepthSpinBox->setEnabled(!checked); });

  QSpinBox *maxGuessesSpinBox = new QSpinBox(&dialog);
  maxGuessesSpinBox->setRange(1, 100);
  maxGuessesSpinBox->setValue(static_cast<int>(config.maxGuesses));
  formLayout->addRow("Maximum Guesses Allowed:", maxGuessesSpinBox);

  mainLayout->addLayout(formLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  mainLayout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    uint8_t oldNumPegs = config.numPegs;
    std::string oldColorChars = config.colorChars;

    config.numPegs = numPegsSpinBox->value();
    config.colorChars = colorCharsLineEdit->text().toStdString();
    if (config.colorChars.empty()) {
      config.colorChars = "RGBCMY"; // Default if empty
    }
    config.allowDuplicates = allowDuplicatesCheckBox->isChecked();
    config.autoDepth = autoDepthCheckBox->isChecked();
    config.maxDepth = maxDepthSpinBox->value();
    config.maxGuesses = static_cast<uint32_t>(maxGuessesSpinBox->value());

    // Clear feedback history if pegs or colors changed
    if (oldNumPegs != config.numPegs || oldColorChars != config.colorChars) {
      config.feedbackHistory.clear();
      ui->patternField->clear();
      initGame();
    }

    return true;
  }

  return false;
}

void MastermindWidget::onSubmit() {
  if (submitPattern()) {
    solveMastermind();
  }
}

void MastermindWidget::onAddOnly() { submitPattern(); }

void MastermindWidget::onInputReturn() {
  // Match frontend: Enter adds without solving; empty input + history solves
  if (!ui->patternField->text().trimmed().isEmpty()) {
    onAddOnly();
  } else if (!config.feedbackHistory.empty()) {
    solveMastermind();
  }
}

bool MastermindWidget::submitPattern() {
  QString patternInput = ui->patternField->text().trimmed();
  if (patternInput.isEmpty()) {
    QMessageBox::information(this, "Input Required", "Please enter a pattern!");
    return false;
  }

  try {
    // Parse pattern string (no spaces between characters)
    std::string patternStr = patternInput.toStdString();
    Mastermind::Pattern pattern;
    pattern.numPegs = 0;

    for (char c : patternStr) {
      if (pattern.numPegs >= Mastermind::MAX_PEGS) {
        break;
      }
      int color = config.charToColor(c);
      if (color < 0) {
        throw std::invalid_argument(
            "Invalid color character. Available colors: " + config.colorChars);
      }
      pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
      pattern.numPegs++;
    }

    if (pattern.numPegs != config.numPegs) {
      throw std::invalid_argument("Wrong number of pegs");
    }

    // Default feedback to 0, 0 (user will edit it afterward)
    Mastermind::Feedback feedback;
    feedback.guess = pattern;
    feedback.correctPosition = 0;
    feedback.correctColor = 0;

    config.feedbackHistory.push_back(feedback);

    // Rebuild the feedback list to show the new entry
    rebuildFeedbackList();

    ui->patternField->clear();
    return true;
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Invalid Input",
                         QString("Error: %1\n\nFormat: consecutive color "
                                 "characters (e.g., RGBY)")
                             .arg(e.what()));
    return false;
  }
}

void MastermindWidget::onNewGame() {
  initGame();
  updateConfigInfo();
}

void MastermindWidget::onSolve() { solveMastermind(); }

void MastermindWidget::onSettings() {
  showConfigDialog();
  updateConfigInfo();
}

void MastermindWidget::onTableRowClicked(int row, int column) {
  Q_UNUSED(column);

  QTableWidget *table = qobject_cast<QTableWidget *>(sender());
  if (!table) {
    return;
  }

  // Extract pattern from the Pattern column (column 0)
  QTableWidgetItem *patternItem = table->item(row, 0);
  if (!patternItem) {
    return;
  }

  // Parse pattern string (no spaces between characters)
  std::string patternStr = patternItem->text().toStdString();
  Mastermind::Pattern pattern;
  pattern.numPegs = 0;

  for (char c : patternStr) {
    if (pattern.numPegs >= Mastermind::MAX_PEGS) {
      break;
    }
    int color = config.charToColor(c);
    if (color >= 0) {
      pattern.colors[pattern.numPegs] = static_cast<uint8_t>(color);
      pattern.numPegs++;
    }
  }

  // Default feedback to 0, 0 (user will edit it afterward)
  Mastermind::Feedback feedback;
  feedback.guess = pattern;
  feedback.correctPosition = 0;
  feedback.correctColor = 0;

  config.feedbackHistory.push_back(feedback);

  // Rebuild the feedback list to show the new entry
  rebuildFeedbackList();

  // Set the pattern in the input field
  // ui->patternField->setText(pattern);
  // ui->patternField->setFocus();
}

void MastermindWidget::initGame() {
  // Config is already set by showConfigDialog()
  config.feedbackHistory.clear();

  // Clear feedback list
  QLayoutItem *item;
  while ((item = feedbackListLayout->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  ui->patternField->clear();

  // Clear result tables
  allResultsTable->setRowCount(0);
  possibleResultsTable->setRowCount(0);

  // Reset tab titles to match frontend
  ui->resultsTabWidget->setTabText(0, "Suggested Guesses");
  ui->resultsTabWidget->setTabText(1, "Possible Patterns");

  // Update placeholder text with current configuration
  QString examplePattern;
  for (unsigned int i = 0; i < config.numPegs && i < config.colorChars.length();
       ++i) {
    examplePattern += config.colorChars[i];
  }
  ui->patternField->setPlaceholderText(
      QString("Enter pattern (e.g., %1)...").arg(examplePattern));
}

void MastermindWidget::populateResults(int maxRows) {
  // Match frontend columns: Pattern | Probability | ENT | WNT
  auto setupHeaders = [](QTableWidget *table) {
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("Pattern"), QStringLiteral("Probability"),
         QStringLiteral("ENT"), QStringLiteral("WNT")});
  };
  setupHeaders(allResultsTable);
  setupHeaders(possibleResultsTable);

  const auto &all = lastAllResults;
  int allRows = std::min(maxRows, static_cast<int>(all.size()));
  allResultsTable->setRowCount(allRows);
  for (int i = 0; i < allRows; ++i) {
    const auto &guess = all[i];

    QString patternStr = QString::fromStdString(guess.pattern.toString(config));
    QTableWidgetItem *patternItem = new QTableWidgetItem(patternStr);
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    patternItem->setFont(monoFont);
    patternItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 0, patternItem);

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

  // Possible Patterns (only entries with probability > 0)
  possibleResultsTable->setRowCount(0);
  lastProbableResults.clear();
  for (int i = 0; i < static_cast<int>(all.size()); ++i) {
    const auto &guess = all[i];
    if (guess.probability <= 0.0)
      continue;
    int row = possibleResultsTable->rowCount();
    possibleResultsTable->insertRow(row);

    QString patternStr = QString::fromStdString(guess.pattern.toString(config));
    QTableWidgetItem *patternItem = new QTableWidgetItem(patternStr);
    QFont monoFont2("Consolas", 10);
    monoFont2.setBold(true);
    patternItem->setFont(monoFont2);
    patternItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 0, patternItem);

    QTableWidgetItem *probItem =
        new QTableWidgetItem(formatProbabilityPercent(guess.probability));
    probItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 1, probItem);

    QTableWidgetItem *entItem =
        new QTableWidgetItem(formatRoundedNum(guess.ent));
    entItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 2, entItem);

    QTableWidgetItem *wntItem =
        new QTableWidgetItem(formatRoundedNum(guess.wnt));
    wntItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 3, wntItem);

    applyProbabilityRowColors(possibleResultsTable, row, 4, guess.probability);

    lastProbableResults.push_back(guess);
    if (static_cast<int>(possibleResultsTable->rowCount()) >= maxRows)
      break;
  }
}

void MastermindWidget::solveMastermind() {
  // Clean up any existing thread and dialog using base class methods
  cleanupSolverThread();
  cleanupProgressDialog();

  // Reset cancellation flag
  cancellationRequested.store(false, std::memory_order_release);

  // Create progress dialog using base class method
  createProgressDialog("Solving Mastermind...", 0, 0);

  // Disable UI elements that could interfere with solving
  ui->patternField->setEnabled(false);
  ui->submitBtn->setEnabled(false);
  ui->addOnlyBtn->setEnabled(false);
  ui->solveBtn->setEnabled(false);
  ui->newGameBtn->setEnabled(false);
  ui->settingsBtn->setEnabled(false);
  ui->resultsTabWidget->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(config, &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &MastermindWidget::onSolverFinished);
  solverThread->start();
}

void MastermindWidget::onSolverFinished() {
  // Re-enable UI elements
  ui->patternField->setEnabled(true);
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
    Mastermind::Result result =
        static_cast<SolverThread *>(solverThread)->getResult();

    // If solver returned no suggestions, inform the user
    if (result.sortedGuesses.empty()) {
      QMessageBox::information(this, "No Results", "No suggestions available");
    } else {
      // Cache results and populate both tables via single call
      lastAllResults = result.sortedGuesses;
      populateResults(1000);

      ui->resultsTabWidget->setTabText(
          0, QString("Suggested Guesses (%1)").arg(lastAllResults.size()));
      ui->resultsTabWidget->setTabText(
          1,
          QString("Possible Patterns (%1)").arg(result.totalPossiblePatterns));
    }

  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Solver Error",
                          QString("Error running solver: %1").arg(e.what()));
  }

  // Clean up (non-blocking)
  cleanupProgressDialog();
  cleanupSolverThread();
}

void MastermindWidget::setUIEnabled(bool enabled) {
  // Show/hide all UI elements except title, config info, and new game button
  ui->patternField->setVisible(enabled);
  ui->submitBtn->setVisible(enabled);
  ui->addOnlyBtn->setVisible(enabled);
  ui->feedbackListLabel->setVisible(enabled);
  feedbackListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
  ui->solveBtn->setVisible(enabled);
}

void MastermindWidget::updateConfigInfo() {
  QString duplicatesStr = config.allowDuplicates ? "Yes" : "No";
  QString depthStr = config.autoDepth ? QStringLiteral("auto")
                                      : QString::number(config.maxDepth);
  QString info =
      QString("<span style='color:#666; font-size:11pt;'>%1 pegs | "
              "Colors: %2 | Duplicates: %3 | Depth: %4 | Max guesses: %5</span>")
          .arg(config.numPegs)
          .arg(QString::fromStdString(config.colorChars))
          .arg(duplicatesStr)
          .arg(depthStr)
          .arg(config.maxGuesses);
  configInfoLabel->setText(info);
}

void MastermindWidget::rebuildFeedbackList() {
  // Clear existing widgets
  QLayoutItem *item;
  while ((item = feedbackListLayout->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  // Rebuild from feedbackHistory
  for (size_t i = 0; i < config.feedbackHistory.size(); ++i) {
    const auto &fb = config.feedbackHistory[i];
    QString displayPattern = QString::fromStdString(fb.guess.toString(config));
    FeedbackRow *row = new FeedbackRow(i, displayPattern, fb.correctColor,
                                       fb.correctPosition, config.numPegs,
                                       feedbackListContainer);
    connect(row, &FeedbackRow::deleteRequested, this,
            &MastermindWidget::onDeleteFeedback);
    connect(row, &FeedbackRow::feedbackChanged, this,
            &MastermindWidget::onFeedbackChanged);
    feedbackListLayout->addWidget(row);
  }
}

void MastermindWidget::onDeleteFeedback(int index) {
  if (index >= 0 && index < static_cast<int>(config.feedbackHistory.size())) {
    config.feedbackHistory.erase(config.feedbackHistory.begin() + index);
    rebuildFeedbackList();
  }
}

void MastermindWidget::onFeedbackChanged(int index) {
  if (index < 0 || index >= static_cast<int>(config.feedbackHistory.size())) {
    return;
  }

  // Get the updated values from the FeedbackRow
  FeedbackRow *row =
      qobject_cast<FeedbackRow *>(feedbackListLayout->itemAt(index)->widget());
  if (row) {
    config.feedbackHistory[index].correctColor = row->getCorrectColors();
    config.feedbackHistory[index].correctPosition = row->getCorrectPositions();
  }
}

#endif // WITH_GUI
