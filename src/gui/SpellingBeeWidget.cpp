#ifdef WITH_GUI

#include "gui/SpellingBeeWidget.hpp"
#include "ui_SpellingBeeWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QPushButton>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <random>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============ HexagonButton Implementation ============

HexagonButton::HexagonButton(bool isCenter, QWidget *parent)
    : QPushButton(parent), isCenterHex(isCenter), currentLetter('\0') {
  setFixedSize(70, 70);
  QFont font;
  font.setPointSize(24);
  font.setBold(true);
  setFont(font);
  setFlat(true);
}

void HexagonButton::setLetter(char letter) {
  currentLetter = letter;
  if (letter == '\0') {
    setText("");
  } else {
    setText(QString(QChar(letter).toUpper()));
  }
  update();
}

void HexagonButton::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  // Draw hexagon
  QPolygonF hexagon;
  double centerX = width() / 2.0;
  double centerY = height() / 2.0;
  double radius = 35.0;

  for (int i = 0; i < 6; ++i) {
    double angle = M_PI / 3.0 * i;
    double x = centerX + radius * cos(angle);
    double y = centerY + radius * sin(angle);
    hexagon << QPointF(x, y);
  }

  // Fill hexagon
  QPainterPath path;
  path.addPolygon(hexagon);

  if (isCenterHex) {
    painter.fillPath(path, QColor("#f7da21")); // Yellow for center
  } else {
    painter.fillPath(path, QColor("#e6e6e6")); // Light gray for outer
  }

  // Draw border
  painter.setPen(QPen(QColor("#3a3a3c"), 2));
  painter.drawPolygon(hexagon);

  // Draw letter
  if (currentLetter != '\0') {
    painter.setPen(Qt::black);
    painter.setFont(font());
    painter.drawText(rect(), Qt::AlignCenter,
                     QString(QChar(currentLetter).toUpper()));
  }
}

// ============ SpellingBeeWidget Implementation ============

SpellingBeeWidget::SpellingBeeWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::SpellingBeeWidget) {
  ui->setupUi(this);

  // Limit input to 7 characters
  ui->inputField->setMaxLength(7);

  // Initialize config
  config.allLetters.fill('\0');
  config.validLettersMap.fill(false);

  // Initialize hex buttons
  hexButtons.fill(nullptr);

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Create results table
  resultsTable = new QTableWidget(this);
  resultsTable->setColumnCount(2);
  resultsTable->setHorizontalHeaderLabels({"Word", "Unique Letters"});
  resultsTable->horizontalHeader()->setStretchLastSection(true);
  resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  resultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onNewGame);
  // Enter should trigger solve immediately
  connect(ui->inputField, &QLineEdit::returnPressed, this,
          &SpellingBeeWidget::onInputSubmit);
  connect(ui->inputField, &QLineEdit::textChanged, this,
          &SpellingBeeWidget::onInputChanged);
  connect(ui->solveBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onInputSubmit);

  // Settings button (repurposed shuffle button)
  ui->settingsBtn->setToolTip("Solver Settings");
  ui->settingsBtn->setMaximumWidth(40);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &SpellingBeeWidget::onSettings);

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Create the hexagon visualization
  createHexagons();

  // Set initial state
  gameInitialized = true;
  updateConfigInfo();
}

SpellingBeeWidget::~SpellingBeeWidget() {
  // Base class destructor handles thread and dialog cleanup
  delete ui;
}

void SpellingBeeWidget::newGame() { onNewGame(); }

bool SpellingBeeWidget::showConfigDialog() {
  // Show configuration dialog
  QDialog dialog(this);
  dialog.setWindowTitle("Spelling Bee Solver Configuration");
  dialog.setMinimumWidth(300);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

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
    config.excludeUncommonWords = excludeCheckbox->isChecked();
    return true;
  }
  return false;
}

void SpellingBeeWidget::initGame() {
  solutions.clear();
  resultsTable->setRowCount(0);
  config.allLetters.fill('\0');
  config.validLettersMap.fill(false);

  // Clear all hexagons
  for (int i = 0; i < 7; ++i) {
    if (hexButtons[i]) {
      hexButtons[i]->setLetter('\0');
    }
  }

  updateConfigInfo();
}

void SpellingBeeWidget::setUIEnabled(bool enabled) {
  Q_UNUSED(enabled);
  // All UI elements are always enabled now
}

void SpellingBeeWidget::updateConfigInfo() {
  QString info;
  if (config.allLetters[0] == '\0') {
    info = "<span style='color:#666; font-size:11pt;'>Enter 7 unique letters "
           "(first is "
           "center)</span>";
    ui->scoreLabel->setText("");
  } else {
    info =
        "<span style='color:#666; font-size:11pt;'>7 letters configured</span>";
  }
  configInfoLabel->setText(info);
}

