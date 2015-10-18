#include "TheMainWindow.h"
#include <QListWidgetItem>
#include <QFileDialog>
#include <QApplication>
#include "backend/core/AudioSink.h"
#include "backend/core/AudioFile.h"
#include "backend/core/RequestQueue.h"
#include "backend/core/xfade/DJCrossfadeCalculator.h"
#include "backend/core/xfade/fademaps/KneeFadeMap.h"
#include "backend/core/filters/CubicInterpFilter.h"
#include "backend/util/StrUtil.h"
#include <string>

class MyRequestQueue : public RequestQueue
{
	QLabel * label;
	MyRequestQueue(QLabel * label) : RequestQueue(), label(label) {}

	void BreakUpTime(double time, int & minute, double & second)
	{
		minute = floor(time / 60.0);
		second = time - 60.0 * minute;
	}
public:
	virtual ~MyRequestQueue() {}

	static MyRequestQueue & Instance(QLabel * label)
	{
		static MyRequestQueue inst(label);
		return inst;
	}

	void OnMetadataLoaded(const AudioFile & file)
	{
		std::string str = "Now playing: " + file.getTitle();
		if (!file.getArtist().empty())
		{
			str += " by " + file.getArtist();
		}
		label->setText(QString(str.c_str()));
	}

	void OnPositionUpdate(const AudioFile & file)
	{
		std::string str = "Now playing: " + file.getTitle();
		if (!file.getArtist().empty())
		{
			str += " by " + file.getArtist();
		}

		int positionMin, durationMin;
		double positionSec, durationSec;
		BreakUpTime(file.getPosition(), positionMin, positionSec);
		BreakUpTime(file.getDuration(), durationMin, durationSec);
		str += " (Position: "
			+ StrUtil::format("%02d:%05.2f/%02d:%05.2f",
					  positionMin, positionSec,
					  durationMin, durationSec) + ')';
		label->setText(QString(str.c_str()));
	}
};

TheMainWindow::TheMainWindow()
{
	w.setupUi(this);
	statusLabel = new QLabel(tr("Ready"));
	w.statusbar->addWidget(statusLabel, 1);

	connect(w.btnImport, SIGNAL(clicked()), this, SLOT(onImport()));
	connect(w.btnPlay, SIGNAL(clicked()), this, SLOT(onPlayOrPause()));
}

TheMainWindow::~TheMainWindow()
{
}

void TheMainWindow::onImport()
{
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::ExistingFiles);
	dialog.setNameFilter(tr("Audio Files (*.mp3 *.ogg *.wav *.flac)"));

	QStringList filenames;
	if (dialog.exec())
	{
		filenames = dialog.selectedFiles();
		Q_FOREACH(const QString & filename, filenames)
		{
			QListWidgetItem * item = new QListWidgetItem(filename);
			w.listWidget->addItem(item);
		}
	}
}

void TheMainWindow::onPlayOrPause()
{
	MyRequestQueue & reqQueue = MyRequestQueue::Instance(statusLabel);
	if (w.btnPlay->text() == tr("Play"))
	{
		// Start playing
//		w.btnPlay->setText(tr("Pause"));
		w.btnPlay->setText(tr("Close"));

		if (AudioSink::Instance().StartSink())
		{
			std::unique_ptr<CubicInterpFilter> filter(new CubicInterpFilter);
			filter->SetTimeInterval(0.0005);
			AudioSink::Instance().takeClickRemovalFilter(std::move(filter));

			MyRequestQueue & reqQueue = MyRequestQueue::Instance(statusLabel);
			std::unique_ptr<KneeFadeMap> fadeMap(new KneeFadeMap);
			reqQueue.TakeFadeMap(std::move(fadeMap));

			AudioFile::InitializeAvformat();
			reqQueue.StartRequestProcessor();
			reqQueue.SetDJXfadeEnabled(true);
		}

		for (int row = 0; row < w.listWidget->count(); ++row)
		{
			QListWidgetItem * item = w.listWidget->item(row);
			QByteArray array = item->text().toLocal8Bit();
			reqQueue.Play(array.data());
		}
	}
	else
	{
		// Close
		reqQueue.StopRequestProcessorAndSink();
		QApplication::exit(0);
	}
#if 0
	else
	{
		// Pause
		w.btnPlay->setText(tr("Play"));

		// Stop
		w.btnPlay->setText(tr("Close"));
		reqQueue.StopRequestProcessorAndSink();
	}
#endif
}
