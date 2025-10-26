#ifdef WITH_GUI

#include "gui/DungleonWidget.hpp"

#include "dungleon/dungleon.hpp"
#include "ui_DungleonWidget.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSpinBox>
#include <QString>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <algorithm>

// ============ CharacterSlot Implementation ============

CharacterSlot::CharacterSlot(QWidget *parent)
    : QLabel(parent), m_characterId(-1), m_color(0) {
  setFixedSize(48, 48);
  setAlignment(Qt::AlignCenter);
  QFont font;
  font.setPointSize(10);
  font.setBold(true);
  setFont(font);
  setCursor(Qt::PointingHandCursor);
  updateStyle();
}

void CharacterSlot::setCharacter(int charId) {
  m_characterId = charId;
  if (charId >= 0 &&
      charId < static_cast<int>(Dungleon::CHARACTER_IDS.size())) {
    QString label = QString::fromStdString(Dungleon::CHARACTER_IDS[charId]);
    setText(label);

    // Try to load icon
    std::string name = Dungleon::CHARACTER_NAMES[charId];
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
      if (c == ' ')
        return '_';
      return static_cast<char>(std::tolower(c));
    });
    QString path =
        QString("resources/dungleon/%1.png").arg(QString::fromStdString(name));
    QPixmap icon(path);
    if (!icon.isNull()) {
      setPixmap(
          icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      setText("");
    }
  } else {
    setText("");
    setPixmap(QPixmap());
  }
  // Update badge visibility according to current color
  QLabel *badge = findChild<QLabel *>("plusBadge");
  if (badge) {
    if (m_color == 3 || m_color == 4)
      badge->show();
    else
      badge->hide();
    badge->raise();
  }
  updateStyle();
}

void CharacterSlot::setColor(int color) {
  m_color = color % 5; // 0-4
  updateStyle();

  // Show or hide + badge for colors 3 and 4 (one more present)
  if (m_characterId >= 0) {
    QLabel *badge = findChild<QLabel *>("plusBadge");
    if (m_color == 3 || m_color == 4) {
      if (!badge) {
        badge = new QLabel("+", this);
        badge->setObjectName("plusBadge");
        badge->setStyleSheet(
            "QLabel { background: rgba(0,0,0,150); color: white; "
            "font-weight: bold; font-size: 12px; border-radius: 8px; "
            "padding: 0px; }");
        badge->setFixedSize(14, 14);
        badge->setAlignment(Qt::AlignCenter);
        badge->move(width() - 18, 2);
        badge->setAttribute(Qt::WA_TransparentForMouseEvents);
        badge->show();
      } else {
        badge->show();
      }
      badge->raise();
    } else {
      if (badge)
        badge->hide();
    }
  }
}

void CharacterSlot::clear() {
  m_characterId = -1;
  m_color = 0;
  setText("");
  setPixmap(QPixmap());

  // Remove badge if it exists
  QLabel *badge = findChild<QLabel *>("plusBadge");
  if (badge) {
    badge->deleteLater();
  }

  updateStyle();
}

void CharacterSlot::updateStyle() {
  QString bgColor, textColor, border;

  // Dungleon colors: 0=not present, 1=diff pos no more, 2=correct pos no more,
  // 3=diff pos one more, 4=correct pos one more
  switch (m_color) {
  case 0: // Not present
    bgColor = "#787c7e";
    textColor = "white";
    border = "#787c7e";
    break;
  case 1: // Different position, no more (yellow-ish)
    bgColor = "#c9b458";
    textColor = "white";
    border = "#c9b458";
    break;
  case 2: // Correct position, no more (green)
    bgColor = "#6aaa64";
    textColor = "white";
    border = "#6aaa64";
    break;
  case 3: // Different position, one more (lighter yellow)
    bgColor = "#e5d366";
    textColor = "white";
    border = "#e5d366";
    break;
  case 4: // Correct position, one more (lighter green)
    bgColor = "#8bc589";
    textColor = "white";
    border = "#8bc589";
    break;
  default:
    bgColor = "#787c7e";
    textColor = "white";
    border = "#787c7e";
  }

  // If empty, show blank box
  if (m_characterId < 0) {
    bgColor = "#d3d6da";
    border = "#878a8c";
    textColor = "#000";
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

void CharacterSlot::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && m_characterId >= 0) {
    // Cycle through colors
    m_color = (m_color + 1) % 5;
    updateStyle();
    emit clicked();
  } else if (event->button() == Qt::RightButton && m_characterId >= 0) {
    // Right-click signals intent to clear this slot
    emit rightClicked();
  }
  QLabel::mousePressEvent(event);
}