void SpellingBeeWidget::createHexagons() {
  // Create a widget to hold the hexagons
  hexWidget = new QWidget(this);

  // Use absolute positioning for perfect hexagon
  hexWidget->setFixedSize(220, 250);

  // Create 7 hexagon buttons
  for (int i = 0; i < 7; ++i) {
    hexButtons[i] = new HexagonButton(i == 0, hexWidget);
  }

  // Position them in perfect hexagon pattern
  // Calculate positions based on hexagon geometry
  int centerX = 110;
  int centerY = 125;
  int radius = 65; // Distance from center to outer hexagons (increased for gap)

  // Center hexagon (index 0)
  hexButtons[0]->move(centerX - 35, centerY - 35);

  // Outer hexagons arranged in a circle
  // Top (index 1)
  hexButtons[1]->move(centerX - 35, centerY - radius - 35);

  // Top-right (index 2)
  hexButtons[2]->move(centerX + radius * 0.866 - 35,
                      centerY - radius * 0.5 - 35);

  // Bottom-right (index 3)
  hexButtons[3]->move(centerX + radius * 0.866 - 35,
                      centerY + radius * 0.5 - 35);

  // Bottom (index 4)
  hexButtons[4]->move(centerX - 35, centerY + radius - 35);

  // Bottom-left (index 5)
  hexButtons[5]->move(centerX - radius * 0.866 - 35,
                      centerY + radius * 0.5 - 35);

  // Top-left (index 6)
  hexButtons[6]->move(centerX - radius * 0.866 - 35,
                      centerY - radius * 0.5 - 35);

  hexWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  // Insert the hex widget after the config label
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(this->layout());
  if (mainLayout) {
    int labelIndex = mainLayout->indexOf(configInfoLabel);
    if (labelIndex >= 0) {
      mainLayout->insertWidget(labelIndex + 1, hexWidget, 0, Qt::AlignCenter);
    }
  }
}

void SpellingBeeWidget::updateHexagonsFromInput(const QString &text) {
  QString cleaned = text.trimmed().toLower();
  cleaned.remove(' ');

  // Update each hexagon based on input
  for (int i = 0; i < 7; ++i) {
    if (i < cleaned.length()) {
      hexButtons[i]->setLetter(cleaned[i].toLatin1());
    } else {
      hexButtons[i]->setLetter('\0');
    }
  }
}

void SpellingBeeWidget::onInputChanged(const QString &text) {
  updateHexagonsFromInput(text);
}

void SpellingBeeWidget::populateResults(int maxRows) {
  resultsTable->setRowCount(0);

  if (solutions.empty()) {
    return;
  }
  int limit = std::min(maxRows, static_cast<int>(solutions.size()));
  resultsTable->setRowCount(limit);

  for (int i = 0; i < limit; ++i) {
    const auto &word = solutions[i];

    // Word column
    QTableWidgetItem *wordItem =
        new QTableWidgetItem(QString::fromStdString(word.wordString).toUpper());
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    wordItem->setFont(monoFont);
    wordItem->setTextAlignment(Qt::AlignCenter);
    resultsTable->setItem(i, 0, wordItem);

    // Unique Letters column
    QTableWidgetItem *lettersItem =
        new QTableWidgetItem(QString::number(word.uniqueLetters));
    lettersItem->setTextAlignment(Qt::AlignCenter);
    resultsTable->setItem(i, 1, lettersItem);

    // Color code by unique letters
    QColor bgColor;
    if (word.uniqueLetters == 7) {
      bgColor = QColor(106, 170, 100); // Green
    } else if (word.uniqueLetters >= 5) {
      bgColor = QColor(201, 180, 88); // Yellow
    } else {
      bgColor = QColor(120, 124, 126); // Grey
    }

    for (int col = 0; col < 2; ++col) {
      resultsTable->item(i, col)->setBackground(bgColor);
      resultsTable->item(i, col)->setForeground(Qt::white);
    }
  }
}

void SpellingBeeWidget::onInputSubmit() {
  QString lettersInput = ui->inputField->text().trimmed().toLower();
  if (lettersInput.isEmpty()) {
    QMessageBox::information(this, "Input Required", "Please enter 7 letters!");
    return;
  }

  // Remove spaces and validate
  std::string letters = lettersInput.toStdString();
  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());

  if (letters.size() != 7) {
    QMessageBox::warning(this, "Invalid Input",
                         "Must provide exactly 7 letters!");
    return;
  }

  // Check for duplicates
  std::set<char> seen;
  for (char c : letters) {
    if (!isalpha(static_cast<unsigned char>(c))) {
      QMessageBox::warning(this, "Invalid Input",
                           "All characters must be letters!");
      return;
    }
    if (seen.count(c)) {
      QMessageBox::warning(this, "Invalid Input",
                           "Duplicate letters not allowed!");
      return;
    }
    seen.insert(c);
  }

  // Set up config
  for (size_t i = 0; i < 7; ++i) {
    config.allLetters[i] = letters[i];
  }

  config.validLettersMap.fill(false);
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  // Update the visual hexagons
  for (int i = 0; i < 7; ++i) {
    hexButtons[i]->setLetter(config.allLetters[i]);
  }

  updateConfigInfo();

  // Clean up any existing thread and dialog using base class methods
  cleanupSolverThread();
  cleanupProgressDialog();

  // Reset cancellation flag
  cancellationRequested.store(false, std::memory_order_release);

  // Create progress dialog using base class method
  createProgressDialog("Solving Spelling Bee...", 0, 0);

  // Disable UI elements that could interfere with solving
  ui->inputField->setEnabled(false);
  ui->solveBtn->setEnabled(false);
  ui->newGameBtn->setEnabled(false);
  ui->settingsBtn->setEnabled(false);
  resultsTable->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(config, &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &SpellingBeeWidget::onSolverFinished);
  solverThread->start();
}

void SpellingBeeWidget::onSolverFinished() {
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
    // Display results
    populateResults(1000);
    ui->scoreLabel->setText(QString("Found: %1 words").arg(solutions.size()));
  }

  // Clean up (non-blocking)
  cleanupProgressDialog();
  cleanupSolverThread();
}

void SpellingBeeWidget::onNewGame() { initGame(); }

void SpellingBeeWidget::onSettings() {
  showConfigDialog();
  updateConfigInfo();
}

#endif // WITH_GUI
