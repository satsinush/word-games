#ifdef WITH_GUI

#pragma once

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

class MastermindWidget : public QWidget {
  Q_OBJECT

public:
  explicit MastermindWidget(QWidget *parent = nullptr);
  ~MastermindWidget();

public slots:
  void newGame();

private slots:
  void onSubmit();
  void onNewGame();
  void onSolve();
  void onTableRowClicked(int row, int column);
  void onDeleteFeedback(int index);
  void onFeedbackChanged(int index);
  void onSettings();
  void onSolverFinished();

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const std::vector<Mastermind::Pattern> &patterns,
                 const std::vector<Mastermind::Feedback> &feedback,
                 const Mastermind::Config &cfg)
        : allPatterns(patterns), feedbackHistory(feedback), config(cfg) {}

    Mastermind::Result getResult() const { return result; }

  protected:
    void run() override {
      result =
          Mastermind::runMastermindSolver(allPatterns, feedbackHistory, config);
    }

  private:
    std::vector<Mastermind::Pattern> allPatterns;
    std::vector<Mastermind::Feedback> feedbackHistory;
    Mastermind::Config config;
    Mastermind::Result result;
  };

private:
  Ui::MastermindWidget *ui;

  Mastermind::Config config;
  std::vector<Mastermind::Pattern> allPatterns;
  std::vector<Mastermind::Feedback> feedbackHistory;
  bool gameInitialized;

  QScrollArea *feedbackListScrollArea;
  QWidget *feedbackListContainer;
  QVBoxLayout *feedbackListLayout;
  QLabel *configInfoLabel;
  QProgressDialog *progressDialog;
  SolverThread *solverThread;

  bool showConfigDialog();
  void initGame();
  void setUIEnabled(bool enabled);
  void updateConfigInfo();
  void rebuildFeedbackList();
  void solveMastermind();
  void populateResultTable(QTableWidget *table,
                           const std::vector<Mastermind::PatternGuess> &guesses,
                           bool filterPossible);
};

#endif // WITH_GUI
