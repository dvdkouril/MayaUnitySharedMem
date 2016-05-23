#include "StreamingWindow.h"
#include <iostream>

StreamingWindow::StreamingWindow(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Form)
{
	ui->setupUi(this);
}

StreamingWindow::~StreamingWindow()
{
	delete ui;
}