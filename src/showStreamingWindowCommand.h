#pragma once

#include <maya/MPxCommand.h>

#include "StreamingWindow.h"

class showStreamingWindowCommand : public MPxCommand
{
public:
	showStreamingWindowCommand() {}
	~showStreamingWindowCommand() {}

	virtual MStatus doIt(const MArgList&) 
	{
		std::cerr << "Showing Streaming Window" << std::endl;

		// I guess that here comes the Qt code 

		StreamingWindow *pluginWin = new StreamingWindow();
		QObject::connect(pluginWin->ui->startButton, SIGNAL(clicked(bool)), pluginWin, SLOT(startStreaming()));
		QObject::connect(pluginWin->ui->stopButton, SIGNAL(clicked(bool)), pluginWin, SLOT(stopStreaming()));
		pluginWin->show();

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new showStreamingWindowCommand;
	}

};