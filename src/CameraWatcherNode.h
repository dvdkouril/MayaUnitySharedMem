#pragma once

#include <maya/MPxLocatorNode.h>
#include <vector>

class CameraWatcherNode : public MPxLocatorNode
{
public:
								CameraWatcherNode(void);
	virtual						~CameraWatcherNode(void);

	//virtual MStatus				compute(const MPlug& plug, MDataBlock& data) override;
	virtual void				draw(M3dView & view, const MDagPath & path, M3dView::DisplayStyle style, M3dView::DisplayStatus status) override;
	virtual bool				isBounded() const override;

	static void*				creator();
	static MStatus				initialize();

	static MTypeId				id;
	// ------------------------- in
	static MObject				aSharedMemoryPointer;
	static MObject				aPosition;
	static MObject				aRotationX;
	static MObject				aRotationY;
	static MObject				aRotationZ;
	// ------------------------ out
	static MObject				aDirtyOutput; // Dummy plug for forcing an evaluation of this mode

private:
	void						writeToMemory(std::vector<float> posMemOutArray, 
											  std::vector<float> rotMemOutArray,
											  void* handle);
};