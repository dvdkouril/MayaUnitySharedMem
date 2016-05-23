#pragma once

#include <maya/MPxCommand.h>

class showStreamingWindowCommand : public MPxCommand
{
public:


	showStreamingWindowCommand() {}
	~showStreamingWindowCommand() {}

	virtual MStatus doIt(const MArgList&) 
	{
		std::cerr << "Showing Streaming Window" << std::endl;

		// I guess that here comes the Qt code 

		return MS::kSuccess;
	}

	static void* creator() 
	{
		return new showStreamingWindowCommand;
	}

};