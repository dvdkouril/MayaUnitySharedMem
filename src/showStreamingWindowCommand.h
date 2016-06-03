#pragma once

#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>

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

		StreamingWindow *pluginWindow = new StreamingWindow();
		//StreamingWindow * pluginWindow = new StreamingWindow(qApp->activeWindow()); // this makes the elements "merge" into the main window
		pluginWindow->setWindowFlags(Qt::WindowStaysOnTopHint);
		QObject::connect(pluginWindow->ui->startButton, SIGNAL(clicked(bool)), pluginWindow, SLOT(startStreaming()));
		QObject::connect(pluginWindow->ui->stopButton, SIGNAL(clicked(bool)), pluginWindow, SLOT(stopStreaming()));
		pluginWindow->show();


		// find custom locator object in the scene
		MSelectionList list;
		MDagPath pathToCustomLocator;
		list.add("CustomLocator");
		list.getDagPath(0, pathToCustomLocator);
		CustomLocator *locator = new CustomLocator();

		return MS::kSuccess;
	}


	static void* creator() 
	{
		return new showStreamingWindowCommand;
	}

};