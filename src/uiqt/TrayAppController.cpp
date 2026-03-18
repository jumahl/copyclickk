#include "TrayAppController.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMessageBox>
#include <QMimeData>
#include <QPalette>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QTextDocumentFragment>
#include <QToolButton>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QImageReader>
#include <QListWidgetItem>
#include <QPixmap>
#include <QSize>
#include <QString>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

#ifdef COPYCLICKK_HAS_KGLOBALACCEL
#include <KGlobalAccel>
#endif

#ifdef COPYCLICKK_HAS_KGUIADDONS
#include <KSystemClipboard>
#endif

namespace copyclickk {

namespace {

class HistoryWindow final : public QWidget {
 public:
  explicit HistoryWindow(QWidget* parent = nullptr) : QWidget(parent) {}
  std::function<void()> onClosed;

 protected:
  void closeEvent(QCloseEvent* event) override {
    if (onClosed) {
      onClosed();
    }
    QWidget::closeEvent(event);
  }
};

class SettingsDialog final : public QDialog {
 public:
  explicit SettingsDialog(QWidget* parent = nullptr) : QDialog(parent) {}
  std::function<void()> onClosed;

 protected:
  void closeEvent(QCloseEvent* event) override {
    if (onClosed) {
      onClosed();
    }
    QDialog::closeEvent(event);
  }
};

std::string payloadToText(const ClipboardItem& item) {
  return std::string(item.payload.begin(), item.payload.end());
}

QString previewTextForItem(const ClipboardItem& item) {
  const QString raw = QString::fromStdString(payloadToText(item));
  if (item.mimeType == ClipboardMimeType::TextHtml) {
    const QString plain = QTextDocumentFragment::fromHtml(raw).toPlainText().trimmed();
    if (!plain.isEmpty()) {
      return plain;
    }
  }

  return raw;
}

QString truncatedPreview(QString text, int maxChars = 30) {
  text.replace('\n', ' ');
  text.replace('\r', ' ');
  if (text.size() <= maxChars) {
    return text;
  }
  return text.left(maxChars) + "...";
}

bool isImageMimeType(ClipboardMimeType mimeType) {
  return mimeType == ClipboardMimeType::ImagePng || mimeType == ClipboardMimeType::ImageJpeg ||
         mimeType == ClipboardMimeType::ImageWebp || mimeType == ClipboardMimeType::ImageBmp ||
         mimeType == ClipboardMimeType::ImageIco;
}

ClipboardMimeType imageMimeTypeFromSuffix(const QString& suffix) {
  const QString lower = suffix.toLower();
  if (lower == "png") {
    return ClipboardMimeType::ImagePng;
  }
  if (lower == "jpg" || lower == "jpeg") {
    return ClipboardMimeType::ImageJpeg;
  }
  if (lower == "webp") {
    return ClipboardMimeType::ImageWebp;
  }
  if (lower == "bmp") {
    return ClipboardMimeType::ImageBmp;
  }
  if (lower == "ico") {
    return ClipboardMimeType::ImageIco;
  }
  return ClipboardMimeType::ImagePng;
}

QPixmap pixmapFromItem(const ClipboardItem& item) {
  QPixmap pixmap;
  pixmap.loadFromData(reinterpret_cast<const uchar*>(item.payload.data()), static_cast<uint>(item.payload.size()));
  return pixmap;
}

QIcon resolveTrayIcon() {
#ifdef COPYCLICKK_ASSETS_DIR
  const QString trayIconPath = QStringLiteral(COPYCLICKK_ASSETS_DIR) + "/icons/tray/clipboard.svg";

  if (QFileInfo::exists(trayIconPath)) {
    const QIcon icon(trayIconPath);
    if (!icon.isNull()) {
      return icon;
    }
  }
#endif
  return QApplication::style()->standardIcon(QStyle::SP_FileDialogDetailedView);
}

QIcon resolveActionIcon(const QString& relativeAssetPath, QStyle::StandardPixmap fallback) {
#ifdef COPYCLICKK_ASSETS_DIR
  const QString fullPath = QStringLiteral(COPYCLICKK_ASSETS_DIR) + "/" + relativeAssetPath;
  if (QFileInfo::exists(fullPath)) {
    const QIcon icon(fullPath);
    if (!icon.isNull()) {
      return icon;
    }
  }
#endif
  return QApplication::style()->standardIcon(fallback);
}

QIcon resolvePinIcon(bool pinned) {
  if (pinned) {
    return resolveActionIcon("icons/tray/pin-filled.svg", QStyle::SP_DialogApplyButton);
  }
  return resolveActionIcon("icons/tray/pin.svg", QStyle::SP_DialogApplyButton);
}

QIcon resolveDeleteIcon() {
  return resolveActionIcon("icons/tray/trash.svg", QStyle::SP_TrashIcon);
}

std::string defaultSettingsFilePath() {
  const char* home = std::getenv("HOME");
  const std::string baseDir = home != nullptr ? std::string(home) + "/.config/copyclickk" : ".";
  std::error_code ec;
  std::filesystem::create_directories(baseDir, ec);
  std::filesystem::permissions(baseDir,
                               std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace,
                               ec);
  return baseDir + "/settings.ini";
}

std::string autostartDesktopFilePath() {
  const char* home = std::getenv("HOME");
  const std::string baseDir = home != nullptr ? std::string(home) + "/.config/autostart" : ".";
  std::error_code ec;
  std::filesystem::create_directories(baseDir, ec);
  std::filesystem::permissions(baseDir,
                               std::filesystem::perms::owner_all,
                               std::filesystem::perm_options::replace,
                               ec);
  return baseDir + "/copyclickk.desktop";
}

void writeAutostartFile(const std::string& desktopFilePath) {
  std::ofstream output(desktopFilePath, std::ios::trunc);
  if (!output.is_open()) {
    return;
  }

  const QString executable = QCoreApplication::applicationFilePath();
  output << "[Desktop Entry]\n";
  output << "Type=Application\n";
  output << "Version=1.0\n";
  output << "Name=CopyClickk\n";
  output << "Exec=\"" << executable.toStdString() << "\"\n";
  output << "X-GNOME-Autostart-enabled=true\n";
  output << "Terminal=false\n";

  output.flush();
  if (!output.good()) {
    return;
  }

  std::error_code ec;
  std::filesystem::permissions(desktopFilePath,
                               std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                               std::filesystem::perm_options::replace,
                               ec);
}

QPalette darkPalette() {
  QPalette palette;
  palette.setColor(QPalette::Window, QColor(37, 37, 38));
  palette.setColor(QPalette::WindowText, QColor(220, 220, 220));
  palette.setColor(QPalette::Base, QColor(30, 30, 30));
  palette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
  palette.setColor(QPalette::ToolTipBase, QColor(240, 240, 240));
  palette.setColor(QPalette::ToolTipText, QColor(20, 20, 20));
  palette.setColor(QPalette::Text, QColor(220, 220, 220));
  palette.setColor(QPalette::Button, QColor(45, 45, 45));
  palette.setColor(QPalette::ButtonText, QColor(220, 220, 220));
  palette.setColor(QPalette::Highlight, QColor(64, 128, 255));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  return palette;
}

QPalette lightPalette() {
  QPalette palette;
  palette.setColor(QPalette::Window, QColor(250, 250, 250));
  palette.setColor(QPalette::WindowText, QColor(30, 30, 30));
  palette.setColor(QPalette::Base, QColor(255, 255, 255));
  palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
  palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
  palette.setColor(QPalette::ToolTipText, QColor(20, 20, 20));
  palette.setColor(QPalette::Text, QColor(30, 30, 30));
  palette.setColor(QPalette::Button, QColor(245, 245, 245));
  palette.setColor(QPalette::ButtonText, QColor(30, 30, 30));
  palette.setColor(QPalette::Highlight, QColor(50, 120, 220));
  palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
  return palette;
}

const QString& systemStyleName() {
  static const QString styleName = QApplication::style() != nullptr ? QApplication::style()->objectName() : "";
  return styleName;
}

const QPalette& systemPalette() {
  static const QPalette palette = QApplication::palette();
  return palette;
}

std::int64_t retentionCutoffTimestampMs(int retentionDays) {
  if (retentionDays <= 0) {
    return 0;
  }

  constexpr std::int64_t kDayMs = 24LL * 60LL * 60LL * 1000LL;
  return QDateTime::currentMSecsSinceEpoch() - static_cast<std::int64_t>(retentionDays) * kDayMs;
}

}  // namespace

TrayAppController::TrayAppController(std::shared_ptr<IHistoryRepository> repository, QObject* parent)
    : QObject(parent),
      repository_(std::move(repository)),
      service_(repository_, PrivacyRuleSet{}),
      trayViewModel_(service_, settings_),
      settingsFilePath_(defaultSettingsFilePath()) {
  loadSettings();
  applyTheme();
  applyStartupEntry();
}

void TrayAppController::start() {
  createTray();
#ifdef COPYCLICKK_HAS_KGUIADDONS
  QObject::connect(KSystemClipboard::instance(),
                   &KSystemClipboard::changed,
                   this,
                   [this](QClipboard::Mode mode) { onClipboardModeChanged(mode); });
#else
  QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() { onClipboardChanged(); });
  QObject::connect(QApplication::clipboard(),
                   &QClipboard::changed,
                   this,
                   [this](QClipboard::Mode mode) { onClipboardModeChanged(mode); });
#endif

