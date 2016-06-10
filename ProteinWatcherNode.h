#pragma once

#include <maya/MPxNode.h>
#include <maya/MFnNumericAttribute.h>
#include <windows.h>
#include <vector>

class ProteinWatcherNode : public MPxNode
{
public:
								ProteinWatcherNode(void);
	virtual						~ProteinWatcherNode(void);

	virtual MStatus				compute(const MPlug& plug, MDataBlock& data) override;
	static void*				creator();
	static MStatus				initialize();

	static MTypeId				id;

	static MObject				aSharedMemoryPointer;

	static MObject				aPosition;
	// TODO: Rotation, but for now just try it with position

	static MObject				aIndexOutput; // this is just so that I have some dependency (so that compute gets called)

private:
	void						writeToMemory(std::vector<float> posMemOutArray, 
											  std::vector<float> rotMemOutArray,
											  int index,
											  int handle);
};

