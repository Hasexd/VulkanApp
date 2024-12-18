#pragma once

#include <vector>

#include "AllocatedBuffer.h"
#include "Vertex.h"



struct Mesh
{
	std::vector<Vertex> Vertices;
	AllocatedBuffer VertexBuffer;
};