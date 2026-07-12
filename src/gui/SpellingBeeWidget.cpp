#ifdef WITH_GUI

#include "gui/SpellingBeeWidget.hpp"
#include "ui_SpellingBeeWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QWidget>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <random>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============ LetterButton Implementation ============

LetterButton::LetterButton(bool isFirst, QWidget *parent)
    : QPushButton(parent), isFirstLetter(isFirst), currentLetter('\0') {
  setFixedSize(50, 50);
  QFont font;
  font.setPointSize(20);
  font.setBold(true);
  setFont(font);
  setFlat(false);

  // Style the button with black letters
  if (isFirst) {
    setStyleSheet("LetterButton { background-color: #f7da21; color: black; "
                  "border: 2px solid "
                  "#3a3a3c; border-radius: 25px; }");
  } else {
    setStyleSheet("LetterButton { background-color: white; color: black; "
                  "border: 2px solid "
                  "#3a3a3c; border-radius: 25px; }");
  }
}

void LetterButton::setLetter(char letter) {
  currentLetter = letter;
  if (letter == '\0') {
    setText("");
  } else {
    setText(QString(QChar(letter).toUpper()));
  }
}

void LetterButton::paintEvent(QPaintEvent *event) {
  QPushButton::paintEvent(event);
}

// ============ SpellingBeeWidget Implementation ============

SpellingBeeWidget::SpellingBeeWidget(QWidget *parent)
    : GameWidget(parent), ui(new Ui::SpellingBeeWidget) {
  ui->setupUi(this);

  // Remove input length limit
  ui->inputField->setMaxLength(100);

  // Initialize config (defaults match frontend)
  config.validLettersMap.fill(false);
  config.mustIncludeFirstLetter = true;
  config.reuseLetters = true;
  config.excludeUncommonWords = true;

  // Store reference to config info label
  configInfoLabel = ui->configInfoLabel;

  // Get results table from UI and configure it
  resultsTable = ui->resultsTable;
  resultsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  resultsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");

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

  // Get references to UI elements for letters
  lettersScrollArea = ui->lettersScrollArea;
  lettersScrollWidget = ui->lettersScrollWidget;
  lettersButtonsLayout = ui->lettersButtonsLayout;

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
  dialog.setWindowTitle("Spelling Bee Settings");
  dialog.setMinimumWidth(300);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

  // Exclude Uncommon Words
  QCheckBox *excludeCheckbox = new QCheckBox(&dialog);
  excludeCheckbox->setChecked(config.excludeUncommonWords);
  formLayout->addRow("Exclude Uncommon Words:", excludeCheckbox);

  // Must Include First Letter (center letter on frontend)
  QCheckBox *mustIncludeFirstCheckbox = new QCheckBox(&dialog);
  mustIncludeFirstCheckbox->setChecked(config.mustIncludeFirstLetter);
  formLayout->addRow("Must Include Center Letter:", mustIncludeFirstCheckbox);

  // Reuse Letters
  QCheckBox *reuseLettersCheckbox = new QCheckBox(&dialog);
  reuseLettersCheckbox->setChecked(config.reuseLetters);
  formLayout->addRow("Allow Letter Reuse:", reuseLettersCheckbox);

  layout->addLayout(formLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    config.excludeUncommonWords = excludeCheckbox->isChecked();
    config.mustIncludeFirstLetter = mustIncludeFirstCheckbox->isChecked();
    config.reuseLetters = reuseLettersCheckbox->isChecked();
    return true;
  }
  return false;
}

