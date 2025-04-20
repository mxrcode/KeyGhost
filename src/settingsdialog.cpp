#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("KeyGhost Settings");
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Security settings group
    QGroupBox *securityGroup = new QGroupBox("Security", this);
    QVBoxLayout *securityLayout = new QVBoxLayout(securityGroup);
    
    maskTextCheck = new QCheckBox("Mask text input (for passwords)", this);
    autoClearCheck = new QCheckBox("Auto-clear text after typing (for sensitive data)", this);
    
    securityLayout->addWidget(maskTextCheck);
    securityLayout->addWidget(autoClearCheck);
    
    // Typing settings group
    QGroupBox *typingGroup = new QGroupBox("Typing", this);
    QVBoxLayout *typingLayout = new QVBoxLayout(typingGroup);
    
    QHBoxLayout *delayLayout = new QHBoxLayout();
    QLabel *delayLabel = new QLabel("Delay between keystrokes (ms):", this);
    typingDelayBox = new QSpinBox(this);
    typingDelayBox->setRange(0, 500);
    delayLayout->addWidget(delayLabel);
    delayLayout->addWidget(typingDelayBox);
    
    useClipboardCheck = new QCheckBox("Use clipboard instead of typing simulation", this);
    
    typingLayout->addLayout(delayLayout);
    typingLayout->addWidget(useClipboardCheck);
    
    // Clipboard settings group
    QGroupBox *clipboardGroup = new QGroupBox("Clipboard", this);
    QVBoxLayout *clipboardLayout = new QVBoxLayout(clipboardGroup);
    
    clearClipboardCheck = new QCheckBox("Automatically clear clipboard after use", this);
    
    QHBoxLayout *clipboardDelayLayout = new QHBoxLayout();
    QLabel *clipboardDelayLabel = new QLabel("Clear clipboard after (seconds):", this);
    clipboardClearDelayBox = new QSpinBox(this);
    clipboardClearDelayBox->setRange(1, 300);
    clipboardDelayLayout->addWidget(clipboardDelayLabel);
    clipboardDelayLayout->addWidget(clipboardClearDelayBox);
    
    clipboardLayout->addWidget(clearClipboardCheck);
    clipboardLayout->addLayout(clipboardDelayLayout);
    
    // Button layout
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *saveButton = new QPushButton("Save", this);
    QPushButton *cancelButton = new QPushButton("Cancel", this);
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);
    
    // Add all groups to main layout
    mainLayout->addWidget(securityGroup);
    mainLayout->addWidget(typingGroup);
    mainLayout->addWidget(clipboardGroup);
    mainLayout->addLayout(buttonLayout);
    
    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &SettingsDialog::saveSettings);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    
    // Load current settings
    loadSettings();
    
    // Set a reasonable size
    resize(400, 400);
}

void SettingsDialog::loadSettings()
{
    maskTextCheck->setChecked(settings.value("MaskText", false).toBool());
    autoClearCheck->setChecked(settings.value("AutoClear", false).toBool());
    useClipboardCheck->setChecked(settings.value("UseClipboard", false).toBool());
    clearClipboardCheck->setChecked(settings.value("ClearClipboard", false).toBool());
    typingDelayBox->setValue(settings.value("TypingDelay", 30).toInt());
    clipboardClearDelayBox->setValue(settings.value("ClipboardClearDelay", 30).toInt());
}

void SettingsDialog::saveSettings()
{
    settings.setValue("MaskText", maskTextCheck->isChecked());
    settings.setValue("AutoClear", autoClearCheck->isChecked());
    settings.setValue("UseClipboard", useClipboardCheck->isChecked());
    settings.setValue("ClearClipboard", clearClipboardCheck->isChecked());
    settings.setValue("TypingDelay", typingDelayBox->value());
    settings.setValue("ClipboardClearDelay", clipboardClearDelayBox->value());
    
    settings.sync();
    accept();
}
