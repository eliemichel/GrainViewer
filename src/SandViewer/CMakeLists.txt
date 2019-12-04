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

set(CMAKE_CXX_STANDARD 17)
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest")
endif(MSVC)

###############################################################################
# Main executable

set(SRC
	Behavior/ImpostorCloudRenderer.h
	Behavior/ImpostorCloudRenderer.cpp
	Behavior/MeshDataBehavior.h
	Behavior/MeshDataBehavior.cpp
	Behavior/MeshRenderer.h
	Behavior/MeshRenderer.cpp
	Behavior/SandRenderer.h
	Behavior/SandRenderer.cpp
	Behavior/TransformBehavior.h
	Behavior/TransformBehavior.cpp
	Behavior/TestPrefixSumRenderer.h
	Behavior/TestPrefixSumRenderer.cpp

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
	utils/guiutils.h
	utils/guiutils.cpp

	Ui/Dialog.h
	Ui/Gui.h
	Ui/Gui.cpp
	Ui/Window.h
	Ui/Window.cpp
	Ui/SandRendererDialog.h
	Ui/SandRendererDialog.cpp
	Ui/SceneDialog.h
	Ui/SceneDialog.cpp

	main.cpp
	bufferFillers.h
	bufferFillers.cpp
	AnimationManager.h
	AnimationManager.cpp
	Camera.h
	Camera.cpp
	IBehaviorHolder.h
	EnvironmentVariables.h
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
	PointCloud.cpp
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
	tinypngout
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(SandViewer ${SRC})
target_include_directories(SandViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SandViewer LINK_PRIVATE ${LIBS})

# For IDE projects
group_source_by_folder(${SRC})

###############################################################################
# Tools

set(PointCloudConvert_SRC
	Tools/PointCloudConvert.cpp

	utils/strutils.h
	utils/strutils.cpp
	utils/fileutils.h
	utils/fileutils.cpp
	Logger.h
	Logger.cpp
	PointCloud.h
	PointCloud.cpp
)

set(PointCloudConvert_LIBS
	glm
)

add_executable(PointCloudConvert ${PointCloudConvert_SRC})
target_include_directories(PointCloudConvert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PointCloudConvert LINK_PRIVATE ${PointCloudConvert_LIBS})
set_property(TARGET PointCloudConvert PROPERTY FOLDER "Tools")

group_source_by_folder(${PointCloudConvert_SRC})