// ============ PatternRow Implementation ============

PatternRow::PatternRow(const Dungleon::Feedback &feedback, QWidget *parent)
    : QWidget(parent), isEditable(false) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(5);
  layout->setContentsMargins(0, 5, 0, 5);

  // Create character slots
  for (int i = 0; i < 5; ++i) {
    CharacterSlot *slot = new CharacterSlot(this);
    slot->setCharacter(feedback.pattern.characters[i]);
    slot->setColor(feedback.getColor(i));
    connect(slot, &CharacterSlot::clicked, this, &PatternRow::onSlotClicked);
    characterSlots[i] = slot;
    layout->addWidget(slot);
  }

  layout->addSpacing(10);

  // Delete button
  deleteBtn = new QPushButton("✕", this);
  deleteBtn->setFixedSize(30, 30);
  deleteBtn->setToolTip("Delete this pattern");
  deleteBtn->setStyleSheet("QPushButton { background-color: #dc3545; color: "
                           "white; border: none; border-radius: 4px; }");
  connect(deleteBtn, &QPushButton::clicked, this, &PatternRow::onDeleteClicked);
  layout->addWidget(deleteBtn);

  layout->addStretch();
}

Dungleon::Feedback PatternRow::getFeedback() const {
  Dungleon::Feedback fb;
  for (int i = 0; i < 5; ++i) {
    fb.pattern.characters[i] = characterSlots[i]->getCharacter();
    fb.setColor(i, characterSlots[i]->getColor());
  }
  fb.pattern.computeCharacterCount();
  return fb;
}

void PatternRow::setEditable(bool editable) {
  isEditable = editable;
  for (int i = 0; i < 5; ++i) {
    characterSlots[i]->setEnabled(editable);
  }
}

void PatternRow::onSlotClicked() {
  if (isEditable) {
    // Rebuild feedback history
    emit editRequested();
  }
}

void PatternRow::onDeleteClicked() { emit deleteRequested(); }

// ============ DungleonWidget Implementation ============

