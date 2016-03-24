#include "ShMemWriterNode.h"
#include <maya/MFnPlugin.h>

MTypeId ShMemWriterNode::id(0x00000001);

void* ShMemWriterNode::creator() 
{
	return new ShMemWriterNode;
}

MStatus ShMemWriterNode::compute(const MPlug& plug, MDataBlock& data) 
{
	return MS::kSuccess;
}

MStatus ShMemWriterNode::initialize() 
{
	return MS::kSuccess;
}

MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, "David Kouril", "0.1", "Any");
	// ja, this is why you need to keep id in the class...
	status = plugin.registerNode("shMemWriterNode", ShMemWriterNode::id, ShMemWriterNode::creator, ShMemWriterNode::initialize);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);
	status = plugin.deregisterNode(ShMemWriterNode::id);
	CHECK_MSTATUS_AND_RETURN_IT(status);

	return status;
}