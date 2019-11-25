# This file is part of Augen Light
#
# Copyright (c) 2017 - 2019 -- Élie Michel <elie.michel@exppad.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided “as is”, without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

if(DEV_MOD)
	add_definitions(-DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
else()
	add_definitions(-DSHARE_DIR="./share")
endif()

set(SRC
	Behavior/ImpostorCloudRenderer.h
	Behavior/ImpostorCloudRenderer.cpp
	Behavior/MeshDataBehavior.h
	Behavior/MeshDataBehavior.cpp
	Behavior/MeshRenderer.h
	Behavior/MeshRenderer.cpp
	Behavior/SandRenderer.h
	Behavior/SandRenderer.cpp

	utils/debug.h
	utils/debug.cpp
	utils/shader.h
	utils/shader.cpp
	utils/tplutils.h
	utils/fileutils.h
	utils/fileutils.cpp
	utils/strutils.h
	utils/strutils.cpp
	utils/jsonutils.h
	utils/jsonutils.cpp

	Ui/Dialog.h
	Ui/Gui.h
	Ui/Gui.cpp
	Ui/Window.h
	Ui/Window.cpp

	main.cpp
	bufferFillers.h
	bufferFillers.cpp
	Camera.h
	Camera.cpp
	IBehaviorHolder.h
	Framebuffer.h
	Framebuffer.cpp
	GlBuffer.h
	GlBuffer.cpp
	GlDeferredShader.h
	GlDeferredShader.cpp
	GlTexture.h
	GlTexture.cpp
	Light.h
	Light.cpp
	Logger.h
	Logger.cpp
	Mesh.h
	Mesh.cpp
	PointCloud.h
	ResourceManager.h
	ResourceManager.cpp
	RuntimeObject.h
	RuntimeObject.cpp
	Scene.cpp
	Scene_load.cpp
	Scene.h
	Shader.h
	Shader.cpp
	ShaderPool.h
	ShaderPool.cpp
	ShaderPreprocessor.h
	ShaderPreprocessor.cpp
	ShaderProgram.h
	ShaderProgram.cpp
	ShadowMap.h
	ShadowMap.cpp
	Triangle.h
	TurntableCamera.h
	TurntableCamera.cpp
	World.h
	World.cpp
)

set(LIBS
	glad
	glfw
	imgui
	rapidjson
	glm
	tinyobjloader
	tinyexr
	libSOIL
)

add_executable(BoundingLightField ${SRC})
target_include_directories(BoundingLightField PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BoundingLightField LINK_PRIVATE ${LIBS})


set(CMAKE_CXX_STANDARD 17)
if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)

# For IDE projects
group_source_by_folder(${SRC})
