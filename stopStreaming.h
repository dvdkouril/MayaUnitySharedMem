#pragma once
#include <maya/MPxCommand.h>
#include <maya/MItDag.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MDGModifier.h>
#include <maya/MGlobal.h>
#include <maya/MItDependencyNodes.h>

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

			if (strstr(nodeName.asChar(), "ProteinWatcher") != NULL)
			{
				//MDGModifier dgModifier; // yup this was fucking it up
				dgModifier.deleteNode(obj);
				//dgModifier.doIt();
			}


		}
		dgModifier.doIt();

		MGlobal::executeCommand("menuItem -edit -enable true startStreamingItem");
		MGlobal::executeCommand("menuItem -edit -enable false stopStreamingItem");

		MGlobal::executeCommand("flushUndo");

		return MS::kSuccess;
	}

	static void * creator()
	{
		return new stopStreaming;
	}
};
