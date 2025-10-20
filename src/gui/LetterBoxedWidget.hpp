#ifdef WITH_GUI

#pragma once

#include "letterBoxed/letterBoxed.hpp"
#include "utils/wordUtils.hpp"
#include <QWidget>
#include <bitset>
#include <vector>

namespace Ui {
class LetterBoxedWidget;
}

class LetterBoxedWidget : public QWidget {
  Q_OBJECT

public:
  explicit LetterBoxedWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~LetterBoxedWidget();

public slots:
  void newGame();

private slots:
  void onSubmit();
  void onNewGame();
  void onSolve();

private:
  Ui::LetterBoxedWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  LetterBoxed::Config config;
  std::vector<LetterBoxed::Solution> solutions;
};

#endif // WITH_GUI
