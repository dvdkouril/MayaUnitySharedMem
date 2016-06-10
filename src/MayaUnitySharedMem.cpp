#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h>
#include <maya/MUiMessage.h>

#include "ProteinWatcherNode.h"

#include "startStreamingCommand.h"
#include "endStreamingCommand.h"
#include "startStreaming.h"

MStatus initializePlugin(MObject obj)
{
	MFnPlugin plugin(obj, "David Kouril", "1.0", "Any");

	MStatus status = plugin.registerNode("proteinWatcherNode",
		ProteinWatcherNode::id,
		ProteinWatcherNode::creator,
		ProteinWatcherNode::initialize);

	status = plugin.registerCommand("startStreaming", startStreaming::creator);

	// add a custom menu with items: Start streaming, Stop streaming
	MGlobal::executeCommand("global string $gMainWindow");
	MGlobal::executeCommand("setParent $gMainWindow");
	MGlobal::executeCommand("menu -label \"MayaToCellVIEW\" mayaToCelViewMenu");

	MGlobal::executeCommand("setParent -menu mayaToCelViewMenu");
	MGlobal::executeCommand("menuItem -label \"startStreaming\" -command \"startStreaming\" startSTreamingItem");

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
	MStatus status = plugin.deregisterNode(ProteinWatcherNode::id);
	plugin.deregisterCommand("startStreaming");

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