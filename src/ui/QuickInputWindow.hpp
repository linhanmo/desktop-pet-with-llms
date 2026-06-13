#pragma once

#include <QMainWindow>
#include <QScopedPointer>
#include <QString>

class QEvent;
class QObject;
class QPoint;

class QuickInputWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit QuickInputWindow(QWidget* parent = nullptr);
    ~QuickInputWindow() override;

Q_SIGNALS:
    void requestSendText(const QString& text);
    void requestLlmStyleChanged(const QString& style);
    void requestLlmModelSizeChanged(const QString& size);
    void requestSwitchToHistoryMode();

public Q_SLOTS:
    void setBusy(bool busy);
    void setDraftText(const QString& text);
    void clearDraftText();
    void focusComposer();
    void setPresentationStyle(const QString& style);
    QString presentationStyle() const;
    void setLlmStyle(const QString& style);
    void setLlmModelSize(const QString& size);

protected:
    bool event(QEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    class Impl;
    QScopedPointer<Impl> d;
};
