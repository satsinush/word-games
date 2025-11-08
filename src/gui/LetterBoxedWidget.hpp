#ifdef WITH_GUI

#pragma once

#include "gui/GameWidget.hpp"
#include "letterBoxed/letterBoxed.hpp"
#include "utils/utils.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QTableWidget>
#include <QThread>
#include <QWidget>
#include <array>
#include <atomic>
#include <bitset>
#include <vector>

namespace Ui {
class LetterBoxedWidget;
}

class LetterBoxDisplay : public QWidget {
  Q_OBJECT
public:
  explicit LetterBoxDisplay(QWidget *parent = nullptr);
  void setLetters(const std::array<char, 12> &letters);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  std::array<char, 12> currentLetters;
};

class LetterBoxedWidget : public GameWidget {
  Q_OBJECT

public:
  explicit LetterBoxedWidget(QWidget *parent = nullptr);
  ~LetterBoxedWidget() override;

public slots:
  void newGame() override;

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame() override;
  void onSolve();
  void onSettings() override;
  void onSolverFinished() override;

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    // Take the word vector by const-ref but store a copy internally so the
    // worker thread does not hold references to data owned by the GUI thread.
    SolverThread(const LetterBoxed::Config &cfg, std::atomic<bool> *cancelFlag)
        : config(cfg), cancellationFlag(cancelFlag) {}

    std::vector<LetterBoxed::Solution> getResult() const {
      return result.solutions;
    }

  protected:
    void run() override {
      // Call solver with our local copy and pass cancellation pointer so the
      // solver can stop cooperatively.
      result = LetterBoxed::runLetterBoxedSolver(config, cancellationFlag);
    }

  private:
    LetterBoxed::Config config;
    // Store a copy so the GUI may modify or destroy its result without
    // affecting the running worker.
    LetterBoxed::Result result;
    std::atomic<bool> *cancellationFlag;
  };

private:
  Ui::LetterBoxedWidget *ui;

  LetterBoxed::Config config;
  std::vector<LetterBoxed::Solution> solutions;
  int currentPreset; // Track selected preset: 1=Default, 2=Fast, 3=Thorough,
                     // 0=Custom

  QTableWidget *resultsTable;
  LetterBoxDisplay *boxDisplay;

  bool showConfigDialog();
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;
  void populateResults(int maxRows);
  void updateLetterBoxFromInput(const QString &text);
};

#endif // WITH_GUI
