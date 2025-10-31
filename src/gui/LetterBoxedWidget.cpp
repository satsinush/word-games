#ifdef WITH_GUI

#include "gui/LetterBoxedWidget.hpp"
#include "ui_LetterBoxedWidget.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QVBoxLayout>
#include <algorithm>
#include <iostream>
#include <random>
#include <set>

// ============ LetterBoxDisplay Implementation ============

LetterBoxDisplay::LetterBoxDisplay(QWidget *parent) : QWidget(parent) {
  setFixedSize(380, 380);
  currentLetters.fill('*');
}

void LetterBoxDisplay::setLetters(const std::array<char, 12> &letters) {
  currentLetters = letters;
  update();
}

void LetterBoxDisplay::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  int boxSize = 240;
  int centerX = width() / 2;
  int centerY = height() / 2;
  int halfBox = boxSize / 2;

  // Draw the central square box
  painter.setPen(QPen(Qt::black, 3));
  painter.setBrush(Qt::NoBrush);
  QRect box(centerX - halfBox, centerY - halfBox, boxSize, boxSize);
  painter.drawRect(box);

  // Font for letters
  QFont letterFont;
  letterFont.setPointSize(24);
  letterFont.setBold(true);
  painter.setFont(letterFont);
  painter.setPen(Qt::black);

  // Positions for letters and circles
  // Top: indices 0, 1, 2
  // Right: indices 3, 4, 5
  // Bottom: indices 6, 7, 8 (reversed)
  // Left: indices 9, 10, 11 (reversed)

  int spacing = boxSize / 4;
  int circleRadius = 8;
  int letterOffset = 40;
  int textWidth = 40;
  int textHeight = 40;

  // Top side (0, 1, 2)
  for (int i = 0; i < 3; ++i) {
    int x = centerX - halfBox + spacing * (i + 1);
    int y = centerY - halfBox;

    // Draw circle on box edge
    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawEllipse(QPoint(x, y), circleRadius, circleRadius);

    // Draw letter above
    if (currentLetters[i] != '*' && currentLetters[i] != '\0') {
      painter.setPen(Qt::white);
      QRect textRect(x - textWidth / 2, y - letterOffset - textHeight / 2,
                     textWidth, textHeight);
      painter.drawText(textRect, Qt::AlignCenter,
                       QString(QChar(currentLetters[i]).toUpper()));
    }
  }

  // Right side (3, 4, 5)
  for (int i = 0; i < 3; ++i) {
    int x = centerX + halfBox;
    int y = centerY - halfBox + spacing * (i + 1);

    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawEllipse(QPoint(x, y), circleRadius, circleRadius);

    if (currentLetters[3 + i] != '*' && currentLetters[3 + i] != '\0') {
      painter.setPen(Qt::white);
      QRect textRect(x + letterOffset - textWidth / 2, y - textHeight / 2,
                     textWidth, textHeight);
      painter.drawText(textRect, Qt::AlignCenter,
                       QString(QChar(currentLetters[3 + i]).toUpper()));
    }
  }

  // Bottom side (6, 7, 8 - drawn right to left)
  for (int i = 0; i < 3; ++i) {
    int x = centerX + halfBox - spacing * (i + 1);
    int y = centerY + halfBox;

    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawEllipse(QPoint(x, y), circleRadius, circleRadius);

    if (currentLetters[6 + i] != '*' && currentLetters[6 + i] != '\0') {
      painter.setPen(Qt::white);
      QRect textRect(x - textWidth / 2, y + letterOffset - textHeight / 2,
                     textWidth, textHeight);
      painter.drawText(textRect, Qt::AlignCenter,
                       QString(QChar(currentLetters[6 + i]).toUpper()));
    }
  }

  // Left side (9, 10, 11 - drawn bottom to top)
  for (int i = 0; i < 3; ++i) {
    int x = centerX - halfBox;
    int y = centerY + halfBox - spacing * (i + 1);

    painter.setBrush(Qt::white);
    painter.setPen(QPen(Qt::black, 2));
    painter.drawEllipse(QPoint(x, y), circleRadius, circleRadius);

    if (currentLetters[9 + i] != '*' && currentLetters[9 + i] != '\0') {
      painter.setPen(Qt::white);
      QRect textRect(x - letterOffset - textWidth / 2, y - textHeight / 2,
                     textWidth, textHeight);
      painter.drawText(textRect, Qt::AlignCenter,
                       QString(QChar(currentLetters[9 + i]).toUpper()));
    }
  }
}

// ============ LetterBoxedWidget Implementation ============

