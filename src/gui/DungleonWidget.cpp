#ifdef WITH_GUI

#include "gui/DungleonWidget.hpp"

#include "dungleon/dungleon.hpp"
#include "ui_DungleonWidget.h"
#include "utils/utils.hpp"
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
    : QLabel(parent), m_characterId(-1), m_color(0), m_showBackground(true) {
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
    setText(label); // Set text as fallback

    // Try to load icon
    std::string name = Dungleon::CHARACTER_NAMES[charId];
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
      if (c == ' ')
        return '_';
      return static_cast<char>(std::tolower(c));
    });
    QString path = QString::fromStdString(
        Utils::getResourceFile("dungleon/" + name + ".png"));
    QPixmap icon(path);
    if (!icon.isNull()) {
      // FIX: Get/create a child label for the icon
      QLabel *iconLabel = findChild<QLabel *>("iconLabel");
      if (!iconLabel) {
        iconLabel = new QLabel(this);
        iconLabel->setObjectName("iconLabel");
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        iconLabel->setGeometry(0, 0, width(), height()); // Fill parent
      }
      // FIX: Set pixmap on the *child* label, not the parent
      iconLabel->setPixmap(
          icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      iconLabel->show();
      setText(""); // Clear parent's text
    } else {
      // No icon, just use text (already set)
      QLabel *iconLabel = findChild<QLabel *>("iconLabel");
      if (iconLabel)
        iconLabel->hide();  // Hide icon label if it exists
      setPixmap(QPixmap()); // Clear parent pixmap
    }
  } else {
    setText("");
    setPixmap(QPixmap()); // Clear parent's pixmap

    // FIX: Clear and hide icon label
    QLabel *iconLabel = findChild<QLabel *>("iconLabel");
    if (iconLabel) {
      iconLabel->setPixmap(QPixmap());
      iconLabel->hide();
    }
  }
  // Update badge visibility according to current color
  QLabel *badge = findChild<QLabel *>("plusBadge");
  if (badge) {
    if (m_color == 3 || m_color == 4)
      badge->show();
    else
      badge->hide();
    badge->raise(); // This will now work, raising it above the icon label
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
        badge = new QLabel(this); // No text initially
        badge->setObjectName("plusBadge");
        badge->setFixedSize(14, 14);
        badge->setAlignment(Qt::AlignCenter);

        // Load pixmap
        QPixmap plusIcon(QString::fromStdString(
            Utils::getResourceFile("dungleon/plus.png")));
        if (!plusIcon.isNull()) {
          badge->setPixmap(plusIcon.scaled(14, 14, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation));
          badge->setStyleSheet("QLabel { background-color: transparent; }");
        } else {
          // Fallback if image fails to load
          badge->setText("+");
          badge->setStyleSheet(
              "QLabel { background: rgba(0,0,0,150); color: white; "
              "font-weight: bold; font-size: 12px; border-radius: 8px; "
              "padding: 0px; }");
        }

        // Common properties
        badge->move(width() - badge->width() - 4, 4);
        badge->setAttribute(Qt::WA_TransparentForMouseEvents);
        badge->show();
      } else {
        badge->show();
      }
      badge->raise(); // Raise badge above icon label
    } else {
      if (badge)
        badge->hide();
    }
  }
}

void CharacterSlot::setShowBackground(bool show) {
  m_showBackground = show;
  updateStyle();
}

void CharacterSlot::clear() {
  m_characterId = -1;
  m_color = 0;
  setText("");
  setPixmap(QPixmap());

  // FIX: Remove icon label if it exists
  QLabel *iconLabel = findChild<QLabel *>("iconLabel");
  if (iconLabel) {
    iconLabel->deleteLater();
  }

  // Remove badge if it exists
  QLabel *badge = findChild<QLabel *>("plusBadge");
  if (badge) {
    badge->deleteLater();
  }

  updateStyle();
}

