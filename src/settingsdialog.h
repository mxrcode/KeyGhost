#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QSpinBox>
#include <QSettings>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private slots:
    void saveSettings();

private:
    QCheckBox *maskTextCheck;
    QCheckBox *autoClearCheck;
    QCheckBox *useClipboardCheck;
    QCheckBox *clearClipboardCheck;
    QSpinBox *typingDelayBox;
    QSpinBox *clipboardClearDelayBox;
    QSettings settings;

    void loadSettings();
};

#endif // SETTINGSDIALOG_H
