#pragma once

#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagModifier.h>
#include <windows.h>

class startStreaming : public MPxCommand
{
	public:


	startStreaming() {}
	~startStreaming() {}

	virtual MStatus doIt(const MArgList&) 
	{
		/*
		TODO:
			- open shared memory
			- go through the scene looking for pdbMolStruc objects
				- clip a ProteinWatcher to every one of them
				- set the shared memory pointer to the ProteinWatcher attribute
		*/
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "PositionsRotationsMemory");
		LPVOID pointer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		MDagModifier dagModifier;

		MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
		for (; !itTran.isDone(); itTran.next())
		{
			MObject obj = itTran.currentItem();
			
			MFnDependencyNode nodeFn(obj);
			MString nodeName = nodeFn.name();

			if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
			{
				std::cerr << "creating node?" << std::endl;
				// create ProteinWatcher node
				// parent ProteinWatcher node to this object
				dagModifier.createNode(ProteinWatcherNode::id, obj);
				dagModifier.doIt();
			}
		}

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreaming;
	}
};