  clipboardPollTimer_ = new QTimer(this);
  clipboardPollTimer_->setInterval(75);
  QObject::connect(clipboardPollTimer_, &QTimer::timeout, this, [this]() {
    processClipboardMode(QClipboard::Clipboard);
  });
  clipboardPollTimer_->start();

  registerOpenHistoryShortcut();
  enforceRetentionPolicy();
  const auto recent = service_.listRecent(1);
  if (!recent.empty()) {
    lastCapturedItem_ = recent.front();
  }
  trayIcon_.show();
}

void TrayAppController::showHistoryFromExternalTrigger() {
  onShowHistoryTriggered();
}

void TrayAppController::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
  if (reason == QSystemTrayIcon::Trigger) {
    trayViewModel_.onTrayIconClicked();
    onToggleHistoryShortcutTriggered();
  }
}

void TrayAppController::onShowHistoryTriggered() {
  trayViewModel_.onShowHistoryClicked();
  ensureHistoryWindow();
  refreshHistoryList();
  historyWindow_->show();
  historyWindow_->raise();
  historyWindow_->activateWindow();
}

void TrayAppController::onClearClipboardTriggered() {
  if (settings_.confirmBeforeClearAll()) {
    const auto result = QMessageBox::question(historyWindow_,
                                              "Clear Clipboard History",
                                              "Delete all unpinned clipboard items?",
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
    if (result != QMessageBox::Yes) {
      return;
    }
  }

  trayViewModel_.onClearClipboardClicked();
  suppressClipboardCapture_ = true;
  QApplication::clipboard()->clear();
  suppressClipboardCapture_ = false;
  refreshHistoryList();
}

void TrayAppController::onSettingsTriggered() {
  trayViewModel_.onSettingsClicked();
  ensureSettingsDialog();
  settingsDialog_->show();
  settingsDialog_->raise();
  settingsDialog_->activateWindow();
}

void TrayAppController::onToggleHistoryShortcutTriggered() {
  if (historyWindow_ != nullptr && historyWindow_->isVisible()) {
    historyWindow_->hide();
    trayViewModel_.onHistoryClosed();
    return;
  }

  onShowHistoryTriggered();
}

void TrayAppController::onClipboardChanged() {
  processClipboardMode(QClipboard::Clipboard);
}

void TrayAppController::onClipboardModeChanged(QClipboard::Mode mode) {
  processClipboardMode(mode);
}

void TrayAppController::processClipboardMode(QClipboard::Mode mode) {
  // Only store regular clipboard updates. Monitoring primary selection can
  // produce repeated/alternating entries on Wayland compositors.
  if (mode != QClipboard::Clipboard) {
    return;
  }

  if (suppressClipboardCapture_) {
    return;
  }

  ClipboardItem item;
#ifdef COPYCLICKK_HAS_KGUIADDONS
  const QMimeData* mimeData = KSystemClipboard::instance()->mimeData(mode);
#else
  const QMimeData* mimeData = QApplication::clipboard()->mimeData(mode);
#endif
  if (!buildClipboardItemFromMimeData(mimeData, &item)) {
    return;
  }

  const std::int64_t nowMs = QDateTime::currentMSecsSinceEpoch();
  if (nowMs - lastRetentionSweepMs_ >= 10'000) {
    enforceRetentionPolicy();
    lastRetentionSweepMs_ = nowMs;
  }

  if (settings_.skipConsecutiveDuplicates() && isConsecutiveDuplicate(item)) {
    return;
  }

  // Some backends can emit duplicate notifications for the same content in a
  // short burst. Ignore those rapid repeats even if duplicate setting is off.
  if (isConsecutiveDuplicate(item)) {
    constexpr std::int64_t kDuplicateBurstWindowMs = 1200;
    if (nowMs - lastCaptureEventMs_ <= kDuplicateBurstWindowMs) {
      return;
    }
  }

  if (!service_.ingestWithLimit(item, static_cast<std::size_t>(settings_.historyLimit()))) {
    return;
  }

  lastCapturedItem_ = item;
  lastCaptureEventMs_ = nowMs;

  if (trayViewModel_.isHistoryVisible()) {
    refreshHistoryList();
  }
}

void TrayAppController::onHistoryItemActivated(QListWidgetItem* item) {
  if (item == nullptr) {
    return;
  }

  const QVariant idData = item->data(Qt::UserRole);
  if (!idData.isValid()) {
    return;
  }

  applyItemToClipboard(idData.toLongLong());

  if (historyWindow_ != nullptr) {
    historyWindow_->hide();
    trayViewModel_.onHistoryClosed();
  }
}

void TrayAppController::createTray() {
  trayIcon_.setToolTip(QString::fromStdString(trayViewModel_.appNameLabel()));
  trayIcon_.setIcon(resolveTrayIcon());

  showHistoryAction_ = trayMenu_.addAction(QString::fromStdString(trayViewModel_.showHistoryLabel()));
  clearClipboardAction_ = trayMenu_.addAction(QString::fromStdString(trayViewModel_.clearClipboardLabel()));
  settingsAction_ = trayMenu_.addAction(QString::fromStdString(trayViewModel_.settingsLabel()));
  trayMenu_.addSeparator();
  quitAction_ = trayMenu_.addAction("Quit");

  QObject::connect(showHistoryAction_, &QAction::triggered, this, &TrayAppController::onShowHistoryTriggered);
  QObject::connect(clearClipboardAction_, &QAction::triggered, this, &TrayAppController::onClearClipboardTriggered);
  QObject::connect(settingsAction_, &QAction::triggered, this, &TrayAppController::onSettingsTriggered);
  QObject::connect(quitAction_, &QAction::triggered, qApp, &QApplication::quit);
  QObject::connect(&trayIcon_, &QSystemTrayIcon::activated, this, &TrayAppController::onTrayActivated);

  trayIcon_.setContextMenu(&trayMenu_);
}

void TrayAppController::ensureHistoryWindow() {
  if (historyWindow_ != nullptr) {
    return;
  }

  auto* window = new HistoryWindow();
  window->setWindowTitle("Clipboard History");
  window->resize(420, 500);

  auto* layout = new QVBoxLayout(window);
  historyList_ = new QListWidget(window);
  auto* clearButton = new QPushButton(QString::fromStdString(trayViewModel_.clearClipboardLabel()), window);
  auto* settingsButton = new QPushButton(QString::fromStdString(trayViewModel_.settingsLabel()), window);

  layout->addWidget(new QLabel("Recent clipboard items", window));
  layout->addWidget(historyList_);
  layout->addWidget(clearButton);
  layout->addWidget(settingsButton);

  QObject::connect(clearButton, &QPushButton::clicked, this, &TrayAppController::onClearClipboardTriggered);
  QObject::connect(settingsButton, &QPushButton::clicked, this, &TrayAppController::onSettingsTriggered);
  QObject::connect(historyList_, &QListWidget::itemClicked, this, &TrayAppController::onHistoryItemActivated);
  QObject::connect(historyList_, &QListWidget::itemActivated, this, &TrayAppController::onHistoryItemActivated);

  window->onClosed = [this]() { trayViewModel_.onHistoryClosed(); };
  historyWindow_ = window;
}

void TrayAppController::applyItemToClipboard(std::int64_t itemId) {
  const auto item = service_.getItem(itemId);
  if (!item.has_value()) {
    return;
  }

  ClipboardItem selected = *item;
  QMimeData* mimeData = mimeDataFromItem(selected);
  if (mimeData == nullptr) {
    return;
  }

  suppressClipboardCapture_ = true;
  QApplication::clipboard()->setMimeData(mimeData);
  suppressClipboardCapture_ = false;
}

QMimeData* TrayAppController::mimeDataFromItem(const ClipboardItem& item) const {
  auto* mimeData = new QMimeData();

  if (settings_.pasteAsPlainText() &&
      (item.mimeType == ClipboardMimeType::TextPlain || item.mimeType == ClipboardMimeType::TextHtml ||
       item.mimeType == ClipboardMimeType::TextUriList)) {
    mimeData->setText(previewTextForItem(item));
    return mimeData;
  }

  if (item.mimeType == ClipboardMimeType::TextPlain) {
    mimeData->setText(QString::fromUtf8(reinterpret_cast<const char*>(item.payload.data()),
                                        static_cast<int>(item.payload.size())));
    return mimeData;
  }

  if (item.mimeType == ClipboardMimeType::TextHtml) {
    mimeData->setHtml(QString::fromUtf8(reinterpret_cast<const char*>(item.payload.data()),
                                        static_cast<int>(item.payload.size())));
    return mimeData;
  }

  if (item.mimeType == ClipboardMimeType::TextUriList) {
    const QString text = QString::fromUtf8(reinterpret_cast<const char*>(item.payload.data()),
                                           static_cast<int>(item.payload.size()));
    const QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    QList<QUrl> urls;
    urls.reserve(lines.size());
    for (const QString& line : lines) {
      urls.push_back(QUrl(line));
    }
    mimeData->setUrls(urls);
    mimeData->setText(text);
    return mimeData;
  }

  if (isImageMimeType(item.mimeType)) {
    QImage image;
    image.loadFromData(reinterpret_cast<const uchar*>(item.payload.data()), static_cast<int>(item.payload.size()));
    if (!image.isNull()) {
      mimeData->setImageData(image);
      return mimeData;
    }
  }

  delete mimeData;
  return nullptr;
}

void TrayAppController::ensureSettingsDialog() {
  if (settingsDialog_ != nullptr) {
    return;
  }

  auto* dialog = new SettingsDialog();
  dialog->setWindowTitle("Settings");
  dialog->resize(460, 640);

  auto* layout = new QVBoxLayout(dialog);

  auto* historySection = new QGroupBox("History", dialog);
  auto* historyLayout = new QVBoxLayout(historySection);
  historyLayout->addWidget(new QLabel("Clipboard history limit", historySection));
  historyLimitSpinBox_ = new QSpinBox(historySection);
  historyLimitSpinBox_->setRange(1, 5000);
  historyLayout->addWidget(historyLimitSpinBox_);

  historyLayout->addWidget(new QLabel("Retention period", historySection));
  retentionPeriodComboBox_ = new QComboBox(historySection);
  retentionPeriodComboBox_->addItem("Always", 0);
  retentionPeriodComboBox_->addItem("1 week (7 days)", 7);
  retentionPeriodComboBox_->addItem("1 month (30 days)", 30);
  retentionPeriodComboBox_->addItem("3 months (90 days)", 90);
  retentionPeriodComboBox_->addItem("6 months (180 days)", 180);
  historyLayout->addWidget(retentionPeriodComboBox_);

  skipConsecutiveDuplicatesCheckBox_ = new QCheckBox("Skip consecutive duplicates", historySection);
  historyLayout->addWidget(skipConsecutiveDuplicatesCheckBox_);
  layout->addWidget(historySection);

  auto* clipboardSection = new QGroupBox("Clipboard", dialog);
  auto* clipboardLayout = new QVBoxLayout(clipboardSection);
  pasteAsPlainTextCheckBox_ = new QCheckBox("Paste text items as plain text", clipboardSection);
  clipboardLayout->addWidget(pasteAsPlainTextCheckBox_);
  confirmClearAllCheckBox_ = new QCheckBox("Ask confirmation before Clear Clipboard", clipboardSection);
  clipboardLayout->addWidget(confirmClearAllCheckBox_);
  layout->addWidget(clipboardSection);

  auto* imagesSection = new QGroupBox("Images", dialog);
  auto* imagesLayout = new QVBoxLayout(imagesSection);
  imagesLayout->addWidget(new QLabel("Image preview size", imagesSection));
  thumbnailSizeSpinBox_ = new QSpinBox(imagesSection);
  thumbnailSizeSpinBox_->setRange(16, 128);
  imagesLayout->addWidget(thumbnailSizeSpinBox_);

  saveImagesCheckBox_ = new QCheckBox("Save image entries", imagesSection);
  imagesLayout->addWidget(saveImagesCheckBox_);
  compressImagesCheckBox_ = new QCheckBox("Compress saved images", imagesSection);
  imagesLayout->addWidget(compressImagesCheckBox_);
  imagesLayout->addWidget(new QLabel("Image compression quality (JPEG)", imagesSection));
  imageCompressionQualitySpinBox_ = new QSpinBox(imagesSection);
  imageCompressionQualitySpinBox_->setRange(30, 100);
  imagesLayout->addWidget(imageCompressionQualitySpinBox_);
  layout->addWidget(imagesSection);

  auto* startupSection = new QGroupBox("System", dialog);
  auto* startupLayout = new QVBoxLayout(startupSection);
  startAtLoginCheckBox_ = new QCheckBox("Start with system", startupSection);
  startupLayout->addWidget(startAtLoginCheckBox_);
  layout->addWidget(startupSection);

  auto* appearanceSection = new QGroupBox("Appearance", dialog);
  auto* appearanceLayout = new QVBoxLayout(appearanceSection);
  appearanceLayout->addWidget(new QLabel("Theme", appearanceSection));
  themeComboBox_ = new QComboBox(appearanceSection);
  themeComboBox_->addItem("System", QStringLiteral("system"));
  themeComboBox_->addItem("Light", QStringLiteral("light"));
  themeComboBox_->addItem("Dark", QStringLiteral("dark"));
  appearanceLayout->addWidget(themeComboBox_);
  layout->addWidget(appearanceSection);

  auto* shortcutSection = new QGroupBox("Shortcuts", dialog);
  auto* shortcutLayout = new QVBoxLayout(shortcutSection);
  shortcutLayout->addWidget(new QLabel(QString::fromStdString(trayViewModel_.openHistoryShortcutLabel()), shortcutSection));
  openHistoryShortcutEdit_ = new QKeySequenceEdit(shortcutSection);
  shortcutLayout->addWidget(openHistoryShortcutEdit_);
  layout->addWidget(shortcutSection);

  auto applySettingsToControls = [this](const SettingsModel& model) {
    historyLimitSpinBox_->setValue(model.historyLimit());
    thumbnailSizeSpinBox_->setValue(model.thumbnailSizePx());

    const int configuredRetentionDays = model.retentionDays();
    int retentionIndex = retentionPeriodComboBox_->findData(configuredRetentionDays);
    if (retentionIndex < 0) {
      retentionPeriodComboBox_->addItem(QString("Custom (%1 days)").arg(configuredRetentionDays), configuredRetentionDays);
      retentionIndex = retentionPeriodComboBox_->count() - 1;
    }
    retentionPeriodComboBox_->setCurrentIndex(retentionIndex);

    skipConsecutiveDuplicatesCheckBox_->setChecked(model.skipConsecutiveDuplicates());
    pasteAsPlainTextCheckBox_->setChecked(model.pasteAsPlainText());
    saveImagesCheckBox_->setChecked(model.saveImages());
    compressImagesCheckBox_->setChecked(model.compressImages());
    imageCompressionQualitySpinBox_->setValue(model.imageCompressionQuality());
    startAtLoginCheckBox_->setChecked(model.startAtLogin());
    confirmClearAllCheckBox_->setChecked(model.confirmBeforeClearAll());

    const int themeIndex = themeComboBox_->findData(QString::fromStdString(model.theme()));
    themeComboBox_->setCurrentIndex(themeIndex >= 0 ? themeIndex : 0);
    openHistoryShortcutEdit_->setKeySequence(QKeySequence(QString::fromStdString(model.openHistoryShortcut())));
  };
  applySettingsToControls(settings_);

  auto updateImageControls = [this]() {
    const bool saveImages = saveImagesCheckBox_ != nullptr && saveImagesCheckBox_->isChecked();
    const bool compressImages = compressImagesCheckBox_ != nullptr && compressImagesCheckBox_->isChecked();
    if (compressImagesCheckBox_ != nullptr) {
      compressImagesCheckBox_->setEnabled(saveImages);
    }
    if (imageCompressionQualitySpinBox_ != nullptr) {
      imageCompressionQualitySpinBox_->setEnabled(saveImages && compressImages);
    }
  };
  QObject::connect(saveImagesCheckBox_, &QCheckBox::toggled, dialog, [updateImageControls](bool) { updateImageControls(); });
  QObject::connect(compressImagesCheckBox_,
                   &QCheckBox::toggled,
                   dialog,
                   [updateImageControls](bool) { updateImageControls(); });
  updateImageControls();

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
  auto* resetButton = buttons->addButton("Reset to Defaults", QDialogButtonBox::ResetRole);
  layout->addWidget(buttons);

  QObject::connect(resetButton, &QPushButton::clicked, dialog, [applySettingsToControls]() {
    const SettingsModel defaults;
    applySettingsToControls(defaults);
  });

  QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog]() {
    trayViewModel_.updateHistoryLimit(historyLimitSpinBox_->value());
    settings_.setThumbnailSizePx(thumbnailSizeSpinBox_->value());
    settings_.setRetentionDays(retentionPeriodComboBox_->currentData().toInt());
    settings_.setSkipConsecutiveDuplicates(skipConsecutiveDuplicatesCheckBox_->isChecked());
    settings_.setPasteAsPlainText(pasteAsPlainTextCheckBox_->isChecked());
    settings_.setSaveImages(saveImagesCheckBox_->isChecked());
    settings_.setCompressImages(compressImagesCheckBox_->isChecked());
    settings_.setImageCompressionQuality(imageCompressionQualitySpinBox_->value());
    settings_.setStartAtLogin(startAtLoginCheckBox_->isChecked());
    settings_.setConfirmBeforeClearAll(confirmClearAllCheckBox_->isChecked());
    settings_.setTheme(themeComboBox_->currentData().toString().toStdString());
    trayViewModel_.updateOpenHistoryShortcut(openHistoryShortcutEdit_->keySequence().toString().toStdString());
    enforceRetentionPolicy();
    applyTheme();
    applyStartupEntry();
    registerOpenHistoryShortcut();
    saveSettings();
    dialog->accept();
    refreshHistoryList();
  });
  QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

  dialog->onClosed = [this]() { trayViewModel_.onSettingsClosed(); };
  settingsDialog_ = dialog;
}

