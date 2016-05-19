#pragma once

#include <maya/MPxCommand.h>
#include <maya/MDagModifier.h>
#include <maya/MFnDependencyNode.h>

class startStreamingCommand : public MPxCommand
{
public:


	startStreamingCommand() {}
	~startStreamingCommand() {}

	virtual MStatus doIt(const MArgList&) 
	{
		std::cout << "Starting to stream to Unity..." << std::endl;
		/* TODO: MEL commands via MGlobal::executeCommand()
		- add customLocator into the scene
		- disable 'Start Streaming' menu item
		- enable 'End Streaming' menu item
		*/
		MDagModifier dagModifier;
		MObject newNode = dagModifier.createNode("customLocator", MObject::kNullObj);
		MFnDependencyNode fn(newNode);
		fn.setName("streamingLocator");
		dagModifier.doIt(); // you need to call this otherwise the node isn't added

		if (!newNode.isNull())
		{ // if everything went well, then I can set UI to state "streaming"
			MGlobal::executeCommand("menuItem -edit -enable false startStreamingItem");
			MGlobal::executeCommand("menuItem -edit -enable true endStreamingItem");
		}
		//} else // else I keep UI in state "not streaming" and also delete the streaming node
		//{
		//	MGlobal::executeCommand("parent -removeObject streamingLocator");
		//}
		//MGlobal::executeCommand("createNode \"customLocator\" -n \"streamingLocator\"");

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreamingCommand;
	}

};