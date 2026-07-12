#pragma once

#ifdef WITH_GUI

#include "dungleon/dungleon.hpp"
#include "gui/GameWidget.hpp"

#include <QLabel>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <array>
#include <vector>

namespace Ui {
class DungleonWidget;
}

// Represents one character slot in a pattern
class CharacterSlot : public QLabel {
  Q_OBJECT
public:
  explicit CharacterSlot(QWidget *parent = nullptr);

  void setCharacter(int charId);
  void setColor(int color);          // 0-4 for Dungleon feedback
  void setShowBackground(bool show); // Control whether to show background color
  void clear();
  int getCharacter() const { return m_characterId; }
  int getColor() const { return m_color; }

signals:
  void clicked();
  void rightClicked();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

private:
  void updateStyle();

  int m_characterId; // -1 = empty
  int m_color;       // 0-4: not present, diff pos no more, correct pos no more,
                     // diff pos one more, correct pos one more
  bool m_showBackground; // Whether to show background color based on feedback
};

// Represents one submitted pattern guess with feedback
class PatternRow : public QWidget {
  Q_OBJECT
public:
  explicit PatternRow(const Dungleon::Feedback &feedback,
                      QWidget *parent = nullptr);

  Dungleon::Feedback getFeedback() const;
  void setEditable(bool editable);

signals:
  void deleteRequested();
  void editRequested();

private slots:
  void onSlotClicked();
  void onDeleteClicked();

private:
  std::array<CharacterSlot *, 5> characterSlots;
  QPushButton *deleteBtn;
  bool isEditable;
};

// Represents one past solution (pattern without feedback) for Gauntlet mode
class SolutionRow : public QWidget {
  Q_OBJECT
public:
  explicit SolutionRow(const Dungleon::Pattern &pattern,
                       QWidget *parent = nullptr);

  Dungleon::Pattern getPattern() const;

signals:
  void deleteRequested();

private slots:
  void onDeleteClicked();

private:
  std::array<CharacterSlot *, 5> characterSlots;
  QPushButton *deleteBtn;
};

class DungleonWidget : public GameWidget {
  Q_OBJECT
public:
  explicit DungleonWidget(QWidget *parent = nullptr);
  ~DungleonWidget();

public slots:
  void newGame() override;

protected:
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void onCharacterBankClicked(int charId);
  void onSubmit();
  void onAddOnly();
  void onSubmitSolution();
  void onNewGame() override;
  void onSettings() override;
  void onSolverFinished() override;
  void onHint();
  void onPatternDeleted();
  void onSolutionDeleted();
  void onTableRowClicked(int row, int column);

private:
  bool showConfigDialog();
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;
  void setupCurrentPattern();
  bool submitCurrentPattern();
  void submitCurrentSolution();
  void rebuildFeedbackHistory();
  void rebuildSolutionHistory();
  void solveDungleon();
  // Populate both results tables (All Suggestions and Possible Solutions).
  // maxRows limits the number of rows shown in each table.
  void populateResults(int maxRows = 1000);

  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const Dungleon::Config &cfg, std::atomic<bool> *cancelFlag)
        : config(cfg), cancellationFlag(cancelFlag) {}

    Dungleon::Result getResult() const { return result; }

  protected:
    void run() override {
      result = Dungleon::runDungleonSolver(config, cancellationFlag);
    }

  private:
    Dungleon::Config config;
    Dungleon::Result result;
    std::atomic<bool> *cancellationFlag;
  };

  // UI pointer (generated from .ui)
  Ui::DungleonWidget *ui = nullptr;

  // Character bank buttons (20 characters)
  QWidget *m_bankContainer;
  std::array<QPushButton *, 20> m_bankButtons;

  // Current pattern being built (5 slots)
  QWidget *currentPatternWidget;
  std::array<CharacterSlot *, 5> currentSlots;
  int currentSlotIndex; // Next slot to fill
  // Container for the current input row (separate from submitted patterns)
  QWidget *currentInputWidget;
  QHBoxLayout *currentInputLayout;
  QPushButton *currentBackspaceBtn;

  // Submitted patterns with feedback
  QWidget *patternListWidget;
  QVBoxLayout *patternListLayout;
  QScrollArea *patternListScrollArea;
  std::vector<PatternRow *> patternRows;

  // Past solutions (Gauntlet mode)
  QWidget *solutionListWidget;
  QVBoxLayout *solutionListLayout;
  QScrollArea *solutionListScrollArea;
  std::vector<SolutionRow *> solutionRows;

  // Result tables
  QTableWidget *allResultsTable;
  QTableWidget *probablePatternsTable;

  // Config
  Dungleon::Config config;
  QLabel *configInfoLabel;

  // Store last results for table click handling
  std::vector<Dungleon::PatternGuess> lastAllResults;
  std::vector<Dungleon::PatternGuess> lastProbableResults;
};

#endif // WITH_GUI
