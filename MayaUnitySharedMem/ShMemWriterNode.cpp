#include "ShMemWriterNode.h"
#include <maya/MFnPlugin.h>

MTypeId ShMemWriterNode::id(0x00000001);

void* ShMemWriterNode::creator() 
{
	return new ShMemWriterNode;
}

MStatus ShMemWriterNode::compute(const MPlug& plug, MDataBlock& data) 
{
	if (plug != aOutput) {
		return MS::kUnknownParameter;
	}

	float inputValue = data.inputValue(aInput).asFloat();
	inputValue *= 2.0f;

	MDataHandle hOutput = data.outputValue(aOutput);
	hOutput.setFloat(inputValue);
	data.setClean(plug);

	/* if this was the place where I could execute the writing code...
	 - iterate over all the objects
	 - get their positions
	 - encode it into a string
	CopyMemory((PVOID)pBuf, szMsg, (_tcslen(szMsg) * sizeof(TCHAR)));
    _getch();

	"the second process (unity) can then use MapViewOfFile function to obtain a pointer to the file view, pBuf.
	The process can display this string as it would any other string."
	*/


	return MS::kSuccess;
}

MStatus ShMemWriterNode::initialize() 
{
	MFnNumericAttribute nAttr;
	
	aOutput = nAttr.create("output", "out", MFnNumericData::kFloat);
	nAttr.setWritable(false);
	nAttr.setStorable(false);
	addAttribute(aOutput);

	aInput = nAttr.create("input", "in", MFnNumericData::kFloat);
	nAttr.setKeyable(true);
	addAttribute(aInput);
	attributeAffects(aInput, aOutput);

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