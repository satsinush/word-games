#ifdef WITH_GUI

#pragma once

#include "mastermind/mastermind.hpp"
#include <QWidget>
#include <vector>

namespace Ui {
class MastermindWidget;
}

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
  void onHint();

private:
  Ui::MastermindWidget *ui;

  Mastermind::Config config;
  std::vector<Mastermind::Pattern> allPatterns;
  std::vector<Mastermind::Feedback> feedbackHistory;

  void initGame();
  void solveMastermind();
};

#endif // WITH_GUI
