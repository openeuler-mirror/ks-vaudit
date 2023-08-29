/*
* Copyright 2017-2018 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/
#pragma once

#include <iostream>
#include <unordered_map>
#include <GL/gl.h>

#include "NvEncoder.h"

class NvEncoderGL : public NvEncoder
{
public:
	/**
	*  @brief The constructor for the NvEncoderGL class
	*  An OpenGL context must be current to the calling thread/process when
	*  creating an instance of this class.
	*/
	NvEncoderGL(uint32_t nWidth, uint32_t nHeight, NV_ENC_BUFFER_FORMAT eBufferFormat,
		uint32_t nExtraOutputDelay = 0, bool bMotionEstimationOnly = false);

	NvEncoderGL(uint32_t nWidth, uint32_t nHeight, NV_ENC_BUFFER_FORMAT eBufferFormat,
		uint32_t nFrameRateNum, uint32_t naverageBitRate,
		uint32_t nExtraOutputDelay = 0, bool bMotionEstimationOnly = false);		

	virtual ~NvEncoderGL();
private:
	/**
	*  @brief This function is used to allocate input buffers for encoding.
	*  This function is an override of virtual function NvEncoder::AllocateInputBuffers().
	*  This function creates OpenGL textures which are used to hold input data.
	*  To obtain handle to input buffers, the application must call NvEncoder::GetNextInputFrame()
	*/
	virtual void AllocateInputBuffers(int32_t numInputBuffers);

	/**
	*  @brief This function is used to release the input buffers allocated for encoding.
	*  This function is an override of virtual function NvEncoder::ReleaseInputBuffers().
	*/
	virtual void ReleaseInputBuffers();
private:
	void ReleaseGLResources();
};