DungleonWidget::DungleonWidget(QWidget *parent)
    : GameWidget(parent), currentSlotIndex(0) {
  ui = new Ui::DungleonWidget();
  ui->setupUi(this);

  // Initialize config
  config.maxDepth = 1;

  // Create character bank
  m_bankContainer = new QWidget(ui->bankScrollArea);
  QHBoxLayout *bankLayout = new QHBoxLayout(m_bankContainer);
  bankLayout->setSpacing(4);
  bankLayout->setContentsMargins(6, 6, 6, 6);

  for (int i = 0; i < 20; ++i) {
    QPushButton *btn = new QPushButton(m_bankContainer);
    btn->setFixedSize(48, 48);
    QString label = QString::fromStdString(Dungleon::CHARACTER_IDS[i]);
    btn->setToolTip(QString::fromStdString(Dungleon::CHARACTER_NAMES[i]));
    btn->setText(label);

    // Try to load icon
    std::string name = Dungleon::CHARACTER_NAMES[i];
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
      if (c == ' ')
        return '_';
      return static_cast<char>(std::tolower(c));
    });
    QString path =
        QString("resources/dungleon/%1.png").arg(QString::fromStdString(name));
    QPixmap icon(path);
    if (!icon.isNull()) {
      btn->setIcon(QIcon(icon));
      btn->setIconSize(QSize(40, 40));
      btn->setText("");
    }

    btn->setStyleSheet("QPushButton { background-color: #d3d6da; border: 2px "
                       "solid #878a8c; border-radius: 4px; }");
    connect(btn, &QPushButton::clicked, this,
            [this, i]() { onCharacterBankClicked(i); });
    m_bankButtons[i] = btn;
    bankLayout->addWidget(btn);
  }

  m_bankContainer->setLayout(bankLayout);
  ui->bankScrollArea->setWidget(m_bankContainer);

  // Create pattern list container
  patternListWidget = new QWidget(this);
  patternListLayout = new QVBoxLayout(patternListWidget);
  patternListLayout->setSpacing(2);
  patternListLayout->setContentsMargins(0, 0, 0, 0);
  patternListLayout->setAlignment(Qt::AlignHCenter);
  patternListLayout->addStretch();

  // Wrap in scroll area
  patternListScrollArea = new QScrollArea(this);
  patternListScrollArea->setWidget(patternListWidget);
  patternListScrollArea->setWidgetResizable(true);
  patternListScrollArea->setMaximumHeight(200);
  patternListScrollArea->setFrameShape(QFrame::NoFrame);

  // Create container for current input row (separate from submitted patterns)
  currentInputWidget = new QWidget(this);
  currentInputLayout = new QHBoxLayout(currentInputWidget);
  currentInputLayout->setSpacing(5);
  currentInputLayout->setContentsMargins(0, 5, 0, 5);

  // Add to main layout: insert current input above submit button, and the
  // submitted-patterns scroll area below the submit button.
  QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
  if (mainLayout) {
    int submitIdx = mainLayout->indexOf(ui->submitBtn);
    mainLayout->insertWidget(submitIdx, currentInputWidget);
    // Recompute submit index (it shifts when we insert)
    int submitIdx2 = mainLayout->indexOf(ui->submitBtn);
    mainLayout->insertWidget(submitIdx2 + 1, patternListScrollArea);
  }

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &DungleonWidget::onSubmit);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &DungleonWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this, &DungleonWidget::onHint);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &DungleonWidget::onSettings);

  // Create result tables
  allResultsTable = new QTableWidget(this);
  allResultsTable->setColumnCount(4);
  allResultsTable->setHorizontalHeaderLabels(
      {"Rank", "Pattern", "ENT", "Probability"});
  allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  allResultsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  allResultsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  allResultsTable->setAlternatingRowColors(true);
  connect(allResultsTable, &QTableWidget::cellClicked, this,
          &DungleonWidget::onTableRowClicked);

  probablePatternsTable = new QTableWidget(this);
  probablePatternsTable->setColumnCount(4);
  probablePatternsTable->setHorizontalHeaderLabels(
      {"Rank", "Pattern", "ENT", "Probability"});
  probablePatternsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  probablePatternsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  probablePatternsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  probablePatternsTable->setAlternatingRowColors(true);
  connect(probablePatternsTable, &QTableWidget::cellClicked, this,
          &DungleonWidget::onTableRowClicked);

  // Add tables to tab widget
  QVBoxLayout *allResultsLayout = new QVBoxLayout();
  allResultsLayout->addWidget(allResultsTable);
  ui->resultsTabWidget->widget(0)->setLayout(allResultsLayout);

  QVBoxLayout *probablePatternsLayout = new QVBoxLayout();
  probablePatternsLayout->addWidget(probablePatternsTable);
  ui->resultsTabWidget->widget(1)->setLayout(probablePatternsLayout);

  // Store config info label reference
  configInfoLabel = ui->configInfoLabel;

  // Initialize game
  initGame();
  updateConfigInfo();
}

DungleonWidget::~DungleonWidget() { delete ui; }

bool DungleonWidget::showConfigDialog() {
  QDialog dialog(this);
  dialog.setWindowTitle("Dungleon Solver Configuration");
  dialog.setMinimumWidth(400);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

  // Max Depth
  QSpinBox *maxDepthSpinner = new QSpinBox(&dialog);
  maxDepthSpinner->setRange(0, 3);
  maxDepthSpinner->setValue(config.maxDepth);
  formLayout->addRow("Search Depth:", maxDepthSpinner);

  layout->addLayout(formLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    config.maxDepth = static_cast<uint8_t>(maxDepthSpinner->value());
    updateConfigInfo();
    return true;
  }
  return false;
}

