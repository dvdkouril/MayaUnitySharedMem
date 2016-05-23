#include "StreamingWindow.h"

#include <iostream>

#include <maya/MGlobal.h>

StreamingWindow::StreamingWindow(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::Form)
{
	ui->setupUi(this);
}

StreamingWindow::~StreamingWindow()
{
	std::cerr << "StreamingWindow destructor has been called" << std::endl;
	delete ui;
}

void StreamingWindow::closeEvent(QCloseEvent *event)
{
	std::cerr << "StreamingWindow::closeEvent has been called" << std::endl;

	delete ui; // I think this should fix the crashing after window close? 
}

void StreamingWindow::startStreaming()
{
	MGlobal::executeCommand("startM2CVStreaming");
}

void StreamingWindow::stopStreaming()
{
	MGlobal::executeCommand("endM2CVStreaming");
}

void StreamingWindow::setFrameTimeLabel(float time)
{
	this->ui->frameTimeLabel->setText("something");
}