void TrayAppController::populateHistoryList(QListWidget* listWidget) {
  if (listWidget == nullptr) {
    return;
  }

  listWidget->clear();
  listWidget->setIconSize(QSize(settings_.thumbnailSizePx(), settings_.thumbnailSizePx()));

  for (const auto& item : trayViewModel_.recentItems()) {
    QString preview;
    if (isImageMimeType(item.mimeType)) {
      preview = "[Image]";
    } else if (item.mimeType == ClipboardMimeType::TextPlain || item.mimeType == ClipboardMimeType::TextHtml ||
               item.mimeType == ClipboardMimeType::TextUriList) {
      preview = truncatedPreview(previewTextForItem(item), 60);
    } else {
      preview = "[Non-text item]";
    }

    auto* rowWidget = new QWidget(listWidget);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(8, 8, 8, 8);
    rowLayout->setSpacing(10);

    if (isImageMimeType(item.mimeType)) {
      const QPixmap pixmap = pixmapFromItem(item);
      if (!pixmap.isNull()) {
        auto* iconLabel = new QLabel(rowWidget);
        const QPixmap scaled = pixmap.scaled(settings_.thumbnailSizePx(), settings_.thumbnailSizePx(), Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);
        iconLabel->setPixmap(scaled);
        rowLayout->addWidget(iconLabel);
      }
    }

    auto* textLabel = new QLabel(QString(item.pinned ? "[Pinned] " : "") + preview, rowWidget);
    textLabel->setWordWrap(true);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    textLabel->setMinimumHeight(34);
    rowLayout->addWidget(textLabel);

    auto* pinButton = new QToolButton(rowWidget);
    pinButton->setIcon(resolvePinIcon(item.pinned));
    pinButton->setIconSize(QSize(18, 18));
    pinButton->setToolTip(item.pinned ? "Unpin item" : "Pin item");
    pinButton->setAutoRaise(true);
    rowLayout->addWidget(pinButton);

    auto* deleteButton = new QToolButton(rowWidget);
    deleteButton->setIcon(resolveDeleteIcon());
    deleteButton->setIconSize(QSize(18, 18));
    deleteButton->setToolTip("Delete item");
    deleteButton->setAutoRaise(true);
    rowLayout->addWidget(deleteButton);

    const std::int64_t itemId = item.id;
    const bool currentlyPinned = item.pinned;
    QObject::connect(pinButton, &QToolButton::clicked, this, [this, itemId, currentlyPinned]() {
      if (trayViewModel_.onPinItemClicked(itemId, !currentlyPinned)) {
        refreshHistoryList();
      }
    });
    QObject::connect(deleteButton, &QToolButton::clicked, this, [this, itemId]() {
      if (trayViewModel_.onDeleteItemClicked(itemId)) {
        refreshHistoryList();
      }
    });

    auto* rowItem = new QListWidgetItem();
    rowWidget->setMinimumHeight(50);
    rowItem->setData(Qt::UserRole, static_cast<qlonglong>(item.id));
    rowItem->setSizeHint(rowWidget->sizeHint());
    listWidget->addItem(rowItem);
    listWidget->setItemWidget(rowItem, rowWidget);
  }
}

