#pragma once

#include <QAudioOutput>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QMediaPlayer>
#include <QPushButton>
#include <QSlider>
#include <QStringList>
#include <QTableWidget>

namespace prs {

class AudioComparisonDialog : public QDialog {
    Q_OBJECT

  public:
    explicit AudioComparisonDialog(const QString& outputDir, QWidget* parent = nullptr);
    ~AudioComparisonDialog() override;

  private slots:
    void onPlayFile(int row);
    void onStopPlayback();
    void onPlayA();
    void onPlayB();
    void onToggleAB();
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 duration);
    void onSeek(int position);
    void onVolumeChanged(int value);
    void onPlayerStateChanged(QMediaPlayer::PlaybackState state);

  private:
    void populateFileList(const QString& dir);
    void loadMetrics(const QString& dir);
    void playFile(const QString& path);
    QString formatTime(qint64 ms) const;

    QStringList wavFiles_;
    QString outputDir_;

    QListWidget* fileList_ = nullptr;
    QComboBox* comboA_ = nullptr;
    QComboBox* comboB_ = nullptr;
    QPushButton* playABtn_ = nullptr;
    QPushButton* playBBtn_ = nullptr;
    QPushButton* toggleBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QSlider* seekSlider_ = nullptr;
    QSlider* volumeSlider_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QLabel* nowPlayingLabel_ = nullptr;

    QTableWidget* metricsTable_ = nullptr;

    QMediaPlayer* player_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    bool playingA_ = true;
};

} // namespace prs
