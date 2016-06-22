#pragma once
#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>

#include "global_variables.h"

class stopStreaming : public MPxCommand
{
public:
	stopStreaming() {};
	~stopStreaming() {};

	virtual MStatus doIt(const MArgList&)
	{
		//MItDag itTran = MItDag(MItDag::kDepthFirst, MFn::kTransform); // yeah ProteinWatcher is not kTransform...
		MItDependencyNodes it = MItDependencyNodes();
		MDGModifier dgModifier;
		for (; !it.isDone(); it.next())
		{
			MObject obj = it.item(); //it.currentItem();
			
			MFnDependencyNode fn(obj);
			MString nodeName = fn.name();

			if ( (strstr(nodeName.asChar(), "ProteinWatcher") != NULL) || (strstr(nodeName.asChar(), "CameraWatcher") != NULL))
			{
				MFnDagNode dagFn(obj);
				int parentCount = dagFn.parentCount();
				MObject parentTransform = dagFn.parent(0);
				dgModifier.deleteNode(obj);
				dgModifier.deleteNode(parentTransform);
			}

		}
		dgModifier.doIt();

		MGlobal::executeCommand("menuItem -edit -enable true startStreamingItem");
		MGlobal::executeCommand("menuItem -edit -enable false stopStreamingItem");

		// release all the handles to shared memory objects
		UnmapViewOfFile(pointer);
		CloseHandle(mainMemoryHandle);

		UnmapViewOfFile(camPointer);
		CloseHandle(camHandle);

		UnmapViewOfFile(pdbPointer);
		CloseHandle(pdbHandle);

		UnmapViewOfFile(scenePointer);
		CloseHandle(sceneHandle);

		MGlobal::executeCommand("flushUndo");

		std::cerr << "Streaming has been successfully stopped" << std::endl;
		return MS::kSuccess;
	}

	static void * creator()
	{
		return new stopStreaming;
	}
};