void TrayAppController::refreshHistoryList() {
  populateHistoryList(historyList_);
}

bool TrayAppController::buildClipboardItemFromMimeData(const QMimeData* mimeData, ClipboardItem* item) const {
  if (mimeData == nullptr || item == nullptr) {
    return false;
  }

  item->timestampMs = QDateTime::currentMSecsSinceEpoch();
  item->sourceApp = "qt-app";

  if (settings_.saveImages() && mimeData->hasImage()) {
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());
    if (!image.isNull()) {
      return encodeImageToItem(image, item);
    }
  }

  if (mimeData->hasText()) {
    const std::string text = mimeData->text().toStdString();
    if (!text.empty()) {
      item->mimeType = ClipboardMimeType::TextPlain;
      item->payload.assign(text.begin(), text.end());
      return true;
    }
  }

  if (mimeData->hasHtml()) {
    const std::string html = mimeData->html().toStdString();
    if (!html.empty()) {
      item->mimeType = ClipboardMimeType::TextHtml;
      item->payload.assign(html.begin(), html.end());
      return true;
    }
  }

  if (mimeData->hasUrls()) {
    const auto urls = mimeData->urls();
    if (!urls.isEmpty()) {
      const auto first = urls.first();
      if (settings_.saveImages() && first.isLocalFile()) {
        const QString localPath = first.toLocalFile();
        QImageReader reader(localPath);
        if (reader.canRead()) {
          const QImage image = reader.read();
          if (!image.isNull()) {
            return encodeImageToItem(image, item);
          }
        }
      }

      std::string joined;
      for (int i = 0; i < urls.size(); ++i) {
        if (i > 0) {
          joined += "\n";
        }
        joined += urls[i].toString().toStdString();
      }

      item->mimeType = ClipboardMimeType::TextUriList;
      item->payload.assign(joined.begin(), joined.end());
      return !item->payload.empty();
    }
  }

  return false;
}

