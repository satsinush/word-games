#ifdef WITH_GUI

#pragma once

#include "utils/wordUtils.hpp"
#include "wordle/wordle.hpp"
#include <QLabel>
#include <QPushButton>
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

// Widget representing one complete guess (5 boxes + delete button)
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
  std::array<LetterBox *, 5> boxes;
  QPushButton *deleteBtn;
  QPushButton *editBtn;
  bool isEditable;
};

class WordleWidget : public QWidget {
  Q_OBJECT

public:
  explicit WordleWidget(const std::vector<Utils::Word> &words,
                        QWidget *parent = nullptr);
  ~WordleWidget();

public slots:
  void newGame();

private slots:
  void onSubmit();
  void onNewGame();
  void onHint();
  void onLetterBoxClicked();
  void onInputChanged(const QString &text);
  void onGuessDeleted();

private:
  Ui::WordleWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  std::vector<Wordle::Feedback> feedbackHistory;
  Wordle::Config config;

  // Current guess row
  std::array<LetterBox *, 5> currentBoxes;
  QWidget *currentRowWidget;

  // Container for past guesses
  QWidget *guessListWidget;
  QVBoxLayout *guessListLayout;
  std::vector<GuessRow *> guessRows;

  void solveWordle();
  void setupCurrentRow();
  void submitCurrentGuess();
  void rebuildFeedbackHistory();
};

#endif // WITH_GUI
