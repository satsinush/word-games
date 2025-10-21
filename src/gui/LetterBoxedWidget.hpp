#ifdef WITH_GUI

#pragma once

#include "letterBoxed/letterBoxed.hpp"
#include "utils/wordUtils.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QTableWidget>
#include <QThread>
#include <QWidget>
#include <array>
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

class LetterBoxedWidget : public QWidget {
  Q_OBJECT

public:
  explicit LetterBoxedWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~LetterBoxedWidget();

public slots:
  void newGame();

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame();
  void onSolve();
  void onSettings();
  void onSolverFinished();

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const LetterBoxed::Config &cfg,
                 const std::vector<Utils::Word> &words)
        : config(cfg), wordVec(words) {}

    std::vector<LetterBoxed::Solution> getResult() const { return solutions; }

  protected:
    void run() override {
      solutions = LetterBoxed::runLetterBoxedSolver(config, wordVec);
    }

  private:
    LetterBoxed::Config config;
    const std::vector<Utils::Word> &wordVec;
    std::vector<LetterBoxed::Solution> solutions;
  };

private:
  Ui::LetterBoxedWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  LetterBoxed::Config config;
  std::vector<LetterBoxed::Solution> solutions;
  bool gameInitialized;
  int currentPreset; // Track selected preset: 1=Default, 2=Fast, 3=Thorough,
                     // 0=Custom

  QLabel *configInfoLabel;
  QTableWidget *resultsTable;
  LetterBoxDisplay *boxDisplay;
  QProgressDialog *progressDialog;
  SolverThread *solverThread;

  bool showConfigDialog();
  void initGame();
  void setUIEnabled(bool enabled);
  void updateConfigInfo();
  void populateResultTable();
  void createLetterBox();
  void updateLetterBoxFromInput(const QString &text);
};

#endif // WITH_GUI
