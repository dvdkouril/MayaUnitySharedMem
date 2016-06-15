#pragma once

#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDagModifier.h>
#include <windows.h>
#include <iomanip>

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
		// I could do a scene traversal once here so that I know how much memory should I allocate...but for now...
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "PositionsRotationsMemory"); // this handle is leaked ATM
		LPVOID pointer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		std::cerr << "Pointer after MapViewOfFile: " << pointer << std::endl;

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
				std::cerr << "pointer: " << pointer << std::endl;
				std::cerr << "(MInt64)pointer: " << std::hex << (MInt64)pointer << std::endl;
				MStatus st = pointerPlug.setInt64((MInt64)pointer);
				if (st != MS::kSuccess)
				{
					std::cerr << ".setInt64 fucked up" << std::endl;
				}
				//pointerPlug.setInt((int)pointer); // This is freaking hacky and if this works I will be ashamed (but happy at the same time)
				std::cerr << "pointer: " << pointer << std::endl;
				//std::cerr << "(int)pointer: " << std::hex << (int)pointer << std::endl;
				//std::cerr << "(long)pointer: " << std::hex << (long)pointer << std::endl;

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