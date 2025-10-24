#ifdef WITH_GUI

#pragma once

#include "gui/GameWidget.hpp"
#include "utils/wordUtils.hpp"
#include "wordle/wordle.hpp"
#include <QLabel>
#include <QProgressDialog>
#include <QPushButton>
#include <QScrollArea>
#include <QTabWidget>
#include <QTableWidget>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>
#include <array>
#include <vector>

namespace Ui {
class WordleWidget;
}

// Custom clickable letter box widget
class LetterBox : public QLabel {
  Q_OBJECT

public:
  explicit LetterBox(QWidget *parent = nullptr);

  void setLetter(char letter);
  char getLetter() const { return currentLetter; }

  void setColor(int color); // 0=grey, 1=yellow, 2=green
  int getColor() const { return currentColor; }

  void clear();

signals:
  void clicked();

protected:
  void mousePressEvent(QMouseEvent *event) override;

private:
  char currentLetter;
  int currentColor; // 0=grey, 1=yellow, 2=green
  void updateStyle();
};

// Widget representing one complete guess (variable-length boxes + delete
// button)
class GuessRow : public QWidget {
  Q_OBJECT

public:
  explicit GuessRow(const Wordle::Feedback &feedback,
                    QWidget *parent = nullptr);

  Wordle::Feedback getFeedback() const;
  void setEditable(bool editable);

signals:
  void deleteRequested();
  void editRequested();

private slots:
  void onLetterBoxClicked();
  void onDeleteClicked();

private:
  std::vector<LetterBox *> boxes;
  QPushButton *deleteBtn;
  QPushButton *editBtn;
  bool isEditable;
};

class WordleWidget : public GameWidget {
  Q_OBJECT

public:
  explicit WordleWidget(const std::vector<Utils::Word> &words,
                        QWidget *parent = nullptr);
  ~WordleWidget() override;

public slots:
  void newGame() override;

private slots:
  void onSubmit();
  void onNewGame() override;
  void onHint();
  void onLetterBoxClicked();
  void onInputChanged(const QString &text);
  void onGuessDeleted();
  void onTableRowClicked(int row, int column);
  void onSettings() override;
  void onSolverFinished() override;

private:
  // Worker thread for solving
  class SolverThread : public QThread {
  public:
    SolverThread(const std::vector<Utils::Word> &words,
                 const std::vector<Wordle::Feedback> &feedback,
                 const Wordle::Config &cfg, std::atomic<bool> *cancelFlag)
        : wordVec(words), feedbackHistory(feedback), config(cfg),
          cancellationFlag(cancelFlag) {}

    Wordle::Result getResult() const { return result; }

  protected:
    void run() override {
      result = Wordle::runWordleSolver(wordVec, feedbackHistory, config);
    }

  private:
    const std::vector<Utils::Word> &wordVec;
    std::vector<Wordle::Feedback> feedbackHistory;
    Wordle::Config config;
    Wordle::Result result;
    std::atomic<bool> *cancellationFlag;
  };

private:
  Ui::WordleWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  std::vector<Wordle::Feedback> feedbackHistory;
  Wordle::Config config;

  // Current guess row
  std::vector<LetterBox *> currentBoxes;
  QWidget *currentRowWidget;

  // Container for past guesses
  QScrollArea *guessListScrollArea;
  QWidget *guessListWidget;
  QVBoxLayout *guessListLayout;
  std::vector<GuessRow *> guessRows;

  // Result tables
  QTabWidget *resultsTabWidget;
  QTableWidget *allResultsTable;
  QTableWidget *probableWordsTable;

  bool showConfigDialog();
  void initGame() override;
  void setUIEnabled(bool enabled) override;
  void updateConfigInfo() override;
  void solveWordle();
  void setupCurrentRow();
  void submitCurrentGuess();
  void rebuildFeedbackHistory();
  void populateResultTable(QTableWidget *table,
                           const std::vector<Wordle::WordGuess> &guesses,
                           int maxRows, int startRank = 1);
};

#endif // WITH_GUI
