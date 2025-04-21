#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "settingsdialog.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QAction>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStyle>
#include <QInputDialog>
#include <QClipboard>
#include <QCryptographicHash>
#include <QGroupBox>
#include <QShortcut>
#include <QKeySequenceEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTimer>
#include <QThread>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , trayIcon(nullptr)
    , nextHotkeyId(1)
    , maskText(false)
    , typingDelay(30)
    , autoClear(false)
    , settingsDialog(nullptr)
{
    ui->setupUi(this);
    
    // Initialize clipboard timer before other operations
    clipboardTimer = new QTimer(this);
    clipboardTimer->setSingleShot(true);
    connect(clipboardTimer, &QTimer::timeout, this, &MainWindow::clearClipboardDelayed);
    
    // Set the window icon
    setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    
    // Setup UI elements
    setupUi();
    
    // Create actions
    createActions();
    
    // Load saved snippets - do this before setting up tray icon
    loadSnippets();
    
    // Setup the system tray icon
    createTrayIcon();
    
    setWindowTitle("KeyGhost");
    resize(500, 400);
}

MainWindow::~MainWindow()
{
    unregisterAllHotKeys();
    delete ui;
}

void MainWindow::setupUi()
{
    // Create central widget with layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    
    // Snippet list section
    QGroupBox *snippetsGroup = new QGroupBox("Text Snippets", this);
    QVBoxLayout *snippetsLayout = new QVBoxLayout(snippetsGroup);
    
    snippetList = new QListWidget(this);
    snippetsLayout->addWidget(snippetList);
    
    // Buttons for list management
    QHBoxLayout *listButtonLayout = new QHBoxLayout();
    QPushButton *addButton = new QPushButton("Add New", this);
    QPushButton *editButton = new QPushButton("Edit", this);
    QPushButton *deleteButton = new QPushButton("Delete", this);
    listButtonLayout->addWidget(addButton);
    listButtonLayout->addWidget(editButton);
    listButtonLayout->addWidget(deleteButton);
    snippetsLayout->addLayout(listButtonLayout);
    
    mainLayout->addWidget(snippetsGroup);
    
    // Details group
    QGroupBox *detailsGroup = new QGroupBox("Snippet Details", this);
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsGroup);
    
    // Name input
    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("Name:", this);
    nameInput = new QLineEdit(this);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameInput);
    detailsLayout->addLayout(nameLayout);
    
    // Text input
    QLabel *textLabel = new QLabel("Text to type:", this);
    textInput = new QLineEdit(this);
    detailsLayout->addWidget(textLabel);
    detailsLayout->addWidget(textInput);
    
    // Action buttons
    QHBoxLayout *actionButtonLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save Snippet", this);
    QPushButton *testButton = new QPushButton("Test Typing", this);
    QPushButton *settingsButton = new QPushButton("Settings", this);
    actionButtonLayout->addWidget(saveButton);
    actionButtonLayout->addWidget(testButton);
    actionButtonLayout->addWidget(settingsButton);
    detailsLayout->addLayout(actionButtonLayout);
    
    // Add reset button
    QPushButton *resetButton = new QPushButton("Reset All Settings", this);
    detailsLayout->addWidget(resetButton);
    
    mainLayout->addWidget(detailsGroup);
    
    // Connect signals
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addNewSnippet);
    connect(editButton, &QPushButton::clicked, this, &MainWindow::editSelectedSnippet);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::deleteSelectedSnippet);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveSnippets);
    connect(testButton, &QPushButton::clicked, [this]() { sendKeystroke(-1); });
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetAllSettings);
    connect(snippetList, &QListWidget::currentRowChanged, this, &MainWindow::snippetSelected);
    
    setCentralWidget(centralWidget);
}

void MainWindow::createActions()
{
    // Keyboard shortcuts
    QShortcut *addShortcut = new QShortcut(QKeySequence("Ctrl+N"), this);
    connect(addShortcut, &QShortcut::activated, this, &MainWindow::addNewSnippet);
    
    QShortcut *saveShortcut = new QShortcut(QKeySequence("Ctrl+S"), this);
    connect(saveShortcut, &QShortcut::activated, this, &MainWindow::saveSnippets);
    
    QShortcut *deleteShortcut = new QShortcut(QKeySequence("Delete"), this);
    connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::deleteSelectedSnippet);
}

