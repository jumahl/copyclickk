#include "TrayAppController.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QStyle>
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

#include <cstdlib>
#include <filesystem>

#ifdef COPYCLICKK_HAS_KGLOBALACCEL
#include <KGlobalAccel>
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

}  // namespace

TrayAppController::TrayAppController(std::shared_ptr<IHistoryRepository> repository, QObject* parent)
    : QObject(parent),
      repository_(std::move(repository)),
      service_(repository_, PrivacyRuleSet{}),
      trayViewModel_(service_, settings_),
      settingsFilePath_(defaultSettingsFilePath()) {
  loadSettings();
}

void TrayAppController::start() {
  createTray();
  QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() { onClipboardChanged(); });
  registerOpenHistoryShortcut();
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
  if (suppressClipboardCapture_) {
    return;
  }

  ClipboardItem item;
  const QMimeData* mimeData = QApplication::clipboard()->mimeData();
  if (!buildClipboardItemFromMimeData(mimeData, &item)) {
    return;
  }

  if (!service_.ingestWithLimit(item, static_cast<std::size_t>(settings_.historyLimit()))) {
    return;
  }

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

  QMimeData* mimeData = mimeDataFromItem(*item);
  if (mimeData == nullptr) {
    return;
  }

  suppressClipboardCapture_ = true;
  QApplication::clipboard()->setMimeData(mimeData);
  suppressClipboardCapture_ = false;
}

QMimeData* TrayAppController::mimeDataFromItem(const ClipboardItem& item) const {
  auto* mimeData = new QMimeData();

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
  dialog->resize(320, 180);

  auto* layout = new QVBoxLayout(dialog);
  layout->addWidget(new QLabel("Clipboard history limit", dialog));

  historyLimitSpinBox_ = new QSpinBox(dialog);
  historyLimitSpinBox_->setRange(1, 5000);
  historyLimitSpinBox_->setValue(settings_.historyLimit());
  layout->addWidget(historyLimitSpinBox_);

  layout->addWidget(new QLabel("Image preview size", dialog));
  thumbnailSizeSpinBox_ = new QSpinBox(dialog);
  thumbnailSizeSpinBox_->setRange(16, 128);
  thumbnailSizeSpinBox_->setValue(settings_.thumbnailSizePx());
  layout->addWidget(thumbnailSizeSpinBox_);

  layout->addWidget(new QLabel(QString::fromStdString(trayViewModel_.openHistoryShortcutLabel()), dialog));
  openHistoryShortcutEdit_ = new QKeySequenceEdit(dialog);
  openHistoryShortcutEdit_->setKeySequence(QKeySequence(QString::fromStdString(trayViewModel_.openHistoryShortcut())));
  layout->addWidget(openHistoryShortcutEdit_);

  auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, dialog);
  layout->addWidget(buttons);

  QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, [this, dialog]() {
    trayViewModel_.updateHistoryLimit(historyLimitSpinBox_->value());
    settings_.setThumbnailSizePx(thumbnailSizeSpinBox_->value());
    trayViewModel_.updateOpenHistoryShortcut(openHistoryShortcutEdit_->keySequence().toString().toStdString());
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

  if (mimeData->hasImage()) {
    const QImage image = qvariant_cast<QImage>(mimeData->imageData());
    if (!image.isNull()) {
      QByteArray bytes;
      QBuffer buffer(&bytes);
      buffer.open(QIODevice::WriteOnly);
      image.save(&buffer, "PNG");
      item->mimeType = ClipboardMimeType::ImagePng;
      item->payload.assign(bytes.begin(), bytes.end());
      return !item->payload.empty();
    }
  }

  if (mimeData->hasUrls()) {
    const auto urls = mimeData->urls();
    if (!urls.isEmpty()) {
      const auto first = urls.first();
      if (first.isLocalFile()) {
        const QString localPath = first.toLocalFile();
        QImageReader reader(localPath);
        if (reader.canRead()) {
          QFile file(localPath);
          if (file.open(QIODevice::ReadOnly)) {
            const QByteArray bytes = file.readAll();
            if (!bytes.isEmpty()) {
              item->mimeType = imageMimeTypeFromSuffix(QFileInfo(localPath).suffix());
              item->payload.assign(bytes.begin(), bytes.end());
              return true;
            }
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

  if (mimeData->hasHtml()) {
    const std::string html = mimeData->html().toStdString();
    if (!html.empty()) {
      item->mimeType = ClipboardMimeType::TextHtml;
      item->payload.assign(html.begin(), html.end());
      return true;
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

  const auto sequence = QKeySequence(QString::fromStdString(trayViewModel_.openHistoryShortcut()));
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

  const auto sequence = QKeySequence(QString::fromStdString(trayViewModel_.openHistoryShortcut()));
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

void TrayAppController::loadSettings() {
  (void)settings_.loadFromFile(settingsFilePath_);
}

void TrayAppController::saveSettings() const {
  (void)settings_.saveToFile(settingsFilePath_);
}

}  // namespace copyclickk
