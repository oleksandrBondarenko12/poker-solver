#include "board_card_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>

BoardSelector::BoardSelector(QWidget *parent, const QString& initial_board)
    : QDialog(parent) {
    setWindowTitle("Select Board Cards (Flop, Turn, River)");
    setModal(true);
    resize(720, 360);

    setupUI();

    // Parse initial board
    QStringList tokens = initial_board.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    for (const QString& tok : tokens) {
        if (tok.length() >= 2) {
            QString standard = tok.left(1).toUpper() + tok.mid(1, 1).toLower();
            if (card_buttons_.contains(standard) && !selected_cards_.contains(standard) && selected_cards_.size() < 5) {
                selected_cards_.append(standard);
            }
        }
    }
    updateCardDisplay();
}

void BoardSelector::setupUI() {
    setStyleSheet("background-color: #111827; color: #F9FAFB; font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header section
    QLabel *titleLabel = new QLabel("BOARD CARD SELECTOR");
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #60A5FA; letter-spacing: 1px;");
    mainLayout->addWidget(titleLabel);

    selectedCardsDisplay_ = new QLabel();
    selectedCardsDisplay_->setStyleSheet("background-color: #1F2937; border: 1px solid #374151; border-radius: 8px; padding: 10px; font-size: 15px; font-weight: bold;");
    mainLayout->addWidget(selectedCardsDisplay_);

    statusLabel_ = new QLabel();
    statusLabel_->setStyleSheet("color: #9CA3AF; font-size: 12px;");
    mainLayout->addWidget(statusLabel_);

    // Card Grid: 4 rows (suits: s, h, d, c) x 13 ranks (A, K, Q, J, T, 9, 8, 7, 6, 5, 4, 3, 2)
    const QStringList ranks = {"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};
    const QStringList suits = {"s", "h", "d", "c"};
    const QStringList suitSymbols = {"♠", "♥", "♦", "♣"};
    const QStringList suitColors = {"#F3F4F6", "#EF4444", "#38BDF8", "#34D399"};

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->setSpacing(6);

    for (int s = 0; s < 4; ++s) {
        for (int r = 0; r < 13; ++r) {
            QString cardStr = ranks[r] + suits[s];
            QString displayStr = ranks[r] + suitSymbols[s];

            QPushButton *btn = new QPushButton(displayStr);
            btn->setFixedSize(48, 38);
            btn->setCursor(Qt::PointingHandCursor);

            QString baseStyle = QString(
                "QPushButton {"
                "  background-color: #1F2937;"
                "  color: %1;"
                "  border: 1px solid #374151;"
                "  border-radius: 6px;"
                "  font-size: 13px;"
                "  font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "  background-color: #374151;"
                "}"
            ).arg(suitColors[s]);
            btn->setStyleSheet(baseStyle);

            connect(btn, &QPushButton::clicked, this, [this, cardStr]() {
                onCardClicked(cardStr);
            });

            card_buttons_[cardStr] = btn;
            gridLayout->addWidget(btn, s, r);
        }
    }
    mainLayout->addLayout(gridLayout);

    // Bottom action bar
    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(12);

    QPushButton *clearBtn = new QPushButton("Clear All");
    clearBtn->setStyleSheet("background-color: #374151; color: #E5E7EB; border: 1px solid #4B5563; border-radius: 6px; padding: 8px 16px; font-weight: bold;");
    connect(clearBtn, &QPushButton::clicked, this, &BoardSelector::onClearClicked);
    bottomLayout->addWidget(clearBtn);

    bottomLayout->addStretch();

    QPushButton *cancelBtn = new QPushButton("Cancel");
    cancelBtn->setStyleSheet("background-color: #374151; color: #9CA3AF; border: 1px solid #4B5563; border-radius: 6px; padding: 8px 16px; font-weight: bold;");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    bottomLayout->addWidget(cancelBtn);

    confirmButton_ = new QPushButton("Confirm Board");
    confirmButton_->setStyleSheet("background-color: #2563EB; color: #FFFFFF; border: 1px solid #3B82F6; border-radius: 6px; padding: 8px 24px; font-weight: bold;");
    connect(confirmButton_, &QPushButton::clicked, this, &BoardSelector::onConfirmClicked);
    bottomLayout->addWidget(confirmButton_);

    mainLayout->addLayout(bottomLayout);
}

void BoardSelector::onCardClicked(const QString& card_str) {
    if (selected_cards_.contains(card_str)) {
        selected_cards_.removeAll(card_str);
    } else {
        if (selected_cards_.size() >= 5) {
            return; // max 5 cards
        }
        selected_cards_.append(card_str);
    }
    updateCardDisplay();
}

void BoardSelector::onClearClicked() {
    selected_cards_.clear();
    updateCardDisplay();
}

void BoardSelector::onConfirmClicked() {
    if (selected_cards_.size() >= 3 && selected_cards_.size() <= 5) {
        accept();
    }
}

QString BoardSelector::getSelectedBoard() const {
    return selected_cards_.join(" ");
}

void BoardSelector::updateCardDisplay() {
    const QStringList suitSymbols = {"♠", "♥", "♦", "♣"};
    const QStringList suitChars = {"s", "h", "d", "c"};
    const QStringList suitColors = {"#F3F4F6", "#EF4444", "#38BDF8", "#34D399"};

    // Update button styles
    for (auto it = card_buttons_.begin(); it != card_buttons_.end(); ++it) {
        QString cardStr = it.key();
        QPushButton *btn = it.value();
        QChar suitChar = cardStr.at(1);
        int suitIdx = suitChars.indexOf(suitChar);
        QString color = (suitIdx >= 0) ? suitColors[suitIdx] : "#FFFFFF";

        if (selected_cards_.contains(cardStr)) {
            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: #2563EB;"
                "  color: #FFFFFF;"
                "  border: 2px solid #60A5FA;"
                "  border-radius: 6px;"
                "  font-size: 13px;"
                "  font-weight: bold;"
                "}"
            ));
        } else {
            btn->setStyleSheet(QString(
                "QPushButton {"
                "  background-color: #1F2937;"
                "  color: %1;"
                "  border: 1px solid #374151;"
                "  border-radius: 6px;"
                "  font-size: 13px;"
                "  font-weight: bold;"
                "}"
                "QPushButton:hover {"
                "  background-color: #374151;"
                "}"
            ).arg(color));
        }
    }

    // Update display HTML
    if (selected_cards_.isEmpty()) {
        selectedCardsDisplay_->setText("<span style=\"color: #6B7280;\">No cards selected. Click cards below to add (3 for Flop, 4 for Turn, 5 for River).</span>");
    } else {
        QString html = "Selected: ";
        for (const QString& cardStr : selected_cards_) {
            QChar rank = cardStr.at(0);
            QChar suitChar = cardStr.at(1);
            int suitIdx = suitChars.indexOf(suitChar);
            QString symbol = (suitIdx >= 0) ? suitSymbols[suitIdx] : "";
            QString color = (suitIdx >= 0) ? suitColors[suitIdx] : "#FFFFFF";
            html += QString("<span style=\"background-color: #374151; color: %1; padding: 3px 8px; border-radius: 4px; border: 1px solid #4B5563; margin-right: 6px;\">%2%3</span> ").arg(color, QString(rank), symbol);
        }
        selectedCardsDisplay_->setText(html);
    }

    // Status text & Confirm state
    int count = selected_cards_.size();
    if (count == 0) {
        statusLabel_->setText("Select at least 3 cards to solve a board.");
        statusLabel_->setStyleSheet("color: #9CA3AF; font-size: 12px;");
        confirmButton_->setEnabled(false);
    } else if (count < 3) {
        statusLabel_->setText(QString("Selected %1 cards — need at least 3 cards for a Flop.").arg(count));
        statusLabel_->setStyleSheet("color: #F59E0B; font-size: 12px; font-weight: bold;");
        confirmButton_->setEnabled(false);
    } else if (count == 3) {
        statusLabel_->setText("Flop board ready (3 cards). You can add 1 more for Turn or 2 more for River.");
        statusLabel_->setStyleSheet("color: #10B981; font-size: 12px; font-weight: bold;");
        confirmButton_->setEnabled(true);
    } else if (count == 4) {
        statusLabel_->setText("Turn board ready (4 cards: 3 Flop + 1 Turn). You can add 1 more for River.");
        statusLabel_->setStyleSheet("color: #10B981; font-size: 12px; font-weight: bold;");
        confirmButton_->setEnabled(true);
    } else if (count == 5) {
        statusLabel_->setText("River board ready (5 cards: 3 Flop + 1 Turn + 1 River). Complete board!");
        statusLabel_->setStyleSheet("color: #10B981; font-size: 12px; font-weight: bold;");
        confirmButton_->setEnabled(true);
    }
}