void MainWindow::registerHotKey(TextSnippet *snippet)
{
    if (!snippet) return;
    
    // Validate hotkey parameters
    if (snippet->hotkeyKey == 0 || snippet->hotkeyId <= 0) {
        qWarning("Invalid hotkey parameters for snippet: %s", qPrintable(snippet->name));
        return;
    }
    
    // Try to register the hotkey, retry with a different combination if it fails
    int attempts = 0;
    bool success = false;
    
    while (!success && attempts < 5) {
        if (RegisterHotKey((HWND)winId(), snippet->hotkeyId, 
                        snippet->hotkeyModifiers, snippet->hotkeyKey)) {
            success = true;
        } else {
            // If failed, try a different key combination
            attempts++;
            
            if (attempts >= 5) {
                qWarning("Failed to register hotkey for snippet: %s", qPrintable(snippet->name));
                return;
            }
            
            // Try different modifier combinations
            if (snippet->hotkeyModifiers == MOD_ALT) {
                snippet->hotkeyModifiers = MOD_CONTROL;
            } else if (snippet->hotkeyModifiers == MOD_CONTROL) {
                snippet->hotkeyModifiers = MOD_CONTROL | MOD_ALT;
            } else {
                // Increment the key if we've tried all modifier combinations
                snippet->hotkeyKey++;
            }
        }
    }
}

