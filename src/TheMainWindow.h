#ifndef THEMAINWINDOW_H_
#define THEMAINWINDOW_H_

#include <QStandardItemModel>
#include <QMainWindow>
#include <QLabel>
#include <QGraphicsView>
#include <QGraphicsScene>
#include "ui_mixing-app.h"

class TheMainWindow: public QMainWindow
{
	Q_OBJECT

public:
	TheMainWindow();
	virtual ~TheMainWindow();

private slots:
	void onImport();
	void onPlayOrPause();

protected:
	Ui::MainWindow w;

private:
	QLabel * statusLabel;
	QStandardItemModel * sim;
};

#endif /* THEMAINWINDOW_H_ */
