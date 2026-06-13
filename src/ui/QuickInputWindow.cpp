#include "ui/QuickInputWindow.hpp"

#include <QBoxLayout>
#include <QEvent>
#include <QGuiApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QScreen>
#include <QSignalBlocker>
#include <QToolButton>

#include "common/SettingsManager.hpp"
#include "ui/theme/ThemeApi.hpp"
#include "ui/theme/ThemeWidgets.hpp"

namespace {

constexpr int kComposerMinLines = 2;
constexpr int kComposerMaxLines = 5;
constexpr int kComposerRadius = 18;
constexpr int kComposerSendButtonSize = 34;
constexpr int kComposerSendIconSize = 18;
constexpr int kComposerFooterControlSize = 24;
constexpr int kComposerClearIconSize = 19;
constexpr qreal kComposerInputDocumentMargin = 3.0;

}  // namespace

class QuickInputWindow::Impl
{
public:
    QString styleId{QStringLiteral("floating")};
    QWidget* central{nullptr};
    QVBoxLayout* root{nullptr};
    QWidget* composerCard{nullptr};
    QWidget* handleRow{nullptr};
    QWidget* handleGrip{nullptr};
    QToolButton* closeBtn{nullptr};
    QBoxLayout* editorRow{nullptr};
    ThemeWidgets::ChatComposerEdit* input{nullptr};
    ThemeWidgets::IconButton* sendBtn{nullptr};
    ThemeWidgets::IconButton* clearBtn{nullptr};
    ThemeWidgets::ComboBox* styleCombo{nullptr};
    ThemeWidgets::ComboBox* modelSizeCombo{nullptr};
    QLabel* countLabel{nullptr};
    QToolButton* switchModeBtn{nullptr};
    bool dragging{false};
    QPoint dragOffset;