LetterBoxedWidget::LetterBoxedWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::LetterBoxedWidget), currentPreset(1) {
  ui->setupUi(this);

  // Limit input to 12 characters
  ui->inputField->setMaxLength(12);

  // Initialize config
  config.allLetters.fill('*');

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Create results table
  resultsTable = new QTableWidget(this);
  resultsTable->setColumnCount(2);
  resultsTable->setHorizontalHeaderLabels({"Solution", "Words"});
  resultsTable->horizontalHeader()->setStretchLastSection(true);
  resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  resultsTable->setSelectionMode(QAbstractItemView::NoSelection);
  resultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  resultsTable->setAlternatingRowColors(true);

  // Replace output area with results table
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
  if (mainLayout) {
    int outputIndex = mainLayout->indexOf(ui->outputArea);
    if (outputIndex >= 0) {
      mainLayout->removeWidget(ui->outputArea);
      ui->outputArea->setVisible(false);
      mainLayout->insertWidget(outputIndex, resultsTable);
    }
  }

  // Connect signals
  // Enter in the input field should automatically solve
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &LetterBoxedWidget::onInputSubmit);
  connect(ui->inputField, &QLineEdit::textChanged, this,
          &LetterBoxedWidget::onInputChanged);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onSolve);

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Connect settings button
  ui->settingsBtn->setToolTip("Solver Settings");
  ui->settingsBtn->setMaximumWidth(40);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &LetterBoxedWidget::onSettings);

  // Set initial state
  gameInitialized = true;

  // Default to preset 2 (Fast)
  config.maxDepth = 2;
  config.minWordLength = 4;
  config.minUniqueLetters = 3;
  config.pruneRedundantPaths = true;
  config.pruneDominatedClasses = true;

  // Create the letter box visualization
  createLetterBox();

  updateConfigInfo();
}

LetterBoxedWidget::~LetterBoxedWidget() {
  // Base class destructor handles thread and dialog cleanup
  delete ui;
}

void LetterBoxedWidget::newGame() { onNewGame(); }

bool LetterBoxedWidget::showConfigDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Solver Configuration");
  dialog.setMinimumWidth(300);

  QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);

  QLabel *infoLabel = new QLabel("Select solver preset:", &dialog);
  mainLayout->addWidget(infoLabel);

  // Preset radio buttons
  QRadioButton *preset1 =
      new QRadioButton("Default: Find ALL solutions up to 2 words", &dialog);
  QRadioButton *preset2 = new QRadioButton(
      "Fast: Find most solutions up to 2 words quickly (recommended)", &dialog);
  QRadioButton *preset3 = new QRadioButton(
      "Thorough: Find ALL solutions up to 3 words (slow)", &dialog);
  QRadioButton *preset0 = new QRadioButton("Custom configuration", &dialog);

  // Use saved preset value
  QButtonGroup *presetGroup = new QButtonGroup(&dialog);
  presetGroup->addButton(preset1, 1);
  presetGroup->addButton(preset2, 2);
  presetGroup->addButton(preset3, 3);
  presetGroup->addButton(preset0, 0);

  if (currentPreset == 1)
    preset1->setChecked(true);
  else if (currentPreset == 2)
    preset2->setChecked(true);
  else if (currentPreset == 3)
    preset3->setChecked(true);
  else
    preset0->setChecked(true);

  mainLayout->addWidget(preset1);
  mainLayout->addWidget(preset2);
  mainLayout->addWidget(preset3);
  mainLayout->addWidget(preset0);

  // Configuration display/edit widgets (always visible)
  QWidget *configWidget = new QWidget(&dialog);
  QFormLayout *configLayout = new QFormLayout(configWidget);

  QSpinBox *maxDepthSpin = new QSpinBox(&dialog);
  maxDepthSpin->setRange(1, 4);
  maxDepthSpin->setValue(config.maxDepth);
  configLayout->addRow("Max words per solution:", maxDepthSpin);

  QSpinBox *minWordLengthSpin = new QSpinBox(&dialog);
  minWordLengthSpin->setRange(1, 12);
  minWordLengthSpin->setValue(config.minWordLength);
  configLayout->addRow("Min word length:", minWordLengthSpin);

  QSpinBox *minUniqueLettersSpin = new QSpinBox(&dialog);
  minUniqueLettersSpin->setRange(1, 12);
  minUniqueLettersSpin->setValue(config.minUniqueLetters);
  configLayout->addRow("Min unique letters per word:", minUniqueLettersSpin);

  QCheckBox *prunePathsCheck = new QCheckBox("Prune redundant paths", &dialog);
  prunePathsCheck->setChecked(config.pruneRedundantPaths);
  configLayout->addRow(prunePathsCheck);

  QCheckBox *pruneClassesCheck =
      new QCheckBox("Prune dominated classes", &dialog);
  pruneClassesCheck->setChecked(config.pruneDominatedClasses);
  configLayout->addRow(pruneClassesCheck);

  mainLayout->addWidget(configWidget);

  // Function to update config display and enable/disable based on preset
  auto updateConfigDisplay = [&](int presetId) {
    bool isCustom = (presetId == 0);
    maxDepthSpin->setEnabled(isCustom);
    minWordLengthSpin->setEnabled(isCustom);
    minUniqueLettersSpin->setEnabled(isCustom);
    prunePathsCheck->setEnabled(isCustom);
    pruneClassesCheck->setEnabled(isCustom);

    if (presetId == 1) {
      // Default
      maxDepthSpin->setValue(2);
      minWordLengthSpin->setValue(3);
      minUniqueLettersSpin->setValue(2);
      prunePathsCheck->setChecked(true);
      pruneClassesCheck->setChecked(false);
    } else if (presetId == 2) {
      // Fast
      maxDepthSpin->setValue(2);
      minWordLengthSpin->setValue(4);
      minUniqueLettersSpin->setValue(3);
      prunePathsCheck->setChecked(true);
      pruneClassesCheck->setChecked(true);
    } else if (presetId == 3) {
      // Thorough
      maxDepthSpin->setValue(3);
      minWordLengthSpin->setValue(3);
      minUniqueLettersSpin->setValue(2);
      prunePathsCheck->setChecked(true);
      pruneClassesCheck->setChecked(false);
    }
  };

  // Update display when preset changes
  connect(presetGroup, QOverload<int>::of(&QButtonGroup::idClicked),
          updateConfigDisplay);

  // Initial display update
  updateConfigDisplay(currentPreset);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  mainLayout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    // Save the selected preset
    currentPreset = presetGroup->checkedId();

    // Always read from the spinboxes/checkboxes
    config.maxDepth = maxDepthSpin->value();
    config.minWordLength = minWordLengthSpin->value();
    config.minUniqueLetters = minUniqueLettersSpin->value();
    config.pruneRedundantPaths = prunePathsCheck->isChecked();
    config.pruneDominatedClasses = pruneClassesCheck->isChecked();

    return true;
  }

  return false;
}

