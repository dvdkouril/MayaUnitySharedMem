#pragma once

#include <maya/MPxCommand.h>

class endStreamingCommand : public MPxCommand
{
public:


	endStreamingCommand() {}
	~endStreamingCommand() {}

	virtual MStatus doIt(const MArgList&)
	{
		/* TODO: MEL commands via MGlobal::executeCommand()
		- delete customLocator from the scene
		- enable 'Start Streaming' menu item
		- disable 'End Streaming' menu item
		*/
		//MGlobal::executeCommand("delete streamingLocator"); // this doesn't delete the parent transform
		MGlobal::executeCommand("parent -removeObject streamingLocator"); 
		// this works but generates warning:
		// Warning: line 1: 'parent' command only operates on transform nodes. To parent shapes, use the -s/shape flag. // 

		MGlobal::executeCommand("menuItem -edit -enable true startStreamingItem");
		MGlobal::executeCommand("menuItem -edit -enable false endStreamingItem");

		return MS::kSuccess;
	}

	static void* creator()
	{
		return new endStreamingCommand;
	}

};