    void updateInputMetrics()
    {
        if (!input)
            return;

        const int targetHeight = input->preferredHeight(kComposerMinLines, kComposerMaxLines);
        if (input->height() != targetHeight)
            input->setFixedHeight(targetHeight);

        const QFontMetrics fm(input->font());
        const int maxDocumentHeight = fm.lineSpacing() * kComposerMaxLines;
        const bool overflow = input->documentHeight() > maxDocumentHeight;
        input->setVerticalScrollBarPolicy(overflow ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
    }

    void updateInputCount()
    {
        if (!countLabel || !input)
            return;
        countLabel->setText(QString::number(input->toPlainText().size()));
    }
};

namespace {

QString normalizeQuickInputPresentationStyle(const QString& style)
{
    const QString s = style.trimmed().toLower();
    if (s == QStringLiteral("dock") || s == QStringLiteral("sidebar") || s == QStringLiteral("edge"))
        return QStringLiteral("dock");
    if (s == QStringLiteral("hud") || s == QStringLiteral("frameless") || s == QStringLiteral("overlay"))
        return QStringLiteral("hud");
    return QStringLiteral("floating");
}

}

QuickInputWindow::QuickInputWindow(QWidget* parent)
    : QMainWindow(parent), d(new Impl)
{
    setWindowTitle(tr("输入"));
    resize(460, 150);
    setMinimumSize(360, 120);

    d->central = new QWidget(this);
    setCentralWidget(d->central);

    d->root = new QVBoxLayout(d->central);
    d->root->setContentsMargins(12, 12, 12, 12);
    d->root->setSpacing(10);

    d->composerCard = new QWidget(d->central);
    d->composerCard->setObjectName(QStringLiteral("quickInputComposerCard"));
    auto* composerLayout = new QVBoxLayout(d->composerCard);
    composerLayout->setContentsMargins(14, 14, 14, 14);
    composerLayout->setSpacing(6);

    d->handleRow = new QWidget(d->composerCard);
    auto* handleLayout = new QHBoxLayout(d->handleRow);
    handleLayout->setContentsMargins(0, 0, 0, 0);
    handleLayout->setSpacing(8);
    handleLayout->addStretch(1);
    d->handleGrip = new QWidget(d->handleRow);
    d->handleGrip->setObjectName(QStringLiteral("quickInputHandleGrip"));
    d->handleGrip->setFixedSize(52, 4);
    handleLayout->addWidget(d->handleGrip, 0, Qt::AlignCenter);
    handleLayout->addStretch(1);
    d->closeBtn = new QToolButton(d->handleRow);
    d->closeBtn->setObjectName(QStringLiteral("quickInputCloseButton"));
    d->closeBtn->setAutoRaise(true);
    d->closeBtn->setCursor(Qt::PointingHandCursor);
    d->closeBtn->setText(QStringLiteral("×"));
    d->closeBtn->setToolTip(tr("关闭"));
    d->closeBtn->setFixedSize(24, 24);
    handleLayout->addWidget(d->closeBtn, 0, Qt::AlignRight | Qt::AlignVCenter);
    composerLayout->addWidget(d->handleRow);

    d->editorRow = new QBoxLayout(QBoxLayout::LeftToRight);
    d->editorRow->setContentsMargins(0, 0, 0, 0);
    d->editorRow->setSpacing(10);

    d->input = new ThemeWidgets::ChatComposerEdit(d->composerCard);
    d->input->document()->setDocumentMargin(kComposerInputDocumentMargin);
    d->input->setPlaceholderText(tr("输入消息... (Enter 发送 / Shift+Enter 换行)"));

    d->sendBtn = new ThemeWidgets::IconButton(d->composerCard);
    d->sendBtn->setTone(ThemeWidgets::IconButton::Tone::Accent);
    d->sendBtn->setIconLogicalSize(kComposerSendIconSize);
    d->sendBtn->setFixedSize(kComposerSendButtonSize, kComposerSendButtonSize);
    d->sendBtn->setToolTip(tr("发送"));
    d->sendBtn->setIcon(Theme::themedIcon(Theme::IconToken::ChatSend));

    d->editorRow->addWidget(d->input, 1, Qt::AlignVCenter);
    d->editorRow->addWidget(d->sendBtn, 0, Qt::AlignRight | Qt::AlignBottom);
    composerLayout->addLayout(d->editorRow);

    auto* footerRow = new QHBoxLayout();
    footerRow->setContentsMargins(0, 0, 0, 0);
    footerRow->setSpacing(8);

    d->clearBtn = new ThemeWidgets::IconButton(d->composerCard);
    d->clearBtn->setTone(ThemeWidgets::IconButton::Tone::Ghost);
    d->clearBtn->setIconLogicalSize(kComposerClearIconSize);
    d->clearBtn->setToolTip(tr("清空输入"));
    d->clearBtn->setFixedSize(kComposerFooterControlSize, kComposerFooterControlSize);
    d->clearBtn->setIcon(Theme::themedIcon(Theme::IconToken::ChatClear));

    d->styleCombo = new ThemeWidgets::ComboBox(d->composerCard);
    d->styleCombo->addItem(QStringLiteral("Original"), QStringLiteral("Original"));
    d->styleCombo->addItem(QStringLiteral("Universal"), QStringLiteral("Universal"));
    d->styleCombo->addItem(QStringLiteral("Anime"), QStringLiteral("Anime"));
    d->styleCombo->setFixedWidth(120);
    {
        const QString saved = SettingsManager::instance().llmStyle();
        const int idx = d->styleCombo->findData(saved);
        d->styleCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }

    d->modelSizeCombo = new ThemeWidgets::ComboBox(d->composerCard);
    d->modelSizeCombo->addItem(QStringLiteral("1.5B"), QStringLiteral("1.5B"));
    d->modelSizeCombo->addItem(QStringLiteral("7B"), QStringLiteral("7B"));
    d->modelSizeCombo->setFixedWidth(90);
    {
        const QString saved = SettingsManager::instance().llmModelSize();
        const int idx = d->modelSizeCombo->findData(saved);
        d->modelSizeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }

    d->switchModeBtn = new QToolButton(d->composerCard);
    d->switchModeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);
    d->switchModeBtn->setText(tr("切换到历史对话框"));
    d->switchModeBtn->setAutoRaise(true);
    d->switchModeBtn->setCursor(Qt::PointingHandCursor);

    d->countLabel = new QLabel(QStringLiteral("0"), d->composerCard);
    d->countLabel->setObjectName(QStringLiteral("quickComposerCountLabel"));
    d->countLabel->setAlignment(Qt::AlignCenter);
    d->countLabel->setFixedSize(kComposerSendButtonSize, kComposerFooterControlSize);
    d->countLabel->setContentsMargins(0, 5, 0, 0);

