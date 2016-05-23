#pragma once

#include <QWidget>
#include <ui_StreamingWindow.h>

class StreamingWindow : public QWidget
{
	Q_OBJECT

protected:
	void closeEvent(QCloseEvent *event) override;

public:
	explicit StreamingWindow(QWidget *parent = 0);
	~StreamingWindow();

	void setFrameTimeLabel(float time);

	Ui::Form *ui;

public slots:
	void startStreaming();
	void stopStreaming();

};