void LetterBoxedWidget::initGame() {
  solutions.clear();
  resultsTable->setRowCount(0);
  config.allLetters.fill('*');

  // Clear the box display
  if (boxDisplay) {
    std::array<char, 12> emptyLetters;
    emptyLetters.fill('*');
    boxDisplay->setLetters(emptyLetters);
  }

  updateConfigInfo();
}
void LetterBoxedWidget::setUIEnabled(bool enabled) {
  Q_UNUSED(enabled);
  // All UI elements are always enabled now
}

void LetterBoxedWidget::updateConfigInfo() {
  QString info;
  info = QString("<span style='color:#666; font-size:11pt;'>Max depth: %1 | "
                 "Prune paths: %2 | Prune classes: %3 | Min unique letters: %4 "
                 "| Min word length: %5</span>")
             .arg(config.maxDepth)
             .arg(config.pruneRedundantPaths ? "Yes" : "No")
             .arg(config.pruneDominatedClasses ? "Yes" : "No")
             .arg(config.minUniqueLetters)
             .arg(config.minWordLength);
  configInfoLabel->setText(info);
}

void LetterBoxedWidget::createLetterBox() {
  // Create the box display widget
  boxDisplay = new LetterBoxDisplay(this);

  // Insert the box widget after the config label
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
  if (mainLayout) {
    int labelIndex = mainLayout->indexOf(configInfoLabel);
    if (labelIndex >= 0) {
      mainLayout->insertWidget(labelIndex + 1, boxDisplay, 0, Qt::AlignCenter);
    }
  }
}

void LetterBoxedWidget::updateLetterBoxFromInput(const QString &text) {
  QString cleaned = text.trimmed().toLower();
  cleaned.remove(' ');

  std::array<char, 12> letters;
  letters.fill('*');

  // Update each letter based on input
  for (int i = 0; i < 12 && i < cleaned.length(); ++i) {
    letters[i] = cleaned[i].toLatin1();
  }

  if (boxDisplay) {
    boxDisplay->setLetters(letters);
  }
}
void LetterBoxedWidget::onInputChanged(const QString &text) {
  updateLetterBoxFromInput(text);
}