void DungleonWidget::initGame() {
  // Clear feedback history
  config.feedbackHistory.clear();

  // Delete all PatternRow widgets
  for (PatternRow *row : patternRows) {
    patternListLayout->removeWidget(row);
    row->deleteLater();
  }
  patternRows.clear();

  // Remove current pattern widget if it exists
  if (currentPatternWidget) {
    if (currentInputLayout)
      currentInputLayout->removeWidget(currentPatternWidget);
    currentPatternWidget->deleteLater();
    currentPatternWidget = nullptr;
  }

  // Clear result tables
  allResultsTable->setRowCount(0);
  probablePatternsTable->setRowCount(0);

  // Reset tab texts
  ui->resultsTabWidget->setTabText(0, "All Suggestions");
  ui->resultsTabWidget->setTabText(1, "Possible Solutions");

  // Setup fresh current pattern
  setupCurrentPattern();

  updateConfigInfo();
}

void DungleonWidget::setUIEnabled(bool enabled) {
  ui->bankScrollArea->setVisible(enabled);
  ui->submitBtn->setVisible(enabled);
  ui->solveBtn->setVisible(enabled);
  patternListScrollArea->setVisible(enabled);
  ui->resultsTabWidget->setVisible(enabled);
}

void DungleonWidget::updateConfigInfo() {
  QString info = QString("<span style='color:#666; font-size:11pt;'>Search "
                         "Depth: %1 | Patterns: %2</span>")
                     .arg(config.maxDepth)
                     .arg(patternRows.size());
  configInfoLabel->setText(info);
}

void DungleonWidget::setupCurrentPattern() {
  // Remove previous input if present
  if (currentPatternWidget) {
    if (currentInputLayout)
      currentInputLayout->removeWidget(currentPatternWidget);
    currentPatternWidget->deleteLater();
    currentPatternWidget = nullptr;
  }

  // Create widget with 5 empty slots and add to current input container
  currentPatternWidget = new QWidget(this);
  QHBoxLayout *rowLayout = new QHBoxLayout(currentPatternWidget);
  rowLayout->setSpacing(5);
  rowLayout->setContentsMargins(0, 5, 0, 5);

  for (int i = 0; i < 5; ++i) {
    CharacterSlot *slot = new CharacterSlot(currentPatternWidget);
    currentSlots[i] = slot;
    // connect right-click to clear/shift behavior
    connect(slot, &CharacterSlot::rightClicked, this, [this, i]() {
      // If there's nothing in this slot, nothing to do
      if (currentSlots[i]->getCharacter() < 0)
        return;
      // Shift following characters left
      for (int j = i; j < 4; ++j) {
        int cid = currentSlots[j + 1]->getCharacter();
        int col = currentSlots[j + 1]->getColor();
        if (cid >= 0) {
          currentSlots[j]->setCharacter(cid);
          currentSlots[j]->setColor(col);
        } else {
          currentSlots[j]->clear();
        }
      }
      // Clear last slot
      currentSlots[4]->clear();
      if (currentSlotIndex > 0)
        --currentSlotIndex;
    });

    rowLayout->addWidget(slot);
  }

  // Backspace button to remove last character
  if (currentBackspaceBtn) {
    currentBackspaceBtn->deleteLater();
    currentBackspaceBtn = nullptr;
  }
  currentBackspaceBtn =
      new QPushButton(QString::fromUtf8("⌫"), currentPatternWidget);
  currentBackspaceBtn->setFixedSize(36, 36);
  currentBackspaceBtn->setToolTip("Remove last character");
  currentBackspaceBtn->setStyleSheet(
      "QPushButton { background-color: #f0f0f0; border: 1px solid #ccc;"
      " border-radius: 4px; }");
  connect(currentBackspaceBtn, &QPushButton::clicked, this, [this]() {
    if (currentSlotIndex > 0) {
      --currentSlotIndex;
      currentSlots[currentSlotIndex]->clear();
    }
  });

  rowLayout->addWidget(currentBackspaceBtn);
  rowLayout->addStretch();

  // Add to current input layout (above submit button)
  if (currentInputLayout)
    currentInputLayout->addWidget(currentPatternWidget);

  currentSlotIndex = 0;
}

