#ifndef BOARDSELECTOR_H
#define BOARDSELECTOR_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QMap>

class QPushButton;
class QLabel;

class BoardSelector : public QDialog {
    Q_OBJECT

public:
    explicit BoardSelector(QWidget *parent = nullptr, const QString& initial_board = "");

    QString getSelectedBoard() const;

private slots:
    void onCardClicked(const QString& card_str);
    void onClearClicked();
    void onConfirmClicked();

private:
    void setupUI();
    void updateCardDisplay();

    QStringList selected_cards_;
    QMap<QString, QPushButton*> card_buttons_;
    QLabel *selectedCardsDisplay_;
    QLabel *statusLabel_;
    QPushButton *confirmButton_;
};

#endif // BOARDSELECTOR_H
