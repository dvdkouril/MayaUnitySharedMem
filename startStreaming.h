#pragma once

#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagModifier.h>
#include <windows.h>
#include "ProteinWatcherNode.h"

class startStreaming : public MPxCommand
{
private:
	static int nextFreeId;
	public:

	startStreaming() {}
	~startStreaming() {}

	virtual MStatus doIt(const MArgList&) 
	{
		std::cerr << "startStreaming::doIt" << std::endl;
		/*
		TODO:
			- open shared memory
			- go through the scene looking for pdbMolStruc objects
				- clip a ProteinWatcher to every one of them
				- set the position attribute
				- set the shared memory pointer to the ProteinWatcher attribute
		*/
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "PositionsRotationsMemory"); // this handle is leaked ATM
		LPVOID pointer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		//MDagModifier dagModifier;
		MDGModifier dgModifier;

		MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
		for (; !itTran.isDone(); itTran.next())
		{
			MObject parentProteinNode = itTran.currentItem();
			
			MFnDependencyNode proteinFn(parentProteinNode);
			MString nodeName = proteinFn.name();

			if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
			{
				// creating the protein watcher node
				MObject newPW = dgModifier.createNode(ProteinWatcherNode::id);
				MFnDependencyNode watcherFn(newPW);
				watcherFn.setName(MString("ProteinWatcher") + nextFreeId);
				nextFreeId += 1;

				// connecting it
				MObject sourceAttr = proteinFn.attribute(MString("translate"));
				MObject destAttr = watcherFn.attribute(MString("PositionInput"));
				MStatus connectRes = dgModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);

				if (connectRes != MS::kSuccess)
				{
					std::cerr << "Fail when connecting ProteinWatcher to Protein" << std::endl;
				}

				// setting the pointer attribute
				MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
				MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
				pointerPlug.setInt((int)pointer); // This is freaking hacky and if this works I will be ashamed (but happy at the same time)

				// commiting the changes
				dgModifier.doIt();
			}
		}

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreaming;
	}
};

int startStreaming::nextFreeId = 0;