void TrayAppController::registerOpenHistoryShortcut() {
#ifdef COPYCLICKK_HAS_KGLOBALACCEL
  if (globalShowHistoryAction_ == nullptr) {
    globalShowHistoryAction_ = new QAction(QString::fromStdString(trayViewModel_.showHistoryLabel()), this);
    globalShowHistoryAction_->setObjectName("copyclickk.show-history");
    QObject::connect(globalShowHistoryAction_,
                     &QAction::triggered,
                     this,
                     &TrayAppController::onToggleHistoryShortcutTriggered);
  }

  QKeySequence sequence = QKeySequence(QString::fromStdString(trayViewModel_.openHistoryShortcut()));
  if (sequence.isEmpty()) {
    sequence = QKeySequence(QStringLiteral("Ctrl+Alt+V"));
    trayViewModel_.updateOpenHistoryShortcut(sequence.toString().toStdString());
    saveSettings();
  }
  QList<QKeySequence> shortcuts;
  if (!sequence.isEmpty()) {
    shortcuts.push_back(sequence);
  }
  KGlobalAccel::setGlobalShortcut(globalShowHistoryAction_, shortcuts);
  return;
#else
  if (shortcutHost_ == nullptr) {
    shortcutHost_ = new QWidget();
    shortcutHost_->setWindowTitle("ShortcutHost");
    shortcutHost_->hide();
  }

  if (openHistoryShortcut_ != nullptr) {
    openHistoryShortcut_->deleteLater();
    openHistoryShortcut_ = nullptr;
  }

  QKeySequence sequence = QKeySequence(QString::fromStdString(trayViewModel_.openHistoryShortcut()));
  if (sequence.isEmpty()) {
    sequence = QKeySequence(QStringLiteral("Ctrl+Alt+V"));
    trayViewModel_.updateOpenHistoryShortcut(sequence.toString().toStdString());
    saveSettings();
  }
  if (sequence.isEmpty()) {
    return;
  }

  openHistoryShortcut_ = new QShortcut(sequence, shortcutHost_);
  openHistoryShortcut_->setContext(Qt::ApplicationShortcut);
  QObject::connect(openHistoryShortcut_,
                   &QShortcut::activated,
                   this,
                   &TrayAppController::onToggleHistoryShortcutTriggered);
#endif
}

