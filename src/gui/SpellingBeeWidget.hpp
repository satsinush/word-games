#ifdef WITH_GUI

#pragma once

#include "spellingBee/spellingBee.hpp"
#include "utils/wordUtils.hpp"
#include <QWidget>
#include <vector>

namespace Ui {
class SpellingBeeWidget;
}

class SpellingBeeWidget : public QWidget {
  Q_OBJECT

public:
  explicit SpellingBeeWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~SpellingBeeWidget();

public slots:
  void newGame();

private slots:
  void onSubmit();
  void onNewGame();
  void onShuffle();

private:
  Ui::SpellingBeeWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  SpellingBee::Config config;
  std::vector<Utils::Word> solutions;
};

#endif // WITH_GUI