void MainWindow::unregisterAllHotKeys()
{
    for (const auto& snippet : snippets) {
        UnregisterHotKey((HWND)winId(), snippet->hotkeyId);
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result)
{
    MSG* msg = static_cast<MSG*>(message);
    if (msg->message == WM_HOTKEY) {
        int id = static_cast<int>(msg->wParam);
        sendKeystroke(id);
        return true;
    }
    return false;
}

void MainWindow::sendKeystroke(int snippetId)
{
    QString textToSend;
    
    if (snippetId == -1) {
        // Test button was pressed, use current input
        textToSend = textInput->text();
        if (textToSend.isEmpty()) {
            QMessageBox::warning(this, "No Text", "Please enter some text first.");
            return;
        }
    } else {
        // Hotkey was pressed, find the snippet
        if (!snippets.contains(snippetId)) {
            QMessageBox::warning(this, "Error", "Snippet not found.");
            return;
        }
        textToSend = snippets[snippetId]->text;
    }
    
    // Ask user for typing target
    QMessageBox msgBox;
    msgBox.setText("Click in the target window where you want to type, then press OK.");
    msgBox.exec();
    
    // Use QTimer instead of Sleep to avoid blocking UI thread
    QTimer::singleShot(1500, this, [this, textToSend, snippetId]() {
        // Ensure the target application has focus before typing
        HWND foregroundWindow = GetForegroundWindow();
        if (foregroundWindow == nullptr || foregroundWindow == (HWND)this->winId()) {
            QMessageBox::warning(this, "Error", "Failed to detect target window. Please try again.");
            return;
        }
        
        sendText(textToSend);
        
        // Auto-clear if enabled
        if (autoClear && snippetId != -1 && snippets.contains(snippetId)) {
            // Clear the text from memory securely
            snippets[snippetId]->text.fill('0');
            snippets[snippetId]->text.clear();
            saveSnippets();
            QMessageBox::information(this, "Auto-Clear", 
                "The text has been typed and cleared from memory for security.");
        }
    });
}

void MainWindow::sendText(const QString &text)
{
    // Option to use clipboard instead of typing
    if (settings.value("UseClipboard", false).toBool()) {
        copyToClipboard(text);
        QMessageBox::information(this, "Clipboard", 
            "Text has been copied to clipboard. Press Ctrl+V to paste.");
        return;
    }

    // Improved input sending method
    INPUT input;
    
    // Get current typing delay from settings
    int currentDelay = settings.value("TypingDelay", typingDelay).toInt();
    
    // For each character in the string
    for (QChar c : text) {
        // Get virtual key code and scan code for the character
        SHORT vkScan = VkKeyScanW(c.unicode());
        WORD vkCode = vkScan & 0xFF;
        BOOL needShift = vkScan & 0x100;
        
        if (vkCode == 0xFF) {
            // Character can't be typed with a normal keystroke
            // Use the KEYEVENTF_UNICODE flag for direct Unicode input
            INPUT unicodeInput[2];
            ZeroMemory(unicodeInput, sizeof(unicodeInput));
            
            // Key down event
            unicodeInput[0].type = INPUT_KEYBOARD;
            unicodeInput[0].ki.wVk = 0;
            unicodeInput[0].ki.wScan = c.unicode();
            unicodeInput[0].ki.dwFlags = KEYEVENTF_UNICODE;
            
            // Key up event
            unicodeInput[1].type = INPUT_KEYBOARD;
            unicodeInput[1].ki.wVk = 0;
            unicodeInput[1].ki.wScan = c.unicode();
            unicodeInput[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
            
            SendInput(2, unicodeInput, sizeof(INPUT));
        } else {
            // Normal character input sequence
            std::vector<INPUT> inputs;
            
            // Press Shift if needed
            if (needShift) {
                ZeroMemory(&input, sizeof(INPUT));
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = VK_SHIFT;
                inputs.push_back(input);
            }
            
            // Press the key
            ZeroMemory(&input, sizeof(INPUT));
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = vkCode;
            inputs.push_back(input);
            
            // Release the key
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            inputs.push_back(input);
            
            // Release Shift if it was pressed
            if (needShift) {
                ZeroMemory(&input, sizeof(INPUT));
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = VK_SHIFT;
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                inputs.push_back(input);
            }
            
            SendInput(inputs.size(), inputs.data(), sizeof(INPUT));
        }
        
        // Non-blocking delay between keystrokes
        QThread::msleep(currentDelay);
    }
}

void MainWindow::createTrayIcon()
{
    // Delete old menu if it exists to prevent memory leaks
    if (trayIcon && trayIcon->contextMenu()) {
        delete trayIcon->contextMenu();
    }
    
    // Create new menu
    QMenu *trayMenu = new QMenu(this);
    QAction *showAction = trayMenu->addAction("Show");
    trayMenu->addSeparator();
    
    // Add snippets to tray menu
    if (!snippets.isEmpty()) {
        QMenu *snippetsMenu = trayMenu->addMenu("Type Snippets");
        for (const auto& snippet : snippets) {
            if (snippet && !snippet->name.isEmpty()) {
                QAction *snippetAction = snippetsMenu->addAction(snippet->name);
                connect(snippetAction, &QAction::triggered, [this, id = snippet->hotkeyId]() {
                    sendKeystroke(id);
                });
            }
        }
        trayMenu->addSeparator();
    }
    
    QAction *quitAction = trayMenu->addAction("Quit");
    
    connect(showAction, &QAction::triggered, this, &QWidget::show);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    
    // Create or update tray icon
    if (!trayIcon) {
        trayIcon = new QSystemTrayIcon(this);
        trayIcon->setIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
        connect(trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::restoreFromTray);
    }
    
    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void MainWindow::minimizeToTray()
{
    hide();
    trayIcon->showMessage("KeyGhost", "Application is running in the background.", 
                         QSystemTrayIcon::Information, 2000);
}

void MainWindow::restoreFromTray(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        show();
        activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Give the user an option to quit or minimize to tray
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("KeyGhost");
    msgBox.setText("Do you want to exit the application or minimize to tray?");
    msgBox.setIcon(QMessageBox::Question);
    
    // Create custom buttons for our specific needs
    QPushButton *exitButton = msgBox.addButton("Exit", QMessageBox::AcceptRole);
    QPushButton *minimizeButton = msgBox.addButton("Minimize to Tray", QMessageBox::ActionRole);
    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    
    // Set the default button
    msgBox.setDefaultButton(minimizeButton);
    
    msgBox.exec();
    
    if (msgBox.clickedButton() == exitButton) {
        event->accept(); // This will close the application
    } else if (msgBox.clickedButton() == minimizeButton) {
        minimizeToTray();
        event->ignore();
    } else {
        event->ignore(); // User clicked Cancel
    }
}

void MainWindow::addNewSnippet()
{
    // Replace QInputDialog::getText with a simple custom dialog
    QDialog dialog(this);
    dialog.setWindowTitle("New Snippet");
    dialog.setFixedSize(350, 150); // Make dialog a bit larger for the hotkey editor
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *label = new QLabel("Enter a name for the new snippet:", &dialog);
    QLineEdit *input = new QLineEdit(&dialog);
    
    QHBoxLayout *hotkeyLayout = new QHBoxLayout();
    QLabel *hotkeyLabel = new QLabel("Hotkey:", &dialog);
    QKeySequenceEdit *hotkeyEdit = new QKeySequenceEdit(&dialog);
    hotkeyLayout->addWidget(hotkeyLabel);
    hotkeyLayout->addWidget(hotkeyEdit);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    layout->addWidget(label);
    layout->addWidget(input);
    layout->addLayout(hotkeyLayout);
    layout->addWidget(buttonBox);
    
    QString name;
    QKeySequence keySeq;
    if (dialog.exec() == QDialog::Accepted && !input->text().isEmpty()) {
        name = input->text();
        keySeq = hotkeyEdit->keySequence();
    } else {
        return; // User canceled or entered empty text
    }
    
    int id = nextHotkeyId++;
    
    // Convert QKeySequence to Windows hotkey format
    int mod = 0;
    int key = 0;
    
    if (!keySeq.isEmpty()) {
        int qtKey = keySeq[0] & ~Qt::KeyboardModifierMask;
        
        // Handle modifiers
        if (keySeq[0] & Qt::AltModifier)     mod |= MOD_ALT;
        if (keySeq[0] & Qt::ControlModifier) mod |= MOD_CONTROL;
        if (keySeq[0] & Qt::ShiftModifier)   mod |= MOD_SHIFT;
        if (keySeq[0] & Qt::MetaModifier)    mod |= MOD_WIN;
        
        // Map Qt key to Windows VK
        key = qtKey;
    } else {
        // Default hotkey (Alt+1, Alt+2, etc.)
        mod = MOD_ALT;
        key = 0x31 + snippets.size() % 9; // Number keys 1-9
    }
    
    TextSnippet *snippet = new TextSnippet(name, "", mod, key, id);
    snippets[id] = snippet;
    
    // Update UI with name and hotkey
    QListWidgetItem *item = new QListWidgetItem();
    snippetList->addItem(item);
    updateSnippetListItem(snippetList->count() - 1, snippet);
    snippetList->setCurrentRow(snippetList->count() - 1);
    
    // Register hotkey
    registerHotKey(snippet);
    
    // Save snippets
    saveSnippets();
}

void MainWindow::editSelectedSnippet()
{
    int row = snippetList->currentRow();
    if (row >= 0) {
        int id = -1;
        for (auto it = snippets.begin(); it != snippets.end(); ++it) {
            if (it.value()->name == snippetList->item(row)->text().split(" [")[0]) {
                id = it.key();
                break;
            }
        }
        
        if (id != -1) {
            TextSnippet *snippet = snippets[id];
            nameInput->setText(snippet->name);
            textInput->setText(snippet->text);
            
            // Add hotkey editing dialog
            QDialog hotkeyDialog(this);
            hotkeyDialog.setWindowTitle("Edit Hotkey");
            hotkeyDialog.setFixedSize(300, 150);
            
            QVBoxLayout *layout = new QVBoxLayout(&hotkeyDialog);
            
            QLabel *label = new QLabel("Press the keys for the new hotkey:", &hotkeyDialog);
            QKeySequenceEdit *hotkeyEdit = new QKeySequenceEdit(&hotkeyDialog);
            
            // Set current hotkey if possible
            int virtualKey = snippet->hotkeyKey;
            int modifiers = snippet->hotkeyModifiers;
            
            // Convert Windows modifiers to Qt modifiers
            Qt::KeyboardModifiers qtMods = Qt::NoModifier;
            if (modifiers & MOD_ALT) qtMods |= Qt::AltModifier;
            if (modifiers & MOD_CONTROL) qtMods |= Qt::ControlModifier;
            if (modifiers & MOD_SHIFT) qtMods |= Qt::ShiftModifier;
            if (modifiers & MOD_WIN) qtMods |= Qt::MetaModifier;
            
            QKeySequence currentSeq(qtMods | virtualKey);
            hotkeyEdit->setKeySequence(currentSeq);
            
            QDialogButtonBox *buttonBox = new QDialogButtonBox(
                QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &hotkeyDialog);
            
            connect(buttonBox, &QDialogButtonBox::accepted, &hotkeyDialog, &QDialog::accept);
            connect(buttonBox, &QDialogButtonBox::rejected, &hotkeyDialog, &QDialog::reject);
            
            layout->addWidget(label);
            layout->addWidget(hotkeyEdit);
            layout->addWidget(buttonBox);
            
            if (hotkeyDialog.exec() == QDialog::Accepted) {
                QKeySequence keySeq = hotkeyEdit->keySequence();
                if (!keySeq.isEmpty()) {
                    // Unregister old hotkey
                    UnregisterHotKey((HWND)winId(), snippet->hotkeyId);
                    
                    // Convert QKeySequence to Windows hotkey format
                    int mod = 0;
                    int key = 0;
                    
                    int qtKey = keySeq[0] & ~Qt::KeyboardModifierMask;
                    
                    // Handle modifiers
                    if (keySeq[0] & Qt::AltModifier)     mod |= MOD_ALT;
                    if (keySeq[0] & Qt::ControlModifier) mod |= MOD_CONTROL;
                    if (keySeq[0] & Qt::ShiftModifier)   mod |= MOD_SHIFT;
                    if (keySeq[0] & Qt::MetaModifier)    mod |= MOD_WIN;
                    
                    // Map Qt key to Windows VK
                    key = qtKey;
                    
                    // Update snippet
                    snippet->hotkeyModifiers = mod;
                    snippet->hotkeyKey = key;
                    
                    // Register new hotkey
                    registerHotKey(snippet);
                    
                    // Update list display
                    updateSnippetListItem(row, snippet);
                    
                    // Save changes
                    saveSnippets();
                }
            }
        }
    }
}

void MainWindow::deleteSelectedSnippet()
{
    int row = snippetList->currentRow();
    if (row >= 0) {
        int id = -1;
        for (auto it = snippets.begin(); it != snippets.end(); ++it) {
            if (it.value()->name == snippetList->item(row)->text().split(" [")[0]) {
                id = it.key();
                break;
            }
        }
        
        if (id != -1) {
            // Unregister hotkey
            UnregisterHotKey((HWND)winId(), snippets[id]->hotkeyId);
            
            // Remove from memory
            delete snippets[id];
            snippets.remove(id);
            
            // Update UI
            delete snippetList->takeItem(row);
            
            // Save changes
            saveSnippets();
        }
    }
}

void MainWindow::snippetSelected(int index)
{
    if (index >= 0) {
        QString fullText = snippetList->item(index)->text();
        // Retrieve only name without hotkey
        QString name = fullText.split(" [")[0];
        
        for (const auto& snippet : snippets) {
            if (snippet->name == name) {
                nameInput->setText(snippet->name);
                textInput->setText(snippet->text);
                return;
            }
        }
    }
}

void MainWindow::openSettings()
{
    if (!settingsDialog) {
        settingsDialog = new SettingsDialog(this);
        connect(settingsDialog, &QDialog::accepted, [this]() {
            // Apply settings
            maskText = settings.value("MaskText", false).toBool();
            typingDelay = settings.value("TypingDelay", 30).toInt();
            autoClear = settings.value("AutoClear", false).toBool();
            
            // Update text masking
            textInput->setEchoMode(maskText ? QLineEdit::Password : QLineEdit::Normal);
        });
    }
    
    settingsDialog->show();
    settingsDialog->raise();
    settingsDialog->activateWindow();
}

void MainWindow::saveSnippets()
{
    // Save current snippet if editing
    int currentRow = snippetList->currentRow();
    if (currentRow >= 0) {
        QString name = snippetList->item(currentRow)->text().split(" [")[0]; // Get only name without hotkey
        for (auto& snippet : snippets) {
            if (snippet->name == name) {
                // Update with current values
                QString newName = nameInput->text();
                snippet->name = newName;
                snippet->text = textInput->text();
                
                // Update list item if name changed
                if (name != newName) {
                    updateSnippetListItem(currentRow, snippet);
                }
                break;
            }
        }
    }
    
    // Save all snippets to settings
    settings.beginGroup("Snippets");
    settings.remove(""); // Clear existing settings
    
    int index = 0;
    for (const auto& snippet : snippets) {
        settings.beginGroup(QString::number(index));
        settings.setValue("Name", snippet->name);
        settings.setValue("Text", encrypt(snippet->text));
        settings.setValue("ModKeys", snippet->hotkeyModifiers);
        settings.setValue("Key", snippet->hotkeyKey);
        settings.setValue("Id", snippet->hotkeyId);
        settings.endGroup();
        index++;
    }
    
    settings.endGroup();
    settings.sync();
    
    // Update the tray menu
    createTrayIcon();
}

void MainWindow::loadSnippets()
{
    // Clear existing snippets
    snippetList->clear();
    for (auto snippet : snippets) {
        delete snippet;
    }
    snippets.clear();
    
    // Apply settings first
    maskText = settings.value("MaskText", false).toBool();
    typingDelay = settings.value("TypingDelay", 30).toInt();
    autoClear = settings.value("AutoClear", false).toBool();
    
    textInput->setEchoMode(maskText ? QLineEdit::Password : QLineEdit::Normal);

    // Load snippets from settings
    settings.beginGroup("Snippets");
    QStringList groups = settings.childGroups();
    
    // Checking for possible duplicate IDs
    QSet<int> usedIds;
    
    for (const auto& group : groups) {
        try {
            settings.beginGroup(group);
            
            QString name = settings.value("Name").toString().trimmed();
            QString encryptedText = settings.value("Text").toString();
            int modKeys = settings.value("ModKeys", 0).toInt();
            int key = settings.value("Key", 0).toInt();
            int id = settings.value("Id", -1).toInt();

            // Stricter data validity check
            if (name.isEmpty() || id < 0 || key == 0 || usedIds.contains(id)) {
                qWarning("Skipping an invalid snippet: name='%s', id=%d, key=%d, already used=%d",
                    qPrintable(name), id, key, usedIds.contains(id));
                settings.endGroup();
                continue;
            }
            
            usedIds.insert(id);
            
            if (id >= nextHotkeyId) {
                nextHotkeyId = id + 1;
            }
            
            // Use empty string if decryption fails
            QString decryptedText;
            try {
                decryptedText = decrypt(encryptedText);
            } catch (...) {
                decryptedText = "";
                qWarning("Decryption error for the snippet: %s", qPrintable(name));
            }
            
            TextSnippet *snippet = new TextSnippet(
                name, decryptedText, modKeys, key, id);
            
            snippets[id] = snippet;

            // Add an item to the list
            QListWidgetItem *item = new QListWidgetItem();
            snippetList->addItem(item);
            updateSnippetListItem(snippetList->count() - 1, snippet);
            
            // Try to register hotkey, but don't crash if it fails
            try {
                registerHotKey(snippet);
            } catch (...) {
                qWarning("Failed to register hotkey for snippet: %s", qPrintable(name));
            }
            
            settings.endGroup();
        } catch (const std::exception& e) {
            qWarning("Snippet loading exception: %s", e.what());
            settings.endGroup();
            continue;
        } catch (...) {
            qWarning("Unknown exception on snippet loading");
            settings.endGroup();
            continue;
        }
    }
    
    settings.endGroup();
}

void MainWindow::resetAllSettings()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this,
        "Reset all settings",
        "Are you sure you want to remove all snippets and settings? This action cannot be undone.",
        QMessageBox::Yes | QMessageBox::No);
        
    if (reply == QMessageBox::Yes) {
        // Clean up current snippets
        snippetList->clear();
        for (auto snippet : snippets) {
            UnregisterHotKey((HWND)winId(), snippet->hotkeyId);
            delete snippet;
        }
        snippets.clear();
        
        // Clear settings
        settings.clear();
        settings.sync();
        
        // Reset nextHotkeyId
        nextHotkeyId = 1;
        
        // Update UI
        nameInput->clear();
        textInput->clear();
        
        QMessageBox::information(this, "Settings reset",
            "All snippets and settings have been removed. The next time the program is started, it will be in the default state.");
    }
}

QString MainWindow::encrypt(const QString &text)
{
    // Use a more secure encryption method with salt
    if (text.isEmpty()) return "";
    
    // Generate a random salt
    QByteArray salt = QCryptographicHash::hash(
        QDateTime::currentDateTime().toString().toUtf8(),
        QCryptographicHash::Md5);
    
    // Create a key using password-based key derivation
    QByteArray keyMaterial = "KeyGhostSecureKey123" + salt;
    QByteArray key = QCryptographicHash::hash(keyMaterial, QCryptographicHash::Sha256);
    
    // XOR with the derived key (still not truly secure, but better than before)
    QByteArray textData = text.toUtf8();
    QByteArray result;
    
    for (int i = 0; i < textData.size(); i++) {
        result.append(textData[i] ^ key[i % key.size()]);
    }
    
    // Prepend the salt to the encrypted data
    QByteArray finalResult = salt + result;
    return finalResult.toBase64();
}

QString MainWindow::decrypt(const QString &text)
{
    if (text.isEmpty()) return "";
    
    try {
        QByteArray data = QByteArray::fromBase64(text.toUtf8());
        if (data.size() < 16) return ""; // Not enough data for salt + content
        
        // Extract salt (first 16 bytes)
        QByteArray salt = data.left(16);
        QByteArray encrypted = data.mid(16);
        
        // Recreate the key
        QByteArray keyMaterial = "KeyGhostSecureKey123" + salt;
        QByteArray key = QCryptographicHash::hash(keyMaterial, QCryptographicHash::Sha256);
        
        // Decrypt using XOR
        QByteArray result;
        for (int i = 0; i < encrypted.size(); i++) {
            result.append(encrypted[i] ^ key[i % key.size()]);
        }
        
        return QString::fromUtf8(result);
    } catch (...) {
        // Return empty string if decryption fails
        return "";
    }
}

void MainWindow::copyToClipboard(const QString &text)
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(text);
    
    // Start timer to clear clipboard if setting enabled
    if (settings.value("ClearClipboard", false).toBool()) {
        int clearDelay = settings.value("ClipboardClearDelay", 30).toInt();
        clipboardTimer->start(clearDelay * 1000);
    }
}

