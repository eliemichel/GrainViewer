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

enable_cpp17()
enable_multiprocessor_compilation()

###############################################################################
# Source subsets

set(AugenCore_SRC
	utils/strutils.h
	utils/strutils.cpp
	utils/fileutils.h
	utils/fileutils.cpp
	utils/debug.h
	utils/debug.cpp

	GlBuffer.h
	GlBuffer.cpp
	GlTexture.h
	GlTexture.cpp
	Logger.h
	Logger.cpp
	ResourceManager.h
	ResourceManager.cpp
	Shader.h
	Shader.cpp
	ShaderPool.h
	ShaderPool.cpp
	ShaderPreprocessor.h
	ShaderPreprocessor.cpp
	ShaderProgram.h
	ShaderProgram.cpp
)

###############################################################################
# Main executable

set(All_SRC
	${AugenCore_SRC}

	Behavior/GltfDataBehavior.h
	Behavior/GltfDataBehavior.cpp
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
	Behavior/LightGizmo.h
	Behavior/LightGizmo.cpp
	Behavior/FarSandRenderer.h
	Behavior/FarSandRenderer.cpp
	Behavior/PointCloudDataBehavior.h
	Behavior/PointCloudDataBehavior.cpp

	utils/shader.h
	utils/shader.cpp
	utils/tplutils.h
	utils/jsonutils.h
	utils/jsonutils.cpp
	utils/guiutils.h
	utils/guiutils.cpp

	Ui/Dialog.h
	Ui/Widgets.h
	Ui/Widgets.cpp
	Ui/Gui.h
	Ui/Gui.cpp
	Ui/TestGui.h
	Ui/TestGui.cpp
	Ui/Window.h
	Ui/Window.cpp
	Ui/SandRendererDialog.h
	Ui/SandRendererDialog.cpp
	Ui/FarSandRendererDialog.h
	Ui/FarSandRendererDialog.cpp
	Ui/SceneDialog.h
	Ui/SceneDialog.cpp
	Ui/LightGizmoDialog.h
	Ui/LightGizmoDialog.cpp

	AnimationManager.h
	AnimationManager.cpp
	Behavior.h
	BehaviorRegistry.h
	BehaviorRegistry.cpp
	Camera.h
	Camera.cpp
	IBehaviorHolder.h
	EnvironmentVariables.h
	Filtering.h
	Filtering.cpp
	Framebuffer.h
	Framebuffer.cpp
	Framebuffer2.h
	Framebuffer2.cpp
	GlBuffer.h
	GlBuffer.cpp
	GlDeferredShader.h
	GlDeferredShader.cpp
	Light.h
	Light.cpp
	Mesh.h
	Mesh.cpp
	PointCloud.h
	PointCloud.cpp
	RuntimeObject.h
	RuntimeObject.cpp
	Scene.h
	Scene.cpp
	Scene_load.cpp
	ShadowMap.h
	ShadowMap.cpp
	Triangle.h
	TurntableCamera.h
	TurntableCamera.cpp
	World.h
	World.cpp

	bufferFillers.h
	bufferFillers.cpp
)

set(SRC
	main.cpp
	${All_SRC}
)

set(LIBS
	modernglad
	glfw
	imgui
	rapidjson
	glm
	tinyobjloader
	tinyexr
	tinypngout
	libpng
	tinygltf
	nanoflann
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(SandViewer ${SRC})
target_include_directories(SandViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SandViewer LINK_PRIVATE ${LIBS})

target_compile_definitions(SandViewer PRIVATE -DNOMINMAX)

# For IDE projects
group_source_by_folder(${SRC})

###############################################################################
# Tools - PointCloudConvert

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
target_compile_definitions(PointCloudConvert PRIVATE -DNOMINMAX)

group_source_by_folder(${PointCloudConvert_SRC})

###############################################################################
# Tools - GenerateLeanMaps

set(GenerateLeanMaps_SRC
	Tools/GenerateLeanMaps.cpp

	${AugenCore_SRC}

	Ui/Window.h
	Ui/Window.cpp

	Framebuffer.h
	Framebuffer.cpp
	Framebuffer2.h
	Framebuffer2.cpp
	Filtering.h
	Filtering.cpp
)

set(GenerateLeanMaps_LIBS
	modernglad
	glfw
	glm
	rapidjson
	tinyexr
	tinypngout
	libpng
	tinygltf
)

add_executable(GenerateLeanMaps ${GenerateLeanMaps_SRC})
target_include_directories(GenerateLeanMaps PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GenerateLeanMaps LINK_PRIVATE ${GenerateLeanMaps_LIBS})
set_property(TARGET GenerateLeanMaps PROPERTY FOLDER "Tools")
target_compile_definitions(GenerateLeanMaps PRIVATE -DNOMINMAX)

group_source_by_folder(${GenerateLeanMaps_SRC})

###############################################################################
# Tools - BenchmarkCullingMethods

set(BenchmarkCullingMethods_SRC
	Tools/BenchmarkCullingMethods.cpp

	Ui/Window.h
	Ui/Window.cpp

	Culling/AbstractElementBufferBuilder.h
	Culling/AbstractElementBufferBuilder.cpp
	Culling/PrefixSumElementBufferBuilder.h
	Culling/PrefixSumElementBufferBuilder.cpp
	Culling/GlobalAtomicElementBufferBuilder.h
	Culling/GlobalAtomicElementBufferBuilder.cpp
	Culling/prefixSum.h
	Culling/prefixSum.cpp

	${AugenCore_SRC}
)

set(BenchmarkCullingMethods_LIBS
	modernglad
	glfw
	glm
	rapidjson
	tinyexr
	tinypngout
	libpng
	tinygltf
)

add_executable(BenchmarkCullingMethods ${BenchmarkCullingMethods_SRC})
target_include_directories(BenchmarkCullingMethods PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BenchmarkCullingMethods LINK_PRIVATE ${BenchmarkCullingMethods_LIBS})
set_property(TARGET BenchmarkCullingMethods PROPERTY FOLDER "Tools")
target_compile_definitions(BenchmarkCullingMethods PRIVATE -DNOMINMAX)

group_source_by_folder(${BenchmarkCullingMethods_SRC})

###############################################################################
# Tests

set(SandViewerTests_SRC
	Tests/SandViewerTests.cpp

	Tests/testHzb.h
	Tests/testHzb.cpp

	Tests/testSplineBlur.h
	Tests/testSplineBlur.cpp

	Tests/testPointcloudDistance.h
	Tests/testPointcloudDistance.cpp

	${All_SRC}
)

add_executable(SandViewerTests ${SandViewerTests_SRC})
target_include_directories(SandViewerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(SandViewerTests LINK_PRIVATE ${LIBS})
set_property(TARGET SandViewerTests PROPERTY FOLDER "Tests")
target_compile_definitions(SandViewerTests PRIVATE -DNOMINMAX)
group_source_by_folder(${SandViewerTests_SRC})

