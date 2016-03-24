#ifndef SHMEMWRITERNODE_H
#define SHMEMWRITERNODE_H

#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MStatus.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>

#include <maya/MPxNode.h>

class ShMemWriterNode : public MPxNode {
public:
	ShMemWriterNode() {}
	virtual MStatus compute(const MPlug& plug, MDataBlock& data);
	static void* creator();
	static MStatus initialize();

	static MTypeId id;
	static MObject aInput;
	static MObject aOutput;
};

#endif