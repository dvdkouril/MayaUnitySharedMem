#pragma once

#include <QWidget>
#include <ui_StreamingWindow.h>

class StreamingWindow : public QWidget
{
	Q_OBJECT

public:
	explicit StreamingWindow(QWidget *parent = 0);
	~StreamingWindow();

	Ui::Form *ui;

};