#pragma once

#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagModifier.h>
#include <windows.h>
#include <iomanip>
#include <map>
#include <string>
#include <algorithm>
#include <sstream>

#include "ProteinWatcherNode.h"
#include "global_variables.h"

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
		nextFreeId = 0;

		// TODO: iterate the scene and count the number of object so that I can allocate appropriate amount of memory
		int numberOfObjects = 0;

		MItDag initialIt = MItDag(MItDag::kDepthFirst, MFn::kTransform);
		for (; !initialIt.isDone(); initialIt.next())
		{
			MObject node = initialIt.currentItem();
			
			MFnDependencyNode fn(node);
			MString nodeName = fn.name();

			if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
			{
				numberOfObjects += 1;
			}
		}

		// I could do a scene traversal once here so that I know how much memory should I allocate...but for now...
		DWORD memorySize = (DWORD)numberOfObjects * sizeof(float) * 4 * 3; // TODO: don't make it DWORD here but make it something big and calculate the two DWORDs inside the method
		/*HANDLE handle			= createSharedMemory("PositionsRotationsMemory", memorySize);
		LPVOID pointer			= createMemoryMapping(handle);*/
		mainMemoryHandle		= createSharedMemory("PositionsRotationsMemory", memorySize);
		pointer					= createMemoryMapping(mainMemoryHandle);
		camHandle				= createSharedMemory("MayaToUnityCameraInfoSharedMem", 256);
		camPointer				= createMemoryMapping(camHandle);
		pdbHandle				= createSharedMemory("MayaToUnityPdbMappingSharedMem", 256);
		pdbPointer				= createMemoryMapping(pdbHandle);
		sceneHandle				= createSharedMemory("MayaToUnitySceneInfoSharedMem", 256);
		scenePointer			= createMemoryMapping(sceneHandle);

		std::cerr << "Pointer after MapViewOfFile: " << pointer << std::endl;

		MDGModifier dgModifier;

		//int numberOfObjects = 0;
		int nextFreeInternalId = 0;
		std::map<std::string, int> pdbIdMap;

		MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform);
		for (; !itTran.isDone(); itTran.next())
		{
			MObject parentProteinNode = itTran.currentItem();
			
			MFnDependencyNode proteinFn(parentProteinNode);
			MString nodeName = proteinFn.name();

			if (strstr(nodeName.asChar(), "persp") != NULL)
			{
				plugCameraWatcher(dgModifier, proteinFn, parentProteinNode, camPointer); // really bad naming...TODO: fix
			}

			if (strstr(nodeName.asChar(), "pdbMolStruc_") != NULL)
			{

				// building up the pdb mapping "database"
				//numberOfObjects += 1; // I already did this
				std::string fullName(nodeName.asChar());
				std::string pdbString = fullName.substr(12, 4);
				std::transform(pdbString.begin(), pdbString.end(), pdbString.begin(), ::tolower); // TODO: this might cause some problems in the future
				// figure out if it's not there already
				std::map<std::string, int>::iterator containsIt = pdbIdMap.find(pdbString);
				if (containsIt == pdbIdMap.end()) 
				{ // if this key is not in the map already...
					pdbIdMap[pdbString] = nextFreeInternalId;
					nextFreeInternalId += 1;
				}

				int internalTypeId = pdbIdMap[pdbString];
				plugProteinWatcher(dgModifier, proteinFn, parentProteinNode, pointer, internalTypeId);
			}
		}

		// set NumberOfObjects attribute on ProteinWatcher nodes
		MItDependencyNodes depIt;
		for (; !depIt.isDone(); depIt.next())
		{
			MObject node = depIt.item();
			MFnDependencyNode fn(node);
			MString name = fn.name();
			if (strstr(name.asChar(), "ProteinWatcher") != NULL)
			{
				MObject objNumbrAttr = fn.attribute(MString("NumberOfObjects"));
				MPlug objNumbrPlug = fn.findPlug(objNumbrAttr);
				MStatus st = objNumbrPlug.setInt(numberOfObjects);
			}
		}

		// writing pdb mapping info into shared memory
		std::stringstream ss;
		for (std::map<std::string, int>::iterator it = pdbIdMap.begin(); it != pdbIdMap.end(); ++it)
		{
			ss << it->first << " " << it->second << std::endl;
		}
		std::string toOutputStr = ss.str();

		CopyMemory(pdbPointer, toOutputStr.c_str(), sizeof(char) * toOutputStr.length());
		CopyMemory(scenePointer, &numberOfObjects, sizeof(int));


		MGlobal::executeCommand("menuItem -edit -enable false startStreamingItem");
		MGlobal::executeCommand("menuItem -edit -enable true stopStreamingItem");
		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new startStreaming;
	}

	static void plugCameraWatcher(MDGModifier& dgModifier, MFnDependencyNode& proteinFn, MObject parentProteinNode, void * camPointer)
	{
		//MObject newCW = dgModifier.createNode(CameraWatcherNode::id);
		MDagModifier dagModifier;
		MObject newCWTransform = dagModifier.createNode("CameraWatcherNode", MObject::kNullObj);
		dagModifier.doIt();
		MFnDagNode watcherTransFn(newCWTransform);
		MStatus status;
		MObject newCW = watcherTransFn.child(0, &status);
		if (status != MS::kSuccess)
		{
			std::cerr << status.errorString() << std::endl;
			return;
		}

		MFnDagNode watcherFn(newCW);
		//MFnDependencyNode watcherFn(newCW);
		watcherFn.setName(MString("CameraWatcher"));

		MObject sourceAttr = proteinFn.attribute(MString("translate"));
		MObject destAttr = watcherFn.attribute(MString("PositionInput"));
		MStatus connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr); // I could have a better name than parentProteinNode cause in this
																										 // case it's camera node
		if (connectRes != MS::kSuccess)
		{
			std::cerr << "Fail when connecting CameraWatcher to a persp camera" << std::endl;
		}

		sourceAttr = proteinFn.attribute(MString("rotateX"));
		destAttr = watcherFn.attribute(MString("RotationInputX"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr);

		sourceAttr = proteinFn.attribute(MString("rotateY"));
		destAttr = watcherFn.attribute(MString("RotationInputY"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr);

		sourceAttr = proteinFn.attribute(MString("rotateZ"));
		destAttr = watcherFn.attribute(MString("RotationInputZ"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newCW, destAttr);

		MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
		MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
		MStatus st = pointerPlug.setInt64((MInt64)camPointer);
		if (st != MS::kSuccess)
		{
			std::cerr << "setting pointer for Camera Watcher has failed" << std::endl;
		}

		// commiting the changes
		dagModifier.doIt();
		//dgModifier.doIt();
	}

	static void plugProteinWatcher(MDGModifier& dgModifier, MFnDependencyNode& proteinFn, MObject parentProteinNode, void * pointer, int internalTypeId)
	{
		//MObject newPW = dgModifier.createNode(ProteinWatcherNode::id);
		MDagModifier dagModifier;
		//MObject newPWTransform = dagModifier.createNode(ProteinWatcherNode::id);
		MObject newPWTransform = dagModifier.createNode("proteinWatcherNode", MObject::kNullObj);
		dagModifier.doIt();
		MFnDagNode watcherTransFn(newPWTransform);
		MStatus status;
		MObject newPW = watcherTransFn.child(0, &status);
		if (status != MS::kSuccess)
		{
			std::cerr << status.errorString() << std::endl;
			return;
		}
		MFnDagNode watcherFn(newPW);
		watcherFn.setName(MString("ProteinWatcher") + nextFreeId);

		// connecting it
		MObject sourceAttr = proteinFn.attribute(MString("translate"));
		MObject destAttr = watcherFn.attribute(MString("PositionInput"));
		MStatus connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);
		
		if (connectRes != MS::kSuccess)
		{
			std::cerr << "Fail when connecting ProteinWatcher to Protein" << std::endl;
		}

		// Rotation connection
		sourceAttr = proteinFn.attribute(MString("rotateX"));
		destAttr = watcherFn.attribute(MString("RotationInputX"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);

		sourceAttr = proteinFn.attribute(MString("rotateY"));
		destAttr = watcherFn.attribute(MString("RotationInputY"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);

		sourceAttr = proteinFn.attribute(MString("rotateZ"));
		destAttr = watcherFn.attribute(MString("RotationInputZ"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);

		/*sourceAttr = proteinFn.attribute(MString("rotateQuaternionW"));
		destAttr = watcherFn.attribute(MString("RotationInputW"));
		connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);*/

		if (connectRes != MS::kSuccess)
		{
			std::cerr << "Fail when connecting on Rotation attribute" << std::endl;
		}

		// setting the pointer attribute
		MObject pointerAttr = watcherFn.attribute(MString("SharedMemoryPointer"));
		MPlug pointerPlug = watcherFn.findPlug(pointerAttr);
		//std::cerr << "pointer: " << pointer << std::endl;
		//std::cerr << "(MInt64)pointer: " << std::hex << (MInt64)pointer << std::endl;
		MStatus st = pointerPlug.setInt64((MInt64)pointer);
		if (st != MS::kSuccess)
		{
			std::cerr << ".setInt64 failed" << std::endl;
		}
		
		MObject indexAttr = watcherFn.attribute(MString("IndexInput"));
		MPlug indexPlug = watcherFn.findPlug(indexAttr);
		indexPlug.setInt(nextFreeId);

		MObject typeIdAttr = watcherFn.attribute(MString("InternalTypeId"));
		MPlug typeIdPlug = watcherFn.findPlug(typeIdAttr);
		typeIdPlug.setInt(internalTypeId); // internalTypeId is a method parameter
		
		// commiting the changes
		//dgModifier.doIt();
		dagModifier.doIt();
		nextFreeId += 1;
	}

	static HANDLE createSharedMemory(char * name, size_t size)
	{   // TODO: compute the lower and higher DWORD based on the size
		return CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)size, name);
	}

	static LPVOID createMemoryMapping(HANDLE handle)
	{
		return MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	}
};

int startStreaming::nextFreeId = 0;