void LetterBoxedWidget::populateResults(int maxRows) {
  resultsTable->setRowCount(0);

  if (solutions.empty()) {
    return;
  }

  // Show top maxRows solutions
  int limit = std::min(maxRows, static_cast<int>(solutions.size()));
  resultsTable->setRowCount(limit);

  for (int i = 0; i < limit; ++i) {
    const auto &solution = solutions[i];

    // Solution text column
    QTableWidgetItem *solutionItem =
        new QTableWidgetItem(QString::fromStdString(solution.text).toUpper());
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    solutionItem->setFont(monoFont);
    solutionItem->setTextAlignment(Qt::AlignCenter);
    resultsTable->setItem(i, 0, solutionItem);

    // Word count column
    QTableWidgetItem *countItem =
        new QTableWidgetItem(QString::number(solution.wordCount));
    countItem->setTextAlignment(Qt::AlignCenter);
    resultsTable->setItem(i, 1, countItem);

    // Color code by word count
    QColor bgColor;
    if (solution.wordCount <= 2) {
      bgColor = QColor(106, 170, 100); // Green for 2-word or fewer solutions
    } else if (solution.wordCount == 3) {
      bgColor = QColor(201, 180, 88); // Yellow for 3-word solutions
    } else {
      bgColor = QColor(120, 124, 126); // Grey for 4+ words
    }

    for (int col = 0; col < 2; ++col) {
      resultsTable->item(i, col)->setBackground(bgColor);
      resultsTable->item(i, col)->setForeground(Qt::white);
    }
  }
}

void LetterBoxedWidget::onInputSubmit() {
  QString lettersInput = ui->inputField->text().trimmed().toLower();
  if (lettersInput.isEmpty()) {
    QMessageBox::information(this, "Input Required",
                             "Please enter 12 letters!");
    return;
  }

  // Remove spaces and validate
  std::string letters = lettersInput.toStdString();
  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());

  if (letters.size() != 12) {
    QMessageBox::warning(this, "Invalid Input",
                         "Must provide exactly 12 letters!");
    return;
  }

  // Check for all letters (duplicates are fine)
  for (char c : letters) {
    if (!isalpha(static_cast<unsigned char>(c))) {
      QMessageBox::warning(this, "Invalid Input",
                           "All characters must be letters!");
      return;
    }
  }

  // Set up config
  for (size_t i = 0; i < 12; ++i) {
    config.allLetters[i] = letters[i];
  }

  // Set up letter mappings
  for (int i = 0; i < 3; ++i)
    config.letterToSideMapping[i] = 0;
  for (int i = 3; i < 6; ++i)
    config.letterToSideMapping[i] = 1;
  for (int i = 6; i < 9; ++i)
    config.letterToSideMapping[i] = 2;
  for (int i = 9; i < 12; ++i)
    config.letterToSideMapping[i] = 3;

  config.uniquePuzzleLetters.reset();
  for (int i = 0; i < 12; ++i)
    config.uniquePuzzleLetters.set(i);

  config.charToIndexMap.fill(-1);
  for (int i = 0; i < 12; ++i) {
    config.charToIndexMap[static_cast<unsigned char>(config.allLetters[i])] = i;
  }

  // Update the visual box display
  if (boxDisplay) {
    boxDisplay->setLetters(config.allLetters);
  }

  updateConfigInfo();

  // Auto-solve immediately after accepting letters
  onSolve();
}

void LetterBoxedWidget::onNewGame() { initGame(); }

void LetterBoxedWidget::onSettings() {
  showConfigDialog();
  updateConfigInfo();
}

void LetterBoxedWidget::onSolve() {
  if (config.allLetters[0] == '*') {
    QMessageBox::information(this, "No Puzzle",
                             "Enter 12 letters first to create a puzzle!");
    return;
  }

  // Clean up any existing thread and dialog using base class methods
  cleanupSolverThread();
  cleanupProgressDialog();

  // Reset cancellation flag
  cancellationRequested.store(false, std::memory_order_release);

  // Create progress dialog using base class method
  createProgressDialog("Solving Letter Boxed...", 0, 0);

  // Disable UI elements that could interfere with solving
  ui->inputField->setEnabled(false);
  ui->solveBtn->setEnabled(false);
  ui->newGameBtn->setEnabled(false);
  ui->settingsBtn->setEnabled(false);
  resultsTable->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(config, &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &LetterBoxedWidget::onSolverFinished);
  solverThread->start();
}

void LetterBoxedWidget::onSolverFinished() {
  // Re-enable UI elements
  ui->inputField->setEnabled(true);
  ui->solveBtn->setEnabled(true);
  ui->newGameBtn->setEnabled(true);
  ui->settingsBtn->setEnabled(true);
  resultsTable->setEnabled(true);

  // Use common handler - returns false if cancelled
  if (!handleSolverFinished()) {
    return;
  }

  solutions = static_cast<SolverThread *>(solverThread)->getResult();

  // If no solutions were found, inform the user
  if (solutions.empty()) {
    QMessageBox::information(this, "No Results", "No solutions found");
  } else {
    populateResults(1000);
  }

  // Clean up (non-blocking)
  cleanupProgressDialog();
  cleanupSolverThread();
}

#endif // WITH_GUI