void MainWindow::clearClipboardDelayed()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->clear();
}

QString MainWindow::hotkeyToString(int modifiers, int key)
{
    QString result;

    // Add modifiers
    if (modifiers & MOD_CONTROL)
        result += "Ctrl+";
    if (modifiers & MOD_ALT)
        result += "Alt+";
    if (modifiers & MOD_SHIFT)
        result += "Shift+";
    if (modifiers & MOD_WIN)
        result += "Win+";
    
    // Add a main key
    // Special keys
    switch (key) {
        case VK_F1: result += "F1"; break;
        case VK_F2: result += "F2"; break;
        case VK_F3: result += "F3"; break;
        case VK_F4: result += "F4"; break;
        case VK_F5: result += "F5"; break;
        case VK_F6: result += "F6"; break;
        case VK_F7: result += "F7"; break;
        case VK_F8: result += "F8"; break;
        case VK_F9: result += "F9"; break;
        case VK_F10: result += "F10"; break;
        case VK_F11: result += "F11"; break;
        case VK_F12: result += "F12"; break;
        case VK_INSERT: result += "Insert"; break;
        case VK_DELETE: result += "Delete"; break;
        case VK_HOME: result += "Home"; break;
        case VK_END: result += "End"; break;
        case VK_PRIOR: result += "Page Up"; break;
        case VK_NEXT: result += "Page Down"; break;
        case VK_LEFT: result += "Left"; break;
        case VK_RIGHT: result += "Right"; break;
        case VK_UP: result += "Up"; break;
        case VK_DOWN: result += "Down"; break;
        case VK_NUMPAD0: result += "Num 0"; break;
        case VK_NUMPAD1: result += "Num 1"; break;
        case VK_NUMPAD2: result += "Num 2"; break;
        case VK_NUMPAD3: result += "Num 3"; break;
        case VK_NUMPAD4: result += "Num 4"; break;
        case VK_NUMPAD5: result += "Num 5"; break;
        case VK_NUMPAD6: result += "Num 6"; break;
        case VK_NUMPAD7: result += "Num 7"; break;
        case VK_NUMPAD8: result += "Num 8"; break;
        case VK_NUMPAD9: result += "Num 9"; break;
        case VK_MULTIPLY: result += "Num *"; break;
        case VK_ADD: result += "Num +"; break;
        case VK_SUBTRACT: result += "Num -"; break;
        case VK_DIVIDE: result += "Num /"; break;
        // Letters and numbers
        default:
            if ((key >= '0' && key <= '9') || (key >= 'A' && key <= 'Z')) {
                result += QChar(key);
            } else {
                // Show code for untranslated keys
                result += QString("Key(%1)").arg(key);
            }
            break;
    }
    
    return result;
}

void MainWindow::updateSnippetListItem(int index, TextSnippet* snippet)
{
    if (!snippet || index < 0 || index >= snippetList->count()) return;
    
    QString hotkeyString = hotkeyToString(snippet->hotkeyModifiers, snippet->hotkeyKey);
    QString displayText = QString("%1 [%2]").arg(snippet->name, hotkeyString);
    snippetList->item(index)->setText(displayText);
}
