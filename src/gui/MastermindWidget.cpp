#ifdef WITH_GUI

#include "gui/MastermindWidget.hpp"
#include "ui_MastermindWidget.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QVBoxLayout>
#include <sstream>

// FeedbackRow implementation
FeedbackRow::FeedbackRow(int index, const QString &pattern, int colors,
                         int positions, int maxPegs, QWidget *parent)
    : QWidget(parent), rowIndex(index), correctColors(colors),
      correctPositions(positions), maxPegs(maxPegs) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setContentsMargins(5, 2, 5, 2);

  // Pattern label with monospace font
  patternLabel = new QLabel(pattern, this);
  QFont monoFont("Consolas", 10);
  monoFont.setBold(true);
  patternLabel->setFont(monoFont);
  patternLabel->setMinimumWidth(100);

  // Label for "Colors:"
  QLabel *colorsLabel = new QLabel("Colors:", this);

  // Colors spinbox
  colorsSpinBox = new QSpinBox(this);
  colorsSpinBox->setMinimum(0);
  colorsSpinBox->setMaximum(maxPegs);
  colorsSpinBox->setValue(colors);
  colorsSpinBox->setMaximumWidth(60);
  connect(colorsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int value) {
            correctColors = value;
            if (correctPositions > this->maxPegs - correctColors) {
              correctPositions = this->maxPegs - correctColors;
              positionsSpinBox->setValue(correctPositions);
            }
            emit feedbackChanged(rowIndex);
          });

  // Label for "Positions:"
  QLabel *positionsLabel = new QLabel("Positions:", this);

  // Positions spinbox
  positionsSpinBox = new QSpinBox(this);
  positionsSpinBox->setMinimum(0);
  positionsSpinBox->setMaximum(maxPegs);
  positionsSpinBox->setValue(positions);
  positionsSpinBox->setMaximumWidth(60);
  connect(positionsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          [this](int value) {
            correctPositions = value;
            if (correctColors > this->maxPegs - correctPositions) {
              correctColors = this->maxPegs - correctPositions;
              colorsSpinBox->setValue(correctColors);
            }
            emit feedbackChanged(rowIndex);
          });

  // Delete button
  deleteButton = new QPushButton("Delete", this);
  deleteButton->setMaximumWidth(60);
  connect(deleteButton, &QPushButton::clicked, this,
          [this]() { emit deleteRequested(rowIndex); });

  layout->addWidget(patternLabel);
  layout->addWidget(colorsLabel);
  layout->addWidget(colorsSpinBox);
  layout->addWidget(positionsLabel);
  layout->addWidget(positionsSpinBox);
  layout->addStretch();
  layout->addWidget(deleteButton);

  setLayout(layout);
}

void FeedbackRow::updateFeedback(int colors, int positions) {
  correctColors = colors;
  correctPositions = positions;
}

