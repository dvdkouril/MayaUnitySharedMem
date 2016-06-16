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

		// I could do a scene traversal once here so that I know how much memory should I allocate...but for now...
		HANDLE handle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "PositionsRotationsMemory"); // this handle is leaked ATM
		LPVOID pointer = MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		HANDLE camHandle = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 256, "MayaToUnityCameraInfoSharedMem");
		LPVOID camPointer = MapViewOfFile(camHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);

		std::cerr << "Pointer after MapViewOfFile: " << pointer << std::endl;

		MDGModifier dgModifier;

		MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
		for (; !itTran.isDone(); itTran.next())
		{
			MObject parentProteinNode = itTran.currentItem();
			
			MFnDependencyNode proteinFn(parentProteinNode);
			MString nodeName = proteinFn.name();

			if (strstr(nodeName.asChar(), "persp") != NULL)
			{
				plugCameraWatcher(dgModifier, proteinFn, parentProteinNode, camPointer); // really bad naming...TODO: fix
				//MObject newCW = dgModifier.createNode(CameraWatcherNode::id);
				//MFnDependencyNode watcherFn(newCW);
				//watcherFn.setName(MString("CameraWatcher"));

				//MObject sourceAttr = proteinFn.attribute(MString("translate"));
				//MObject destAttr = watcherFn.attribute(MString("PositionInput"));

				//MStatus connectRes = dgModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr); // I could have a better name than parentProteinNode cause in this
				//																						 // case it's camera node

				//if (connectRes != MS::kSuccess)
				//{
				//	std::cerr << "Fail when connecting CameraWatcher to a persp camera" << std::endl;
				//}

				//MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
				//MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
				//MStatus st = pointerPlug.setInt64((MInt64)camPointer);
				//if (st != MS::kSuccess)
				//{
				//	std::cerr << "setting pointer for Camera Watcher fucked up" << std::endl;
				//}

				//// commiting the changes
				//dgModifier.doIt();
			}

			if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
			{
				plugProteinWatcher(dgModifier, proteinFn, parentProteinNode, pointer);
				// creating the protein watcher node
				//MObject newPW = dgModifier.createNode(ProteinWatcherNode::id);
				//MFnDependencyNode watcherFn(newPW);
				//watcherFn.setName(MString("ProteinWatcher") + nextFreeId);

				//// connecting it
				//MObject sourceAttr = proteinFn.attribute(MString("translate"));
				//MObject destAttr = watcherFn.attribute(MString("PositionInput"));
				//MStatus connectRes = dgModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);

				//if (connectRes != MS::kSuccess)
				//{
				//	std::cerr << "Fail when connecting ProteinWatcher to Protein" << std::endl;
				//}

				//// setting the pointer attribute
				//MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
				//MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
				////std::cerr << "pointer: " << pointer << std::endl;
				////std::cerr << "(MInt64)pointer: " << std::hex << (MInt64)pointer << std::endl;
				//MStatus st = pointerPlug.setInt64((MInt64)pointer);
				//if (st != MS::kSuccess)
				//{
				//	std::cerr << ".setInt64 fucked up" << std::endl;
				//}
				////pointerPlug.setInt((int)pointer); // This is freaking hacky and if this works I will be ashamed (but happy at the same time)
				////std::cerr << "pointer: " << pointer << std::endl;

				//MObject indexAttr = watcherFn.attribute(MString("IndexInput"));
				//MPlug indexPlug = watcherFn.findPlug(indexAttr);
				//indexPlug.setInt(nextFreeId);

				//// commiting the changes
				//dgModifier.doIt();
				//nextFreeId += 1;
			}
		}

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreaming;
	}

	static void plugCameraWatcher(MDGModifier& dgModifier, MFnDependencyNode& proteinFn, MObject parentProteinNode, void * camPointer)
	{
		MObject newCW = dgModifier.createNode(CameraWatcherNode::id);
		MFnDependencyNode watcherFn(newCW);
		watcherFn.setName(MString("CameraWatcher"));

		MObject sourceAttr = proteinFn.attribute(MString("translate"));
		MObject destAttr = watcherFn.attribute(MString("PositionInput"));

		MStatus connectRes = dgModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr); // I could have a better name than parentProteinNode cause in this
																										 // case it's camera node

		if (connectRes != MS::kSuccess)
		{
			std::cerr << "Fail when connecting CameraWatcher to a persp camera" << std::endl;
		}

		MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
		MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
		MStatus st = pointerPlug.setInt64((MInt64)camPointer);
		if (st != MS::kSuccess)
		{
			std::cerr << "setting pointer for Camera Watcher fucked up" << std::endl;
		}

		// commiting the changes
		dgModifier.doIt();
	}

	static void plugProteinWatcher(MDGModifier& dgModifier, MFnDependencyNode& proteinFn, MObject parentProteinNode, void * pointer)
	{
		MObject newPW = dgModifier.createNode(ProteinWatcherNode::id);
		MFnDependencyNode watcherFn(newPW);
		watcherFn.setName(MString("ProteinWatcher") + nextFreeId);

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
		//std::cerr << "pointer: " << pointer << std::endl;
		//std::cerr << "(MInt64)pointer: " << std::hex << (MInt64)pointer << std::endl;
		MStatus st = pointerPlug.setInt64((MInt64)pointer);
		if (st != MS::kSuccess)
		{
			std::cerr << ".setInt64 fucked up" << std::endl;
		}
		//pointerPlug.setInt((int)pointer); // This is freaking hacky and if this works I will be ashamed (but happy at the same time)
		//std::cerr << "pointer: " << pointer << std::endl;
		
		MObject indexAttr = watcherFn.attribute(MString("IndexInput"));
		MPlug indexPlug = watcherFn.findPlug(indexAttr);
		indexPlug.setInt(nextFreeId);
		
		// commiting the changes
		dgModifier.doIt();
		nextFreeId += 1;
	}
};

int startStreaming::nextFreeId = 0;