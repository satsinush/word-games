#ifdef WITH_GUI

#pragma once

#include "gui/GameWidget.hpp"
#include "mastermind/mastermind.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <atomic>
#include <vector>

namespace Ui {
class MastermindWidget;
}

// FeedbackRow: Display a submitted pattern with its feedback
class FeedbackRow : public QWidget {
  Q_OBJECT

public:
  explicit FeedbackRow(int index, const QString &pattern, int correctColors,
                       int correctPositions, int maxPegs,
                       QWidget *parent = nullptr);

  int getIndex() const { return rowIndex; }
  void setIndex(int index) { rowIndex = index; }
  int getCorrectColors() const { return correctColors; }
  int getCorrectPositions() const { return correctPositions; }
  void updateFeedback(int colors, int positions);

signals:
  void deleteRequested(int index);
  void feedbackChanged(int index);

private:
  int rowIndex;
  int correctColors;
  int correctPositions;
  int maxPegs;
  QLabel *patternLabel;
  QSpinBox *colorsSpinBox;
  QSpinBox *positionsSpinBox;
  QPushButton *deleteButton;
};

class MastermindWidget : public GameWidget {
  Q_OBJECT

public:
  explicit MastermindWidget(QWidget *parent = nullptr);
  ~MastermindWidget() override;

public slots:
  void newGame() override;

private slots:
  void onSubmit();
  void onNewGame() override;
  void onSolve();
  void onTableRowClicked(int row, int column);
  void onDeleteFeedback(int index);
  void onFeedbackChanged(int index);
  void onSettings() override;
  void onSolverFinished() override;

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const Mastermind::Config &cfg, std::atomic<bool> *cancelFlag)
        : config(cfg), cancellationFlag(cancelFlag) {}

    Mastermind::Result getResult() const { return result; }

  protected:
    void run() override {
      result = Mastermind::runMastermindSolver(config, cancellationFlag);
    }

  private:
    Mastermind::Config config;
    Mastermind::Result result;
    std::atomic<bool> *cancellationFlag;
  };

private:
  Ui::MastermindWidget *ui;

  Mastermind::Config config;

  QScrollArea *feedbackListScrollArea;
  QWidget *feedbackListContainer;
  QVBoxLayout *feedbackListLayout;

  // Result tables
  QTableWidget *allResultsTable;
  QTableWidget *possibleResultsTable;

  bool showConfigDialog();
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;
  void rebuildFeedbackList();
  bool submitPattern();
  void solveMastermind();
  // Populate both result tables (Suggested Guesses and Possible Patterns)
  void populateResults(int maxRows = 1000);

  // Cached results for table click handling
  std::vector<Mastermind::PatternGuess> lastAllResults;
  std::vector<Mastermind::PatternGuess> lastProbableResults;
};

#endif // WITH_GUI
