#pragma once

#include <maya/MPxCommand.h>

class startStreamingCommand : public MPxCommand
{
public:


	startStreamingCommand() {}
	~startStreamingCommand() {}

	virtual MStatus doIt(const MArgList&) 
	{
		/* TODO: MEL commands via MGlobal::executeCommand()
		- add customLocator into the scene
		- disable 'Start Streaming' menu item
		- enable 'End Streaming' menu item
		*/
		MGlobal::executeCommand("createNode \"customLocator\" -n \"streamingLocator\"");
		MGlobal::executeCommand("menuItem -edit -enable false startStreamingItem");
		MGlobal::executeCommand("menuItem -edit -enable true endStreamingItem");

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreamingCommand;
	}

};