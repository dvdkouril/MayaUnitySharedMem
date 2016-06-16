#pragma once

#include <maya/MPxNode.h>
#include <vector>

class CameraWatcherNode : public MPxNode
{
public:
								CameraWatcherNode(void);
	virtual						~CameraWatcherNode(void);

	virtual MStatus				compute(const MPlug& plug, MDataBlock& data) override;
	static void*				creator();
	static MStatus				initialize();

	static MTypeId				id;
	// ------------------------- in
	static MObject				aSharedMemoryPointer;
	static MObject				aPosition;
	//static MObject				aRotation;
	// ------------------------ out
	static MObject				aDirtyOutput; // Dummy plug for forcing an evaluation of this mode

private:
	void						writeToMemory(std::vector<float> posMemOutArray, 
											  std::vector<float> rotMemOutArray,
											  void* handle);
};