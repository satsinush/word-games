#ifdef WITH_GUI

#pragma once

#include "spellingBee/spellingBee.hpp"
#include "utils/wordUtils.hpp"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>
#include <array>
#include <vector>

namespace Ui {
class SpellingBeeWidget;
}

class HexagonButton : public QPushButton {
  Q_OBJECT
public:
  explicit HexagonButton(bool isCenter = false, QWidget *parent = nullptr);
  void setLetter(char letter);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  bool isCenterHex;
  char currentLetter;
};

class SpellingBeeWidget : public QWidget {
  Q_OBJECT

public:
  explicit SpellingBeeWidget(const std::vector<Utils::Word> &words,
                             QWidget *parent = nullptr);
  ~SpellingBeeWidget();

public slots:
  void newGame();

private slots:
  void onInputSubmit();
  void onInputChanged(const QString &text);
  void onNewGame();
  void onSettings();

private:
  Ui::SpellingBeeWidget *ui;
  const std::vector<Utils::Word> &wordVec;

  SpellingBee::Config config;
  std::vector<Utils::Word> solutions;
  bool gameInitialized;

  QLabel *configInfoLabel;
  QTableWidget *resultsTable;
  QWidget *hexWidget;
  std::array<HexagonButton *, 7> hexButtons;

  bool showConfigDialog();
  void initGame();
  void setUIEnabled(bool enabled);
  void updateConfigInfo();
  void populateResultTable();
  void createHexagons();
  void updateHexagonsFromInput(const QString &text);
};

#endif // WITH_GUI
