#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>

#include "CustomLocator.h"

#include "startStreamingCommand.h"
#include "endStreamingCommand.h"
#include "showStreamingWindowCommand.h"

// ------------------------------------------------------------------------------------- 
// MAYA PLUGIN REQUIRED FUNCTIONS
// ------------------------------------------------------------------------------------- 
// it might be better to move this somewhere else, but for now...

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "David Kouril", "1.0", "Any");

	MStatus status = plugin.registerNode(CustomLocator::typeName,
		CustomLocator::typeId,
		CustomLocator::creator,
		CustomLocator::initialize,
		MPxNode::kLocatorNode); // is this why it didn't work before????????? yep, looks like it

								// register startStreamingCommand command
	status = plugin.registerCommand("startM2CVStreaming", startStreamingCommand::creator);
	status = plugin.registerCommand("endM2CVStreaming", endStreamingCommand::creator);
	status = plugin.registerCommand("showM2CVWindow", showStreamingWindowCommand::creator);

	// add a custom menu with items: Start streaming, Stop streaming
	MGlobal::executeCommand("global string $gMainWindow");
	MGlobal::executeCommand("setParent $gMainWindow");
	MGlobal::executeCommand("menu -label \"MayaToCellVIEW\" mayaToCelViewMenu");

	MGlobal::executeCommand("setParent -menu mayaToCelViewMenu");
	MGlobal::executeCommand("menuItem -label \"Start Streaming\" -command \"startM2CVStreaming\" startStreamingItem");
	MGlobal::executeCommand("menuItem -label \"End Streaming\" -command \"endM2CVStreaming\" -enable false endStreamingItem");
	MGlobal::executeCommand("menuItem -label \"Show Streaming Window\" -command \"showM2CVWindow\" -enable true showWindowItem");

	if (!status)
	{
		status.perror("Failed to register customLocator\n");
		return status;
	}

	std::cout << "initializePlugin successful." << std::endl;

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MFnPlugin plugin(obj);

	// TODO: delete custom menu
	MStatus status = plugin.deregisterNode(CustomLocator::typeId);
	status = plugin.deregisterCommand("startM2CVStreaming");
	status = plugin.deregisterCommand("endM2CVStreaming");
	status = plugin.deregisterCommand("showM2CVWindow");

	MGlobal::executeCommand("global string $gMainWindow");
	MGlobal::executeCommand("setParent $gMainWindow");
	MGlobal::executeCommand("deleteUI \"mayaToCelViewMenu\""); // finally this works

	if (!status)
	{
		status.perror("Failed to deregister customLocator\n");
		return status;
	}

	std::cout << "uninitializePlugin successful." << std::endl;
	return status;
}

// ------------------------------------------------------------------------------------- MAYA PLUGIN REQUIRED FUNCTIONS end