#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtGlobal>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSystemTrayIcon>
#include <QMenu>
#include <Windows.h>
#include <QListWidget>
#include <QSettings>
#include <QMap>
#include <QTimer>

// New snippet class forward declaration
class TextSnippet;
class SettingsDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

private slots:
    void sendKeystroke(int snippetId = -1);
    void minimizeToTray();
    void restoreFromTray(QSystemTrayIcon::ActivationReason reason);
    void addNewSnippet();
    void editSelectedSnippet();
    void deleteSelectedSnippet();
    void snippetSelected(int index);
    void openSettings();
    void saveSnippets();
    void loadSnippets();
    void clearClipboardDelayed();
    void resetAllSettings(); // Новый метод для сброса настроек

private:
    Ui::MainWindow *ui;
    QLineEdit *nameInput;
    QLineEdit *textInput;
    QListWidget *snippetList;
    QSystemTrayIcon *trayIcon;
    QMap<int, TextSnippet*> snippets;
    QSettings settings;
    SettingsDialog *settingsDialog;
    QTimer *clipboardTimer;
    
    int nextHotkeyId;
    bool maskText;
    int typingDelay;
    bool autoClear;

    void sendText(const QString &text);
    void createTrayIcon();
    void registerHotKey(TextSnippet *snippet);
    void unregisterAllHotKeys();
    void setupUi();
    void createActions();
    QString encrypt(const QString &text);
    QString decrypt(const QString &text);
    void copyToClipboard(const QString &text);
    QString hotkeyToString(int modifiers, int key); // Новая функция для преобразования кодов в текст
    void updateSnippetListItem(int index, TextSnippet* snippet); // Новая функция для обновления элемента списка
};

// New class to represent a text snippet
class TextSnippet {
public:
    QString name;
    QString text;
    int hotkeyModifiers;
    int hotkeyKey;
    int hotkeyId;
    
    TextSnippet(const QString &name, const QString &text, int modifiers, int key, int id)
        : name(name), text(text), hotkeyModifiers(modifiers), hotkeyKey(key), hotkeyId(id) {}
};

#endif // MAINWINDOW_H