void SpellingBeeWidget::initGame() {
  solutions.clear();
  resultsTable->setRowCount(0);
  config.allLetters.clear();
  config.validLettersMap.fill(false);

  // Clear all letter buttons from layout (keeping the spacer at the end)
  QLayoutItem *item;
  while (lettersButtonsLayout->count() > 1) { // Keep the last item (spacer)
    item = lettersButtonsLayout->takeAt(0);
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  letterButtons.clear();

  updateConfigInfo();
}

void SpellingBeeWidget::setUIEnabled(bool enabled) {
  Q_UNUSED(enabled);
  // All UI elements are always enabled now
}

void SpellingBeeWidget::updateConfigInfo() {
  QString info;
  if (config.allLetters.empty()) {
    info = "<span style='color:#666; font-size:11pt;'>Enter letters "
           "(minimum 3, duplicates allowed, first is special)</span>";
    ui->scoreLabel->setText("");
  } else {
    info = QString("<span style='color:#666; font-size:11pt;'>%1 letters "
                   "configured</span>")
               .arg(config.allLetters.size());
  }
  configInfoLabel->setText(info);
}

void SpellingBeeWidget::createLetterButtons(int count) {
  // Clear existing buttons from layout (keeping the spacer at the end)
  QLayoutItem *item;
  while (lettersButtonsLayout->count() > 1) { // Keep the last item (spacer)
    item = lettersButtonsLayout->takeAt(0);
    if (item->widget()) {
      item->widget()->deleteLater();
    }
    delete item;
  }
  letterButtons.clear();

  // Create new buttons and add to layout (before the spacer)
  for (int i = 0; i < count; ++i) {
    LetterButton *btn = new LetterButton(i == 0, lettersScrollWidget);
    letterButtons.push_back(btn);
    lettersButtonsLayout->insertWidget(i, btn); // Insert before the spacer
  }
}

void SpellingBeeWidget::updateLetterButtonsFromInput(const QString &text) {
  QString cleaned = text.trimmed().toLower();
  cleaned.remove(' ');

  // Create buttons if count changed
  if (letterButtons.size() != cleaned.length()) {
    createLetterButtons(cleaned.length());
  }

  // Update each button based on input
  for (int i = 0; i < letterButtons.size(); ++i) {
    if (i < cleaned.length()) {
      letterButtons[i]->setLetter(cleaned[i].toLatin1());
    } else {
      letterButtons[i]->setLetter('\0');
    }
  }
}

void SpellingBeeWidget::onInputChanged(const QString &text) {
  updateLetterButtonsFromInput(text);
}

void SpellingBeeWidget::populateResults(int maxRows) {
  resultsTable->setRowCount(0);

  if (solutions.empty()) {
    return;
  }

  // Match frontend: Word | Length | Unique Letters
  if (resultsTable->columnCount() < 3) {
    resultsTable->setColumnCount(3);
    resultsTable->setHorizontalHeaderLabels(
        {QStringLiteral("Word"), QStringLiteral("Length"),
         QStringLiteral("Unique Letters")});
  }

  int limit = std::min(maxRows, static_cast<int>(solutions.size()));
  resultsTable->setRowCount(limit);

  const int puzzleUnique = static_cast<int>([&]() {
    std::set<char> uniq;
    for (char c : config.allLetters)
      uniq.insert(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return uniq.size();
  }());

  for (int i = 0; i < limit; ++i) {
    const auto &word = solutions[i];

    QTableWidgetItem *wordItem =
        new QTableWidgetItem(QString::fromStdString(word.wordString).toUpper());
    QFont monoFont("Consolas", 10);
    monoFont.setBold(true);
    wordItem->setFont(monoFont);
    resultsTable->setItem(i, 0, wordItem);

    QTableWidgetItem *lengthItem =
        new QTableWidgetItem(QString::number(
            static_cast<int>(word.wordString.size())));
    lengthItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    resultsTable->setItem(i, 1, lengthItem);

    QTableWidgetItem *lettersItem =
        new QTableWidgetItem(QString::number(word.uniqueLetters));
    lettersItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    resultsTable->setItem(i, 2, lettersItem);

    // Pangrams only (match frontend yellow highlight)
    const bool isPangram =
        puzzleUnique > 0 && word.uniqueLetters == puzzleUnique;
    if (isPangram) {
      const QColor pangramBg(255, 235, 59, 38); // ~15% yellow
      for (int col = 0; col < 3; ++col) {
        resultsTable->item(i, col)->setBackground(pangramBg);
      }
    }
  }
}

void SpellingBeeWidget::onInputSubmit() {
  QString lettersInput = ui->inputField->text().trimmed().toLower();
  if (lettersInput.isEmpty()) {
    QMessageBox::information(this, "Input Required", "Please enter letters!");
    return;
  }

  // Remove spaces and validate
  std::string letters = lettersInput.toStdString();
  letters.erase(std::remove_if(letters.begin(), letters.end(), ::isspace),
                letters.end());

  if (letters.size() < 3) {
    QMessageBox::warning(this, "Invalid Input",
                         "Must provide at least 3 letters!");
    return;
  }

  // Validate letters (duplicates now allowed)
  for (char c : letters) {
    if (!isalpha(static_cast<unsigned char>(c))) {
      QMessageBox::warning(this, "Invalid Input",
                           "All characters must be letters!");
      return;
    }
  }

  // Set up config
  config.allLetters.clear();
  for (char c : letters) {
    config.allLetters.push_back(c);
  }

  config.validLettersMap.fill(false);
  for (char c : config.allLetters) {
    config.validLettersMap[static_cast<unsigned char>(c)] = true;
  }

  // Update the visual letter buttons
  updateLetterButtonsFromInput(lettersInput);

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