void DungleonWidget::onCharacterBankClicked(int charId) {
  // Add character to next empty slot
  if (currentSlotIndex < 5) {
    currentSlots[currentSlotIndex]->setCharacter(charId);
    currentSlotIndex++;
  }
}

void DungleonWidget::onSubmit() { submitCurrentPattern(); }

void DungleonWidget::submitCurrentPattern() {
  // Check if all 5 slots are filled
  if (currentSlotIndex < 5) {
    QMessageBox::warning(
        this, "Incomplete Pattern",
        "Please fill all 5 character slots before submitting.");
    return;
  }

  // Create feedback object with pattern
  Dungleon::Feedback fb;
  for (int i = 0; i < 5; ++i) {
    fb.pattern.characters[i] = currentSlots[i]->getCharacter();
    fb.setColor(i, 0); // Default to "not present"
  }
  fb.pattern.computeCharacterCount();

  config.feedbackHistory.push_back(fb);

  // Remove current pattern from input layout
  if (currentInputLayout)
    currentInputLayout->removeWidget(currentPatternWidget);
  currentPatternWidget->deleteLater();

  // Create a PatternRow for this feedback
  PatternRow *patternRow = new PatternRow(fb, patternListWidget);
  patternRow->setEditable(true);
  connect(patternRow, &PatternRow::deleteRequested, this,
          &DungleonWidget::onPatternDeleted);
  connect(patternRow, &PatternRow::editRequested, this,
          [this]() { rebuildFeedbackHistory(); });

  patternRows.push_back(patternRow);

  // Add to layout before stretch
  patternListLayout->insertWidget(patternListLayout->count() - 1, patternRow);

  // Setup new current pattern
  setupCurrentPattern();
}

void DungleonWidget::rebuildFeedbackHistory() {
  config.feedbackHistory.clear();
  for (PatternRow *row : patternRows) {
    config.feedbackHistory.push_back(row->getFeedback());
  }
}

void DungleonWidget::onPatternDeleted() {
  PatternRow *row = qobject_cast<PatternRow *>(sender());
  if (row) {
    auto it = std::find(patternRows.begin(), patternRows.end(), row);
    if (it != patternRows.end()) {
      patternRows.erase(it);
    }
    patternListLayout->removeWidget(row);
    row->deleteLater();
    rebuildFeedbackHistory();
    updateConfigInfo();
  }
}

void DungleonWidget::onTableRowClicked(int row, int column) {
  Q_UNUSED(column);

  QTableWidget *table = qobject_cast<QTableWidget *>(sender());
  if (!table)
    return;

  // Get the pattern from stored results
  const std::vector<Dungleon::PatternGuess> *results = nullptr;
  if (table == allResultsTable) {
    results = &lastAllResults;
  } else if (table == probablePatternsTable) {
    results = &lastProbableResults;
  }

  if (!results || row >= static_cast<int>(results->size()))
    return;

  const Dungleon::Pattern &pattern = (*results)[row].pattern;

  // Clear current pattern
  for (int i = 0; i < 5; ++i) {
    currentSlots[i]->clear();
  }
  currentSlotIndex = 0;

  // Fill with pattern from table
  for (int i = 0; i < 5; ++i) {
    currentSlots[i]->setCharacter(pattern.characters[i]);
    currentSlotIndex++;
  }
}

void DungleonWidget::newGame() { onNewGame(); }

void DungleonWidget::onNewGame() {
  initGame();
  updateConfigInfo();
}

void DungleonWidget::onSettings() {
  showConfigDialog();
  updateConfigInfo();
}

void DungleonWidget::onHint() { solveDungleon(); }

void DungleonWidget::solveDungleon() {
  // Clean up any existing thread and dialog using base class methods
  cleanupSolverThread();
  cleanupProgressDialog();

  // Reset cancellation flag
  cancellationRequested.store(false, std::memory_order_release);

  // Rebuild feedback history from UI
  rebuildFeedbackHistory();

  // Create progress dialog using base class method
  createProgressDialog("Calculating optimal patterns...", 0, 0);

  // Disable UI elements that could interfere with solving
  ui->submitBtn->setEnabled(false);
  ui->solveBtn->setEnabled(false);
  ui->newGameBtn->setEnabled(false);
  ui->settingsBtn->setEnabled(false);
  ui->resultsTabWidget->setEnabled(false);

  // Create and start solver thread with cancellation flag
  solverThread = new SolverThread(config, &cancellationRequested);
  connect(solverThread, &QThread::finished, this,
          &DungleonWidget::onSolverFinished);
  solverThread->start();
}

