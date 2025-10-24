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
  config.colorChars = "012345"; // Default to 6 colors represented as digits
  config.allowDuplicates = true;
  config.maxDepth = 1;

  // Create scroll area for feedback list
  feedbackListScrollArea = new QScrollArea(this);
  feedbackListScrollArea->setWidgetResizable(true);
  feedbackListScrollArea->setMaximumHeight(200);

  feedbackListContainer = new QWidget(feedbackListScrollArea);
  feedbackListLayout = new QVBoxLayout(feedbackListContainer);
  feedbackListLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
  feedbackListContainer->setLayout(feedbackListLayout);
  feedbackListScrollArea->setWidget(feedbackListContainer);

  // Add scroll area to the main layout after feedback list label and before
  // Solve button
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
  if (mainLayout) {
    int solveBtnIndex = mainLayout->indexOf(ui->solveBtn);
    if (solveBtnIndex >= 0) {
      mainLayout->insertWidget(solveBtnIndex, feedbackListScrollArea);
    }
  }

  // Setup result tables
  QStringList headers = {"Rank", "Pattern", "ENT", "Probability"};

  ui->allResultsTable->setColumnCount(4);
  ui->allResultsTable->setHorizontalHeaderLabels(headers);
  ui->allResultsTable->horizontalHeader()->setStretchLastSection(true);
  ui->allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  ui->possibleResultsTable->setColumnCount(4);
  ui->possibleResultsTable->setHorizontalHeaderLabels(headers);
  ui->possibleResultsTable->horizontalHeader()->setStretchLastSection(true);
  ui->possibleResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &MastermindWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &MastermindWidget::onSolve);
  connect(ui->patternField, &QLineEdit::returnPressed, this,
          &MastermindWidget::onSubmit);

  connect(ui->allResultsTable, &QTableWidget::cellClicked, this,
          &MastermindWidget::onTableRowClicked);
  connect(ui->possibleResultsTable, &QTableWidget::cellClicked, this,
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
      config.colorChars = "012345"; // Default if empty
    }
    config.maxDepth = maxDepthSpinBox->value();

    // Clear feedback history if pegs or colors changed
    if (oldNumPegs != config.numPegs || oldColorChars != config.colorChars) {
      feedbackHistory.clear();
      ui->patternField->clear();
    }

    // Regenerate patterns and reinitialize game with new configuration
    initGame();
    updateConfigInfo();
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

    feedbackHistory.push_back(feedback);

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

  // TODO: figure out why it doesn't populate correctly
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

  feedbackHistory.push_back(feedback);

  // Rebuild the feedback list to show the new entry
  rebuildFeedbackList();

  // Set the pattern in the input field
  // ui->patternField->setText(pattern);
  // ui->patternField->setFocus();
}

void MastermindWidget::initGame() {
  // Config is already set by showConfigDialog()
  feedbackHistory.clear();

  // Clear feedback list
  QLayoutItem *item;
  while ((item = feedbackListLayout->takeAt(0)) != nullptr) {
    delete item->widget();
    delete item;
  }

  ui->patternField->clear();

  // Clear result tables
  ui->allResultsTable->setRowCount(0);
  ui->possibleResultsTable->setRowCount(0);

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

  // Generate all possible patterns with new configuration
  allPatterns = Mastermind::generateAllPatterns(config);
}

void MastermindWidget::populateResultTable(
    QTableWidget *table, const std::vector<Mastermind::PatternGuess> &guesses,
    bool filterPossible) {
  table->setRowCount(0);

  int displayedRank = 0;
  for (size_t i = 0; i < guesses.size(); ++i) {
    const auto &guess = guesses[i];

    // Skip if filtering for possible and this has 0 probability
    if (filterPossible && guess.probability <= 0.0) {
      continue;
    }

    displayedRank++;
    int row = table->rowCount();
    table->insertRow(row);

    // Rank - use actual position in full list (i+1), not displayedRank
    int actualRank = static_cast<int>(i) + 1;
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(actualRank));
    rankItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 0, rankItem);

    // Pattern (uppercase monospace)
    QString patternStr =
        QString::fromStdString(guess.pattern.toString(config)).toUpper();
    QTableWidgetItem *patternItem = new QTableWidgetItem(patternStr);
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    patternItem->setFont(monoFont);
    patternItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 1, patternItem);

    // ENT
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 2, entItem);

    // Probability
    QString probStr = QString::number(guess.probability * 100.0, 'f', 2) + "%";
    QTableWidgetItem *probItem = new QTableWidgetItem(probStr);
    probItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(row, 3, probItem);

    // Color coding
    if (guess.probability >= 1.0) {
      // Green for 100% probability
      QColor bgColor(144, 238, 144);
      for (int col = 0; col < 4; ++col) {
        if (table->item(row, col)) {
          table->item(row, col)->setBackground(bgColor);
          table->item(row, col)->setForeground(Qt::black);
        }
      }
    } else if (guess.probability > 0.0) {
      // Yellow for 0% < probability < 100%
      QColor bgColor(255, 255, 153);
      for (int col = 0; col < 4; ++col) {
        if (table->item(row, col)) {
          table->item(row, col)->setBackground(bgColor);
          table->item(row, col)->setForeground(Qt::black);
        }
      }
    }
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

  // Disable UI during solve
  ui->solveBtn->setEnabled(false);
  ui->submitBtn->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(allPatterns, feedbackHistory, config,
                                  &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &MastermindWidget::onSolverFinished);
  solverThread->start();
}

void MastermindWidget::onSolverFinished() {
  if (!solverThread) {
    return;
  }

  // Re-enable UI immediately for responsiveness
  ui->solveBtn->setEnabled(true);
  ui->submitBtn->setEnabled(true);

  // Close progress dialog immediately
  if (progressDialog) {
    progressDialog->close();
  }

  // Check if thread was interrupted (cancelled)
  if (solverThread->isInterruptionRequested()) {
    cleanupProgressDialog();
    cleanupSolverThread();
    return;
  }

  try {
    Mastermind::Result result =
        static_cast<SolverThread *>(solverThread)->getResult();

    // Populate both tables
    populateResultTable(ui->allResultsTable, result.sortedGuesses, false);
    populateResultTable(ui->possibleResultsTable, result.sortedGuesses, true);

    // Update tab titles with counts
    ui->resultsTabWidget->setTabText(
        0, QString("All Suggestions (%1)").arg(result.sortedGuesses.size()));
    ui->resultsTabWidget->setTabText(
        1, QString("Possible Answers (%1)").arg(result.totalPossiblePatterns));

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
                         "Colors: %2 | Depth: %3</span>")
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
  for (size_t i = 0; i < feedbackHistory.size(); ++i) {
    const auto &fb = feedbackHistory[i];
    QString displayPattern =
        QString::fromStdString(fb.guess.toString(config)).toUpper();
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
  if (index >= 0 && index < static_cast<int>(feedbackHistory.size())) {
    feedbackHistory.erase(feedbackHistory.begin() + index);
    rebuildFeedbackList();
  }
}

void MastermindWidget::onFeedbackChanged(int index) {
  if (index < 0 || index >= static_cast<int>(feedbackHistory.size())) {
    return;
  }

  // Get the updated values from the FeedbackRow
  FeedbackRow *row =
      qobject_cast<FeedbackRow *>(feedbackListLayout->itemAt(index)->widget());
  if (row) {
    feedbackHistory[index].correctColor = row->getCorrectColors();
    feedbackHistory[index].correctPosition = row->getCorrectPositions();
  }
}

#endif // WITH_GUI