MastermindWidget::MastermindWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::MastermindWidget) {
  ui->setupUi(this);

  // Initialize config defaults
  config.numPegs = 4;
  config.colorChars = "RGBCMY"; // Default to 6 colors represented as letters
  config.allowDuplicates = true;
  config.maxDepth = 1;

  // Setup feedback list container (scroll area from UI)
  feedbackListScrollArea = ui->feedbackListScrollArea;
  feedbackListContainer = new QWidget();
  feedbackListLayout = new QVBoxLayout(feedbackListContainer);
  feedbackListLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
  feedbackListScrollArea->setWidget(feedbackListContainer);

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
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &MastermindWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSolve);
  connect(ui->patternField, &QLineEdit::returnPressed, this,
          &MastermindWidget::onSubmit);

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
  dialog.setWindowTitle("Game Configuration");

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

  QSpinBox *maxDepthSpinBox = new QSpinBox(&dialog);
  maxDepthSpinBox->setMinimum(0);
  maxDepthSpinBox->setMaximum(2);
  maxDepthSpinBox->setValue(config.maxDepth);
  formLayout->addRow("Search Depth:", maxDepthSpinBox);

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
    config.maxDepth = maxDepthSpinBox->value();

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
  QString patternInput = ui->patternField->text().trimmed();
  if (patternInput.isEmpty()) {
    QMessageBox::information(this, "Input Required", "Please enter a pattern!");
    return;
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
  } catch (const std::exception &e) {
    QMessageBox::warning(this, "Invalid Input",
                         QString("Error: %1\n\nFormat: 1 2 3 4\n(space-"
                                 "separated color numbers)")
                             .arg(e.what()));
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

  // Extract pattern from the Pattern column (column 1)
  QTableWidgetItem *patternItem = table->item(row, 1);
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

  // Reset tab titles
  ui->resultsTabWidget->setTabText(0, "All Suggestions");
  ui->resultsTabWidget->setTabText(1, "Possible Answers");

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
  // Populate All Results
  const auto &all = lastAllResults;
  allResultsTable->setRowCount(0);
  int limit = std::min(maxRows, static_cast<int>(all.size()));
  for (int i = 0; i < limit; ++i) {
    const auto &guess = all[i];
    if (allResultsTable->rowCount() >= maxRows)
      break;
    int row = allResultsTable->rowCount();
    allResultsTable->insertRow(row);

    int actualRank = i + 1;
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(actualRank));
    rankItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(row, 0, rankItem);

    QString patternStr = QString::fromStdString(guess.pattern.toString(config));
    QTableWidgetItem *patternItem = new QTableWidgetItem(patternStr);
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    patternItem->setFont(monoFont);
    patternItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(row, 1, patternItem);

    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(row, 2, entItem);

    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(row, 3, probItem);

    // Color coding
    if (guess.probability >= 1.0) {
      QColor bgColor(144, 238, 144);
      for (int col = 0; col < 4; ++col) {
        if (allResultsTable->item(row, col)) {
          allResultsTable->item(row, col)->setBackground(bgColor);
          allResultsTable->item(row, col)->setForeground(Qt::black);
        }
      }
    } else if (guess.probability > 0.0) {
      QColor bgColor(255, 255, 153);
      for (int col = 0; col < 4; ++col) {
        if (allResultsTable->item(row, col)) {
          allResultsTable->item(row, col)->setBackground(bgColor);
          allResultsTable->item(row, col)->setForeground(Qt::black);
        }
      }
    }
  }

  // Populate Possible Results (only entries with probability > 0), preserving
  // original rank
  possibleResultsTable->setRowCount(0);
  lastProbableResults.clear();
  for (int i = 0; i < static_cast<int>(all.size()); ++i) {
    const auto &guess = all[i];
    if (guess.probability <= 0.0)
      continue;
    int row = possibleResultsTable->rowCount();
    possibleResultsTable->insertRow(row);

    int actualRank = i + 1;
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(actualRank));
    rankItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 0, rankItem);

    QString patternStr = QString::fromStdString(guess.pattern.toString(config));
    QTableWidgetItem *patternItem = new QTableWidgetItem(patternStr);
    QFont monoFont2("Consolas", 10);
    monoFont2.setBold(true);
    patternItem->setFont(monoFont2);
    patternItem->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 1, patternItem);

    QTableWidgetItem *entItem2 =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem2->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 2, entItem2);

    QTableWidgetItem *probItem2 = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem2->setTextAlignment(Qt::AlignCenter);
    possibleResultsTable->setItem(row, 3, probItem2);

    // Color coding
    if (guess.probability >= 1.0) {
      QColor bgColor(144, 238, 144);
      for (int col = 0; col < 4; ++col) {
        if (possibleResultsTable->item(row, col)) {
          possibleResultsTable->item(row, col)->setBackground(bgColor);
          possibleResultsTable->item(row, col)->setForeground(Qt::black);
        }
      }
    } else {
      QColor bgColor(255, 255, 153);
      for (int col = 0; col < 4; ++col) {
        if (possibleResultsTable->item(row, col)) {
          possibleResultsTable->item(row, col)->setBackground(bgColor);
          possibleResultsTable->item(row, col)->setForeground(Qt::black);
        }
      }
    }

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
          0, QString("All Suggestions (%1)").arg(lastAllResults.size()));
      ui->resultsTabWidget->setTabText(
          1,
          QString("Possible Answers (%1)").arg(result.totalPossiblePatterns));
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
  ui->feedbackListLabel->setVisible(enabled);
  feedbackListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
  ui->solveBtn->setVisible(enabled);
}

void MastermindWidget::updateConfigInfo() {
  QString info = QString("<span style='color:#666; font-size:11pt;'>%1 pegs | "
                         "Colors: %2 | Search Depth: %3</span>")
                     .arg(config.numPegs)
                     .arg(QString::fromStdString(config.colorChars))
                     .arg(config.maxDepth);
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
    FeedbackRow *row =
        new FeedbackRow(i, displayPattern, fb.correctColor, fb.correctPosition,
                        config.numPegs, this);
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