void DungleonWidget::populateResultTable(
    QTableWidget *table, const std::vector<Dungleon::PatternGuess> &guesses,
    int maxRows, int startRank) {
  table->setRowCount(0);

  int numRows = std::min(maxRows, static_cast<int>(guesses.size()));
  table->setRowCount(numRows);

  // Set row height to accommodate images
  for (int i = 0; i < numRows; ++i) {
    table->setRowHeight(i, 50);
  }

  for (int i = 0; i < numRows; ++i) {
    const Dungleon::PatternGuess &guess = guesses[i];

    // Rank
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(startRank + i));
    rankItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 0, rankItem);

    // Pattern - create a widget with images
    QWidget *patternWidget = new QWidget();
    QHBoxLayout *patternLayout = new QHBoxLayout(patternWidget);
    patternLayout->setSpacing(2);
    patternLayout->setContentsMargins(2, 2, 2, 2);

    for (int j = 0; j < 5; ++j) {
      uint8_t charId = guess.pattern.characters[j];
      QLabel *charLabel = new QLabel();
      charLabel->setFixedSize(40, 40);
      charLabel->setAlignment(Qt::AlignCenter);

      // Load character icon
      std::string name = Dungleon::CHARACTER_NAMES[charId];
      std::transform(name.begin(), name.end(), name.begin(),
                     [](unsigned char c) {
                       if (c == ' ')
                         return '_';
                       return static_cast<char>(std::tolower(c));
                     });
      QString iconPath = QString("resources/dungleon/%1.png")
                             .arg(QString::fromStdString(name));
      QPixmap icon(iconPath);

      if (!icon.isNull()) {
        charLabel->setPixmap(
            icon.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      } else {
        // Fallback to text
        charLabel->setText(
            QString::fromStdString(Dungleon::CHARACTER_IDS[charId]));
        charLabel->setStyleSheet("QLabel { background-color: #d3d6da; border: "
                                 "1px solid #878a8c; border-radius: 3px; }");
      }

      patternLayout->addWidget(charLabel);
    }

    patternLayout->addStretch();
    patternWidget->setLayout(patternLayout);
    table->setCellWidget(i, 1, patternWidget);

    // ENT
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 2, entItem);

    // Probability
    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    table->setItem(i, 3, probItem);
  }
}

void DungleonWidget::onSolverFinished() {
  // Re-enable UI elements
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
    Dungleon::Result result =
        static_cast<SolverThread *>(solverThread)->getResult();

    if (result.sortedGuesses.empty()) {
      QMessageBox::information(
          this, "No Solutions",
          "No valid patterns found matching the given feedback.");
      return;
    }

    // Split into all guesses and probable patterns
    std::vector<Dungleon::PatternGuess> allGuesses;
    std::vector<Dungleon::PatternGuess> probablePatterns;

    for (const auto &guess : result.sortedGuesses) {
      allGuesses.push_back(guess);
      if (guess.probability > 0.0) {
        probablePatterns.push_back(guess);
      }
    }

    // Store results for table click handling
    lastAllResults = allGuesses;
    lastProbableResults = probablePatterns;

    // Populate tables
    populateResultTable(allResultsTable, allGuesses, 100);
    populateResultTable(probablePatternsTable, probablePatterns, 100);

    // Update tab labels
    ui->resultsTabWidget->setTabText(
        0, QString("All Suggestions (%1)").arg(allGuesses.size()));
    ui->resultsTabWidget->setTabText(
        1,
        QString("Possible Solutions (%1)").arg(result.totalPossiblePatterns));

    // Switch to appropriate tab
    if (!probablePatterns.empty()) {
      ui->resultsTabWidget->setCurrentIndex(1);
    } else {
      ui->resultsTabWidget->setCurrentIndex(0);
    }
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Error",
                          QString("Solver error: %1").arg(e.what()));
  }
}

#endif // WITH_GUI
