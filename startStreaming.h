#pragma once

#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
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
		HANDLE handle			= createSharedMemory("PositionsRotationsMemory", 256);
		LPVOID pointer			= createMemoryMapping(handle);
		HANDLE camHandle		= createSharedMemory("MayaToUnityCameraInfoSharedMem", 256);
		LPVOID camPointer		= createMemoryMapping(camHandle);
		HANDLE pdbHandle		= createSharedMemory("MayaToUnityPdbMappingSharedMem", 256);
		LPVOID pdbPointer		= createMemoryMapping(pdbHandle);
		HANDLE sceneHandle		= createSharedMemory("MayaToUnitySceneInfoSharedMem", 256);
		LPVOID scenePointer		= createMemoryMapping(sceneHandle);

		std::cerr << "Pointer after MapViewOfFile: " << pointer << std::endl;

		MDGModifier dgModifier;

		int numberOfObjects = 0;
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
				plugProteinWatcher(dgModifier, proteinFn, parentProteinNode, pointer);

				// building up the pdb mapping "database"
				numberOfObjects += 1;
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
			}
		}

		MFnDependencyNode watcherFn = /*I need to get the current watcher node...here will be another iterator*/;
		// TODO: I need to set another attribute numberOfObjects...
		MObject objNumbrAttr = watcherFn.attribute(MString("NumberOfObjects"));
		MPlug objNumbrPlug = watcherFn.findPlug(objNumbrAttr);
		MStatus st = objNumbrPlug.setInt(numberOfObjects);

		std::stringstream ss;
		for (std::map<std::string, int>::iterator it = pdbIdMap.begin(); it != pdbIdMap.end(); ++it)
		{
			ss << it->first << " " << it->second << std::endl;
		}
		std::string toOutputStr = ss.str();

		CopyMemory(pdbPointer, toOutputStr.c_str(), sizeof(char) * toOutputStr.length());
		CopyMemory(scenePointer, &numberOfObjects, sizeof(int));

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
		//MObject newPW = dgModifier.createNode(ProteinWatcherNode::id);
		MDagModifier dagModifier;
		//MObject newPWTransform = dagModifier.createNode(ProteinWatcherNode::id);
		MObject newPWTransform = dagModifier.createNode("proteinWatcherNode", MObject::kNullObj);
		dagModifier.doIt();
		MFnDagNode watcherTransFn(newPWTransform);
		MStatus status;
		unsigned childNum = watcherTransFn.childCount();
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
		//MStatus connectRes = dgModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);
		MStatus connectRes = dagModifier.connect(parentProteinNode, sourceAttr, newPW, destAttr);
		
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