void TrayAppController::applyTheme() {
  if (settings_.theme() == "dark") {
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setPalette(darkPalette());
  } else if (settings_.theme() == "light") {
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QApplication::setPalette(lightPalette());
  } else {
    const QString styleName = systemStyleName();
    if (!styleName.isEmpty()) {
      QApplication::setStyle(QStyleFactory::create(styleName));
    }
    QApplication::setPalette(systemPalette());
  }

  for (QWidget* topLevel : QApplication::topLevelWidgets()) {
    if (topLevel == nullptr) {
      continue;
    }
    topLevel->setPalette(QApplication::palette());
    topLevel->update();
  }
}

void TrayAppController::applyStartupEntry() {
  const std::string desktopFilePath = autostartDesktopFilePath();
  if (settings_.startAtLogin()) {
    writeAutostartFile(desktopFilePath);
    return;
  }

  std::error_code ec;
  std::filesystem::remove(desktopFilePath, ec);
}

void TrayAppController::enforceRetentionPolicy() {
  const std::int64_t cutoff = retentionCutoffTimestampMs(settings_.retentionDays());
  if (cutoff <= 0) {
    return;
  }
  service_.removeOlderThan(cutoff);
}

bool TrayAppController::isConsecutiveDuplicate(const ClipboardItem& item) const {
  if (!lastCapturedItem_.has_value()) {
    return false;
  }

  return lastCapturedItem_->mimeType == item.mimeType && lastCapturedItem_->payload == item.payload;
}

bool TrayAppController::encodeImageToItem(const QImage& image, ClipboardItem* item) const {
  if (item == nullptr || image.isNull()) {
    return false;
  }

  QByteArray bytes;
  QBuffer buffer(&bytes);
  if (!buffer.open(QIODevice::WriteOnly)) {
    return false;
  }

  if (settings_.compressImages()) {
    if (!image.save(&buffer, "JPEG", settings_.imageCompressionQuality())) {
      return false;
    }
    item->mimeType = ClipboardMimeType::ImageJpeg;
  } else {
    if (!image.save(&buffer, "PNG")) {
      return false;
    }
    item->mimeType = ClipboardMimeType::ImagePng;
  }

  item->payload.assign(bytes.begin(), bytes.end());
  return !item->payload.empty();
}

void TrayAppController::loadSettings() {
  (void)settings_.loadFromFile(settingsFilePath_);
}

void TrayAppController::saveSettings() const {
  (void)settings_.saveToFile(settingsFilePath_);
}

}  // namespace copyclickk
