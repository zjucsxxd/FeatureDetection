/*
 * RenderDevice.hpp
 *
 *  Created on: 23.07.2013
 *      Author: Patrik Huber
 */
#pragma once

#ifndef RENDERDEVICE_HPP_
#define RENDERDEVICE_HPP_

#include "render/Camera.hpp"
#include "render/Mesh.hpp"

#include "opencv2/core/core.hpp"

#include <vector>

using cv::Mat;
using cv::Vec2f;
using cv::Vec4f;
using std::vector;

namespace render {

/**
 * Desc
 */
class RenderDevice
{
public:
	RenderDevice() {}; // I think I don't want a default constructor?
	RenderDevice(unsigned int screenWidth, unsigned int screenHeight) {};
	~RenderDevice() {}; // Why no virtual possible?

	virtual cv::Mat getImage() = 0; // make these a '&' ?
	virtual cv::Mat getDepthBuffer() = 0;

	Camera& getCamera() {
			return camera;
	};

	virtual void setBackgroundImage(Mat background) = 0;

	virtual void updateWindow() = 0;

	// Render... Does not do any clipping.
	virtual Vec2f renderVertex(Vec4f vertex) = 0;
	virtual vector<Vec2f> renderVertexList(vector<Vec4f> vertexList) = 0;
	virtual void renderMesh(Mesh mesh) = 0;

	virtual void setWorldTransform(Mat worldTransform) = 0;

protected:
	unsigned int screenWidth;
	unsigned int screenHeight;
	float aspect;
	Camera camera; // init this in c'tor? (no it's on stack so default Camera c'tor init)

};

 } /* namespace render */

#endif /* RENDERDEVICE_HPP_ */