void CharacterSlot::updateStyle() {
  QString bgColor, textColor, border;

  if (!m_showBackground) {
    // No background color - just show character with neutral styling
    bgColor = "transparent";
    border = "#878a8c";
    textColor = "#000";
  } else {
    // Dungleon colors: 0=not present, 1=diff pos no more, 2=correct pos no
    // more, 3=diff pos one more, 4=correct pos one more
    if (m_color == 0) {
      bgColor = "#cd4848";
      textColor = "white";
      border = "#cd4848";
    } else if (m_color == 1 || m_color == 3) {
      bgColor = "#c9b458";
      textColor = "white";
      border = "#c9b458";
    } else if (m_color == 2 || m_color == 4) {
      bgColor = "#6aaa64";
      textColor = "white";
      border = "#6aaa64";
    } else {
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
  }

  // This stylesheet now applies to the parent slot, which acts
  // as the background/border.
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

void CharacterSlot::resizeEvent(QResizeEvent *event) {
  QLabel::resizeEvent(event);

  // FIX: Reposition icon label to fill parent
  QLabel *iconLabel = findChild<QLabel *>("iconLabel");
  if (iconLabel) {
    iconLabel->setGeometry(0, 0, width(), height());
  }

  // Reposition badge (top-right) if present
  QLabel *badge = findChild<QLabel *>("plusBadge");
  if (badge) {
    // move a few pixels from the top-right corner
    badge->move(width() - badge->width() - 4, 4);
    badge->raise();
  }
}

void CharacterSlot::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && m_characterId >= 0) {
    // Cycle through colors
    m_color = (m_color + 1) % 5;
    // FIX: Must call setColor to update badge, not just updateStyle
    setColor(m_color);
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

// ============ SolutionRow Implementation ============

SolutionRow::SolutionRow(const Dungleon::Pattern &pattern, QWidget *parent)
    : QWidget(parent) {
  QHBoxLayout *layout = new QHBoxLayout(this);
  layout->setSpacing(5);
  layout->setContentsMargins(0, 5, 0, 5);

  // Create character slots (show characters but no background color)
  for (int i = 0; i < 5; ++i) {
    CharacterSlot *slot = new CharacterSlot(this);
    slot->setCharacter(pattern.characters[i]);
    slot->setShowBackground(false); // No background color for past solutions
    // Don't use setEnabled(false) as it greys out the icons
    // Instead, block mouse events to prevent interaction
    slot->setAttribute(Qt::WA_TransparentForMouseEvents);
    slot->setCursor(Qt::ArrowCursor); // Change cursor to indicate not clickable
    characterSlots[i] = slot;
    layout->addWidget(slot);
  }

  layout->addSpacing(10);

  // Delete button
  deleteBtn = new QPushButton("✕", this);
  deleteBtn->setFixedSize(30, 30);
  deleteBtn->setToolTip("Delete this solution");
  deleteBtn->setStyleSheet("QPushButton { background-color: #dc3545; color: "
                           "white; border: none; border-radius: 4px; }");
  connect(deleteBtn, &QPushButton::clicked, this,
          &SolutionRow::onDeleteClicked);
  layout->addWidget(deleteBtn);

  layout->addStretch();
}

Dungleon::Pattern SolutionRow::getPattern() const {
  Dungleon::Pattern pattern;
  for (int i = 0; i < 5; ++i) {
    pattern.characters[i] = characterSlots[i]->getCharacter();
  }
  pattern.computeCharacterCount();
  return pattern;
}

void SolutionRow::onDeleteClicked() { emit deleteRequested(); }

// ============ DungleonWidget Implementation ============

DungleonWidget::DungleonWidget(QWidget *parent)
    : GameWidget(parent), currentPatternWidget(nullptr),
      currentBackspaceBtn(nullptr), currentSlotIndex(0) {
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
    QString path = QString::fromStdString(
        Utils::getResourceFile("dungleon/" + name + ".png"));
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

  // Setup pattern list container (scroll area from UI)
  patternListScrollArea = ui->patternListScrollArea;
  patternListWidget = new QWidget();
  patternListLayout = new QVBoxLayout(patternListWidget);
  patternListLayout->setSpacing(2);
  patternListLayout->setContentsMargins(0, 0, 0, 0);
  patternListLayout->setAlignment(Qt::AlignHCenter);
  patternListLayout->addStretch();
  patternListScrollArea->setWidget(patternListWidget);

  // Create container for current input row (separate from submitted patterns)
  currentInputWidget = new QWidget(this);
  currentInputLayout = new QHBoxLayout(currentInputWidget);
  currentInputLayout->setSpacing(5);
  currentInputLayout->setContentsMargins(0, 5, 0, 5);

  // If the UI file provides a `currentInputContainer` (added to the .ui),
  // add the current input widget there so the slots appear to the right of
  // the bank.
  QWidget *container = ui->currentInputContainer;
  if (container) {
    // Ensure the container has a layout we can add into.
    QLayout *existing = container->layout();
    if (!existing) {
      QHBoxLayout *containerLayout = new QHBoxLayout(container);
      containerLayout->setSpacing(6);
      containerLayout->setContentsMargins(0, 0, 0, 0);
      container->setLayout(containerLayout);
    }
    container->layout()->addWidget(currentInputWidget);
  } else {
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout *>(layout());
    if (mainLayout) {
      int submitIdx = mainLayout->indexOf(ui->submitBtn);
      mainLayout->insertWidget(submitIdx, currentInputWidget);
    }
  }

  // Setup solution list container (scroll area from UI)
  solutionListScrollArea = ui->solutionListScrollArea;
  solutionListWidget = new QWidget();
  solutionListLayout = new QVBoxLayout(solutionListWidget);
  solutionListLayout->setSpacing(2);
  solutionListLayout->setContentsMargins(0, 0, 0, 0);
  solutionListLayout->setAlignment(Qt::AlignHCenter);
  solutionListLayout->addStretch();
  solutionListScrollArea->setWidget(solutionListWidget);

  // Connect signals
  connect(ui->submitBtn, &QPushButton::clicked, this,
          &DungleonWidget::onSubmit);
  connect(ui->submitSolutionBtn, &QPushButton::clicked, this,
          &DungleonWidget::onSubmitSolution);
  connect(ui->newGameBtn, &QPushButton::clicked, this,
          &DungleonWidget::onNewGame);
  connect(ui->solveBtn, &QPushButton::clicked, this, &DungleonWidget::onHint);
  connect(ui->settingsBtn, &QPushButton::clicked, this,
          &DungleonWidget::onSettings);

  // Get result tables from UI and configure them
  allResultsTable = ui->allResultsTable;
  allResultsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  allResultsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");
  connect(allResultsTable, &QTableWidget::cellClicked, this,
          &DungleonWidget::onTableRowClicked);

  probablePatternsTable = ui->probablePatternsTable;
  probablePatternsTable->horizontalHeader()->setSectionResizeMode(
      QHeaderView::Stretch);
  probablePatternsTable->setStyleSheet(
      "QTableWidget::item:hover { background-color: none; }");
  connect(probablePatternsTable, &QTableWidget::cellClicked, this,
          &DungleonWidget::onTableRowClicked);

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
  dialog.setMinimumWidth(300);

  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QFormLayout *formLayout = new QFormLayout();

  // Max Depth
  QSpinBox *maxDepthSpinner = new QSpinBox(&dialog);
  maxDepthSpinner->setRange(0, 2);
  maxDepthSpinner->setValue(config.maxDepth);
  formLayout->addRow("Search Depth:", maxDepthSpinner);

  // Exclude Impossible Patterns
  QCheckBox *excludeImpossibleCheckbox = new QCheckBox(&dialog);
  excludeImpossibleCheckbox->setChecked(config.excludeImpossiblePatterns);
  formLayout->addRow("Exclude Impossible Patterns:", excludeImpossibleCheckbox);

  layout->addLayout(formLayout);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
  connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
  layout->addWidget(buttonBox);

  if (dialog.exec() == QDialog::Accepted) {
    config.maxDepth = static_cast<uint8_t>(maxDepthSpinner->value());
    config.excludeImpossiblePatterns = excludeImpossibleCheckbox->isChecked();
    return true;
  }
  return false;
}

void DungleonWidget::initGame() {
  // Clear feedback history
  config.feedbackHistory.clear();
  config.solutionHistory.clear();

  // Delete all PatternRow widgets
  for (PatternRow *row : patternRows) {
    patternListLayout->removeWidget(row);
    row->deleteLater();
  }
  patternRows.clear();

  // Delete all SolutionRow widgets
  for (SolutionRow *row : solutionRows) {
    solutionListLayout->removeWidget(row);
    row->deleteLater();
  }
  solutionRows.clear();

  // Remove current pattern widget if it exists
  if (currentPatternWidget) {
    // Remove from layout and schedule for deletion
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
  QString info =
      QString("<span style='color:#666; font-size:11pt;'>Search "
              "Depth: %1 | %2 | Guesses: %3 | Past Solutions: %4</span>")
          .arg(config.maxDepth)
          .arg(config.excludeImpossiblePatterns ? "Possible Patterns"
                                                : "All Patterns")
          .arg(config.feedbackHistory.size())
          .arg(config.solutionHistory.size());
  configInfoLabel->setText(info);
}

void DungleonWidget::setupCurrentPattern() {
  // Remove previous input if present
  if (currentPatternWidget) {
    // Remove from layout and schedule for deletion
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
      "QPushButton { background-color: #505050; border: 1px solid #111;"
      " border-radius: 4px; color: #f0f0f0; }");
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

void DungleonWidget::onSubmitSolution() { submitCurrentSolution(); }

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
    fb.setColor(i, currentSlots[i]->getColor());
  }
  fb.pattern.computeCharacterCount();

  config.feedbackHistory.push_back(fb);

  // Remove current pattern from input layout and schedule for deletion
  if (currentInputLayout)
    currentInputLayout->removeWidget(currentPatternWidget);
  currentPatternWidget->deleteLater();
  currentPatternWidget = nullptr;

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

void DungleonWidget::submitCurrentSolution() {
  // Check if all 5 slots are filled
  if (currentSlotIndex < 5) {
    QMessageBox::warning(
        this, "Incomplete Pattern",
        "Please fill all 5 character slots before submitting.");
    return;
  }

  // Create pattern from current slots
  Dungleon::Pattern pattern;
  for (int i = 0; i < 5; ++i) {
    pattern.characters[i] = currentSlots[i]->getCharacter();
  }
  pattern.computeCharacterCount();

  config.solutionHistory.push_back(pattern);

  // Remove current pattern from input layout and schedule for deletion
  if (currentInputLayout)
    currentInputLayout->removeWidget(currentPatternWidget);
  currentPatternWidget->deleteLater();
  currentPatternWidget = nullptr;

  // Create a SolutionRow for this pattern
  SolutionRow *solutionRow = new SolutionRow(pattern, solutionListWidget);
  connect(solutionRow, &SolutionRow::deleteRequested, this,
          &DungleonWidget::onSolutionDeleted);

  solutionRows.push_back(solutionRow);

  // Add to layout before stretch
  solutionListLayout->insertWidget(solutionListLayout->count() - 1,
                                   solutionRow);

  // Setup new current pattern
  setupCurrentPattern();
}

void DungleonWidget::rebuildFeedbackHistory() {
  config.feedbackHistory.clear();
  for (PatternRow *row : patternRows) {
    config.feedbackHistory.push_back(row->getFeedback());
  }
}

void DungleonWidget::rebuildSolutionHistory() {
  config.solutionHistory.clear();
  for (SolutionRow *row : solutionRows) {
    config.solutionHistory.push_back(row->getPattern());
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

void DungleonWidget::onSolutionDeleted() {
  SolutionRow *row = qobject_cast<SolutionRow *>(sender());
  if (row) {
    auto it = std::find(solutionRows.begin(), solutionRows.end(), row);
    if (it != solutionRows.end()) {
      solutionRows.erase(it);
    }
    solutionListLayout->removeWidget(row);
    row->deleteLater();
    rebuildSolutionHistory();
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

  // Fill current input with the pattern from the table (so the user sees it)
  for (int i = 0; i < 5; ++i) {
    currentSlots[i]->setCharacter(pattern.characters[i]);
    currentSlots[i]->setColor(0);
  }
  currentSlotIndex = 5;

  // Create a feedback object from the clicked pattern (default color = 0)
  Dungleon::Feedback fb;
  for (int i = 0; i < 5; ++i) {
    fb.pattern.characters[i] = pattern.characters[i];
    fb.setColor(i, 0);
  }
  fb.pattern.computeCharacterCount();

  // Append to config feedback history and UI as a new PatternRow
  config.feedbackHistory.push_back(fb);

  PatternRow *patternRow = new PatternRow(fb, patternListWidget);
  patternRow->setEditable(true);
  connect(patternRow, &PatternRow::deleteRequested, this,
          &DungleonWidget::onPatternDeleted);
  connect(patternRow, &PatternRow::editRequested, this,
          [this]() { rebuildFeedbackHistory(); });

  patternRows.push_back(patternRow);
  // Insert before the stretch at the end
  patternListLayout->insertWidget(patternListLayout->count() - 1, patternRow);

  updateConfigInfo();

  // Reset current input to a fresh empty row
  setupCurrentPattern();
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

// Populate both results tables in one call. This ensures the "Possible
// Solutions" table shows the actual rank of a pattern as it appears in the
// full sorted list (lastAllResults). maxRows limits rows shown per table.
void DungleonWidget::populateResults(int maxRows) {
  // --- All suggestions table ---
  const std::vector<Dungleon::PatternGuess> &all = lastAllResults;
  int allRows = std::min(maxRows, static_cast<int>(all.size()));
  allResultsTable->setRowCount(allRows);
  for (int i = 0; i < allRows; ++i) {
    allResultsTable->setRowHeight(i, 50);

    const Dungleon::PatternGuess &guess = all[i];

    // Rank (1-based)
    QTableWidgetItem *rankItem = new QTableWidgetItem(QString::number(i + 1));
    rankItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 0, rankItem);

    // Pattern widget (icons or fallback text)
    QWidget *patternWidget = new QWidget();
    QHBoxLayout *patternLayout = new QHBoxLayout(patternWidget);
    patternLayout->setSpacing(2);
    patternLayout->setContentsMargins(2, 2, 2, 2);
    for (int j = 0; j < 5; ++j) {
      uint8_t charId = guess.pattern.characters[j];
      QLabel *charLabel = new QLabel();
      charLabel->setFixedSize(40, 40);
      charLabel->setAlignment(Qt::AlignCenter);

      std::string name = Dungleon::CHARACTER_NAMES[charId];
      std::transform(name.begin(), name.end(), name.begin(),
                     [](unsigned char c) {
                       if (c == ' ')
                         return '_';
                       return static_cast<char>(std::tolower(c));
                     });
      QString iconPath = QString::fromStdString(
          Utils::getResourceFile("dungleon/" + name + ".png"));
      QPixmap icon(iconPath);
      if (!icon.isNull()) {
        charLabel->setPixmap(
            icon.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      } else {
        charLabel->setText(
            QString::fromStdString(Dungleon::CHARACTER_IDS[charId]));
        charLabel->setStyleSheet("QLabel { background-color: #d3d6da; border: "
                                 "1px solid #878a8c; border-radius: 3px; }");
      }
      patternLayout->addWidget(charLabel);
    }
    patternLayout->addStretch();
    patternWidget->setLayout(patternLayout);
    allResultsTable->setCellWidget(i, 1, patternWidget);

    // ENT
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 2, entItem);

    // Probability
    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    allResultsTable->setItem(i, 3, probItem);

    // Color coding
    if (guess.probability >= 1.0) {
      QColor bgColor(144, 238, 144);
      for (int col = 0; col < 4; ++col) {
        if (allResultsTable->item(i, col)) {
          allResultsTable->item(i, col)->setBackground(bgColor);
          allResultsTable->item(i, col)->setForeground(Qt::black);
        }
      }
      patternWidget->setStyleSheet(
          QString("QWidget { background-color: %1; }").arg(bgColor.name()));
    } else if (guess.probability > 0.0) {
      QColor bgColor(255, 255, 153);
      for (int col = 0; col < 4; ++col) {
        if (allResultsTable->item(i, col)) {
          allResultsTable->item(i, col)->setBackground(bgColor);
          allResultsTable->item(i, col)->setForeground(Qt::black);
        }
      }
      patternWidget->setStyleSheet(
          QString("QWidget { background-color: %1; }").arg(bgColor.name()));
    }
  }

  // --- Possible solutions table ---
  // Build rows from the full list but only include entries with non-zero
  // probability, preserving their original rank from the full list.
  std::vector<std::pair<int, Dungleon::PatternGuess>> probable;
  probable.reserve(all.size());
  for (int i = 0; i < static_cast<int>(all.size()); ++i) {
    if (all[i].probability > 0.0) {
      probable.emplace_back(i + 1, all[i]); // store original rank (1-based)
    }
  }

  int probRows = std::min(maxRows, static_cast<int>(probable.size()));
  probablePatternsTable->setRowCount(probRows);
  for (int r = 0; r < probRows; ++r) {
    probablePatternsTable->setRowHeight(r, 50);
    int originalRank = probable[r].first;
    const Dungleon::PatternGuess &guess = probable[r].second;

    // Rank column shows the original rank from the full list
    QTableWidgetItem *rankItem =
        new QTableWidgetItem(QString::number(originalRank));
    rankItem->setTextAlignment(Qt::AlignCenter);
    probablePatternsTable->setItem(r, 0, rankItem);

    // Pattern widget
    QWidget *patternWidget = new QWidget();
    QHBoxLayout *patternLayout = new QHBoxLayout(patternWidget);
    patternLayout->setSpacing(2);
    patternLayout->setContentsMargins(2, 2, 2, 2);
    for (int j = 0; j < 5; ++j) {
      uint8_t charId = guess.pattern.characters[j];
      QLabel *charLabel = new QLabel();
      charLabel->setFixedSize(40, 40);
      charLabel->setAlignment(Qt::AlignCenter);

      std::string name = Dungleon::CHARACTER_NAMES[charId];
      std::transform(name.begin(), name.end(), name.begin(),
                     [](unsigned char c) {
                       if (c == ' ')
                         return '_';
                       return static_cast<char>(std::tolower(c));
                     });
      QString iconPath = QString::fromStdString(
          Utils::getResourceFile("dungleon/" + name + ".png"));
      QPixmap icon(iconPath);
      if (!icon.isNull()) {
        charLabel->setPixmap(
            icon.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
      } else {
        charLabel->setText(
            QString::fromStdString(Dungleon::CHARACTER_IDS[charId]));
        charLabel->setStyleSheet("QLabel { background-color: #d3d6da; border: "
                                 "1px solid #878a8c; border-radius: 3px; }");
      }
      patternLayout->addWidget(charLabel);
    }
    patternLayout->addStretch();
    patternWidget->setLayout(patternLayout);
    probablePatternsTable->setCellWidget(r, 1, patternWidget);

    // ENT
    QTableWidgetItem *entItem =
        new QTableWidgetItem(QString::number(guess.ent, 'f', 3));
    entItem->setTextAlignment(Qt::AlignCenter);
    probablePatternsTable->setItem(r, 2, entItem);

    // Probability
    QTableWidgetItem *probItem = new QTableWidgetItem(
        QString::number(guess.probability * 100.0, 'f', 2) + "%");
    probItem->setTextAlignment(Qt::AlignCenter);
    probablePatternsTable->setItem(r, 3, probItem);

    // Color coding
    if (guess.probability >= 1.0) {
      QColor bgColor(144, 238, 144);
      for (int col = 0; col < 4; ++col) {
        if (probablePatternsTable->item(r, col)) {
          probablePatternsTable->item(r, col)->setBackground(bgColor);
          probablePatternsTable->item(r, col)->setForeground(Qt::black);
        }
      }
      patternWidget->setStyleSheet(
          QString("QWidget { background-color: %1; }").arg(bgColor.name()));
    } else {
      QColor bgColor(255, 255, 153);
      for (int col = 0; col < 4; ++col) {
        if (probablePatternsTable->item(r, col)) {
          probablePatternsTable->item(r, col)->setBackground(bgColor);
          probablePatternsTable->item(r, col)->setForeground(Qt::black);
        }
      }
      patternWidget->setStyleSheet(
          QString("QWidget { background-color: %1; }").arg(bgColor.name()));
    }
  }

  // Update cached probable results to match what is displayed (keeping
  // lastProbableResults usable elsewhere if needed)
  lastProbableResults.clear();
  lastProbableResults.reserve(probable.size());
  for (auto &p : probable)
    lastProbableResults.push_back(p.second);
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

    // Populate tables (single call will fill both tables and preserve
    // original ranking for possible patterns)
    populateResults(1000);

    // Update tab labels
    ui->resultsTabWidget->setTabText(
        0, QString("All Suggestions (%1)").arg(allGuesses.size()));
    ui->resultsTabWidget->setTabText(
        1,
        QString("Possible Solutions (%1)").arg(result.totalPossiblePatterns));
  } catch (const std::exception &e) {
    QMessageBox::critical(this, "Error",
                          QString("Solver error: %1").arg(e.what()));
  }
}

#endif // WITH_GUI