    footerRow->addWidget(d->clearBtn, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerRow->addWidget(d->styleCombo, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerRow->addWidget(d->modelSizeCombo, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerRow->addWidget(d->switchModeBtn, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerRow->addStretch(1);
    footerRow->addWidget(d->countLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    composerLayout->addLayout(footerRow);

    d->root->addWidget(d->composerCard);

    auto sendNow = [this]{
        if (!d || !d->sendBtn || !d->input || !d->sendBtn->isEnabled())
            return;

        const QString text = d->input->toPlainText().trimmed();
        if (text.isEmpty())
            return;

        d->input->clear();
        d->sendBtn->setEnabled(false);
        d->input->setEnabled(false);
        emit requestSendText(text);
    };

    connect(d->sendBtn, &QToolButton::clicked, this, sendNow);
    connect(d->input, &ThemeWidgets::ChatComposerEdit::sendRequested, this, sendNow);
    connect(d->input, &ThemeWidgets::ChatComposerEdit::metricsChanged, this, [this]{
        if (!d)
            return;
        d->updateInputMetrics();
    });
    connect(d->input, &QTextEdit::textChanged, this, [this]{
        if (!d)
            return;
        d->updateInputCount();
    });
    connect(d->clearBtn, &QToolButton::clicked, this, [this]{
        if (!d || !d->input)
            return;
        d->input->clear();
        d->updateInputMetrics();
        d->updateInputCount();
        d->input->setFocus(Qt::OtherFocusReason);
    });
    connect(d->styleCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int){
        if (!d || !d->styleCombo)
            return;
        emit requestLlmStyleChanged(d->styleCombo->currentData().toString());
    });
    connect(d->modelSizeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int){
        if (!d || !d->modelSizeCombo)
            return;
        emit requestLlmModelSizeChanged(d->modelSizeCombo->currentData().toString());
    });
    connect(d->switchModeBtn, &QToolButton::clicked, this, [this]{
        emit requestSwitchToHistoryMode();
    });
    connect(d->closeBtn, &QToolButton::clicked, this, [this]{
        hide();
    });

    d->handleRow->installEventFilter(this);
    d->handleGrip->installEventFilter(this);
    d->updateInputMetrics();
    d->updateInputCount();
    setPresentationStyle(QStringLiteral("floating"));
}

QuickInputWindow::~QuickInputWindow() = default;

void QuickInputWindow::setBusy(bool busy)
{
    if (!d)
        return;
    if (d->sendBtn)
        d->sendBtn->setEnabled(!busy);
    if (d->clearBtn)
        d->clearBtn->setEnabled(!busy);
    if (d->input)
        d->input->setEnabled(!busy);
}

void QuickInputWindow::setDraftText(const QString& text)
{
    if (!d || !d->input)
        return;

    const QSignalBlocker blocker(d->input);
    d->input->setPlainText(text);
    d->input->moveCursor(QTextCursor::End);
    d->updateInputMetrics();
}

void QuickInputWindow::clearDraftText()
{
    setDraftText(QString());
}

void QuickInputWindow::focusComposer()
{
    if (!d || !d->input)
        return;
    d->input->setFocus(Qt::OtherFocusReason);
    d->input->moveCursor(QTextCursor::End);
}

QString QuickInputWindow::presentationStyle() const
{
    return d ? d->styleId : QStringLiteral("floating");
}

void QuickInputWindow::setLlmStyle(const QString& style)
{
    if (!d || !d->styleCombo)
        return;
    const int idx = d->styleCombo->findData(style);
    if (idx < 0 || d->styleCombo->currentIndex() == idx)
        return;
    const QSignalBlocker blocker(d->styleCombo);
    d->styleCombo->setCurrentIndex(idx);
}

void QuickInputWindow::setLlmModelSize(const QString& size)
{
    if (!d || !d->modelSizeCombo)
        return;
    const int idx = d->modelSizeCombo->findData(size);
    if (idx < 0 || d->modelSizeCombo->currentIndex() == idx)
        return;
    const QSignalBlocker blocker(d->modelSizeCombo);
    d->modelSizeCombo->setCurrentIndex(idx);
}

void QuickInputWindow::setPresentationStyle(const QString& style)
{
    if (!d)
        return;

    const QString normalized = normalizeQuickInputPresentationStyle(style);
    if (d->styleId == normalized && !isHidden())
    {
        d->updateInputMetrics();
    }

    d->styleId = normalized;
    const bool hud = (normalized == QStringLiteral("hud"));
    const bool dock = (normalized == QStringLiteral("dock"));

    Qt::WindowFlags flags = Qt::Window;
    if (hud)
        flags |= Qt::Tool | Qt::FramelessWindowHint;
    else if (dock)
        flags |= Qt::Tool;

    const bool wasVisible = isVisible();
    setAttribute(Qt::WA_TranslucentBackground, hud);
    setWindowFlags(flags);

    if (d->root)
        d->root->setContentsMargins(hud ? 18 : 12, hud ? 18 : 12, hud ? 18 : 12, hud ? 18 : 12);
    if (d->composerCard)
    {
        if (hud) {
            d->composerCard->setStyleSheet(
                QStringLiteral("QWidget#quickInputComposerCard {"
                               "background: rgba(18, 24, 38, 210);"
                               "border: 1px solid rgba(255, 255, 255, 36);"
                               "border-radius: 22px; }"
                               "QWidget#quickInputHandleGrip {"
                               "background: rgba(255, 255, 255, 120);"
                               "border-radius: 2px; }"
                               "QToolButton#quickInputCloseButton {"
                               "color: rgba(255, 255, 255, 180);"
                               "font-size: 18px; border: none; background: transparent; }"
                               "QToolButton#quickInputCloseButton:hover {"
                               "color: rgba(255, 255, 255, 255); }"));
        } else if (dock) {
            d->composerCard->setStyleSheet(
                QStringLiteral("QWidget#quickInputComposerCard { border-radius: 18px; }"
                               "QWidget#quickInputHandleGrip { background: palette(PlaceholderText); border-radius: 2px; }"
                               "QToolButton#quickInputCloseButton { font-size: 18px; border: none; background: transparent; color: palette(ButtonText); }"));
        } else {
            d->composerCard->setStyleSheet(
                QStringLiteral("QWidget#quickInputComposerCard { border-radius: 18px; }"
                               "QWidget#quickInputHandleGrip { background: palette(PlaceholderText); border-radius: 2px; }"
                               "QToolButton#quickInputCloseButton { font-size: 18px; border: none; background: transparent; color: palette(ButtonText); }"));
        }
    }

    if (d->handleRow)
        d->handleRow->setVisible(hud || dock);

    if (d->editorRow)
        d->editorRow->setDirection(dock ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    if (d->sendBtn)
        d->sendBtn->setFixedWidth(dock ? 72 : kComposerSendButtonSize);
    if (d->switchModeBtn)
        d->switchModeBtn->setToolButtonStyle(Qt::ToolButtonTextOnly);

    resize(dock ? QSize(360, 310) : (hud ? QSize(620, 210) : QSize(520, 210)));
    setMinimumSize(dock ? QSize(320, 280) : (hud ? QSize(520, 190) : QSize(420, 180)));

    d->updateInputMetrics();

    if (wasVisible) {
        show();
        raise();
        activateWindow();
    }
}

bool QuickInputWindow::event(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
    {
        setWindowTitle(tr("输入"));
        if (d && d->input)
            d->input->setPlaceholderText(tr("输入消息... (Enter 发送 / Shift+Enter 换行)"));
        if (d && d->sendBtn)
            d->sendBtn->setToolTip(tr("发送"));
        if (d && d->clearBtn)
            d->clearBtn->setToolTip(tr("清空输入"));
        if (d && d->closeBtn)
            d->closeBtn->setToolTip(tr("关闭"));
        if (d && d->switchModeBtn)
            d->switchModeBtn->setText(tr("切换到历史对话框"));
    }

    if (e->type() == QEvent::Resize)
    {
        if (d)
            d->updateInputMetrics();
    }

    if (e->type() == QEvent::Show)
    {
        if (d)
        {
            QScreen* screen = QGuiApplication::screenAt(frameGeometry().center());
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            const QRect available = screen ? screen->availableGeometry() : QRect(0, 0, 1280, 800);

            if (d->styleId == QStringLiteral("dock")) {
                const int margin = 16;
                move(available.right() - width() - margin, available.center().y() - height() / 2);
            } else if (d->styleId == QStringLiteral("hud")) {
                move(available.center().x() - width() / 2, available.bottom() - height() - 60);
            }
        }
    }

    if (e->type() == QEvent::ApplicationPaletteChange
        || e->type() == QEvent::PaletteChange
        || e->type() == QEvent::ThemeChange
        || e->type() == QEvent::StyleChange)
    {
        if (d && d->sendBtn)
            d->sendBtn->setIcon(Theme::themedIcon(Theme::IconToken::ChatSend));
        if (d && d->clearBtn)
            d->clearBtn->setIcon(Theme::themedIcon(Theme::IconToken::ChatClear));
        if (d)
            d->updateInputMetrics();
    }

    return QMainWindow::event(e);
}

bool QuickInputWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (!d)
        return QMainWindow::eventFilter(watched, event);

    const bool isHandle = (watched == d->handleRow || watched == d->handleGrip);
    if (isHandle && presentationStyle() == QStringLiteral("hud"))
    {
        if (event->type() == QEvent::MouseButtonPress)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                d->dragging = true;
                d->dragOffset = mouseEvent->globalPosition().toPoint() - frameGeometry().topLeft();
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove && d->dragging)
        {
            auto* mouseEvent = static_cast<QMouseEvent*>(event);
            move(mouseEvent->globalPosition().toPoint() - d->dragOffset);
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            d->dragging = false;
            return true;
        }
    }

    return QMainWindow::eventFilter(watched, event);
}
