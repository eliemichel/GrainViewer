# This file is part of GrainViewer
#
# Copyright (c) 2017 - 2020 -- Télécom Paris (Élie Michel <elie.michel@telecom-paris.fr>)
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

if (DEV_MOD)
	add_definitions(-DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
else ()
	add_definitions(-DSHARE_DIR="./share")
endif ()

enable_cpp17()
enable_multiprocessor_compilation()

###############################################################################
# Source subsets

set(Core_SRC
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
	EnvironmentVariables.h
	EnvironmentVariables.cpp
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
	${Core_SRC}

	Behavior/GltfDataBehavior.h
	Behavior/GltfDataBehavior.cpp
	Behavior/MeshDataBehavior.h
	Behavior/MeshDataBehavior.cpp
	Behavior/MeshRenderer.h
	Behavior/MeshRenderer.cpp
	Behavior/TransformBehavior.h
	Behavior/TransformBehavior.cpp
	Behavior/LightGizmo.h
	Behavior/LightGizmo.cpp
	Behavior/FarGrainRenderer.h
	Behavior/FarGrainRenderer.cpp
	Behavior/PointCloudDataBehavior.h
	Behavior/PointCloudDataBehavior.cpp
	Behavior/PointCloudSplitter.h
	Behavior/PointCloudSplitter.cpp
	Behavior/PointCloudView.h
	Behavior/InstanceGrainRenderer.h
	Behavior/InstanceGrainRenderer.cpp
	Behavior/ImpostorGrainRenderer.h
	Behavior/ImpostorGrainRenderer.cpp
	Behavior/GrainBehavior.h
	Behavior/GrainBehavior.cpp
	Behavior/QuadMeshData.h
	Behavior/QuadMeshData.cpp

	utils/shader.h
	utils/shader.cpp
	utils/jsonutils.h
	utils/jsonutils.cpp
	utils/guiutils.h
	utils/guiutils.cpp
	utils/mathutils.h
	utils/mathutils.cpp
	utils/behaviorutils.h
	utils/behaviorutils.cpp
	utils/ScopedFramebufferOverride.h
	utils/ReflectionAttributes.h
	utils/impostor.glsl.h
	utils/impostor.glsl.cpp

	Ui/Dialog.h
	Ui/Widgets.h
	Ui/Widgets.cpp
	Ui/Gui.h
	Ui/Gui.cpp
	Ui/TestGui.h
	Ui/TestGui.cpp
	Ui/Window.h
	Ui/Window.cpp
	Ui/FarGrainRendererDialog.h
	Ui/FarGrainRendererDialog.cpp
	Ui/SceneDialog.h
	Ui/SceneDialog.cpp
	Ui/LightGizmoDialog.h
	Ui/LightGizmoDialog.cpp
	Ui/TransformDialog.h
	Ui/TransformDialog.cpp
	Ui/DeferredShadingDialog.h
	Ui/DeferredShadingDialog.cpp
	Ui/WorldDialog.h
	Ui/WorldDialog.cpp
	Ui/PointCloudSplitterDialog.h
	Ui/PointCloudSplitterDialog.cpp
	Ui/InstanceGrainRendererDialog.h
	Ui/InstanceGrainRendererDialog.cpp
	Ui/ImpostorGrainRendererDialog.h
	Ui/ImpostorGrainRendererDialog.cpp
	Ui/GrainBehaviorDialog.h
	Ui/GrainBehaviorDialog.cpp
	Ui/GlobalTimerDialog.h
	Ui/GlobalTimerDialog.cpp
	Ui/MeshRendererDialog.h
	Ui/MeshRendererDialog.cpp
	Ui/QuadMeshDataDialog.h
	Ui/QuadMeshDataDialog.cpp

	AnimationManager.h
	AnimationManager.cpp
	Behavior.h
	BehaviorRegistry.h
	BehaviorRegistry.cpp
	BehaviorRegistryEntry.h
	Camera.h
	Camera.cpp
	IBehaviorHolder.h
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
	GlobalTimer.h
	GlobalTimer.cpp
	ImpostorAtlasMaterial.h
	ImpostorAtlasMaterial.cpp
	IPointCloudData.h
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
	SerializationType.h
	ShadowMap.h
	ShadowMap.cpp
	StandardMaterial.h
	StandardMaterial.cpp
	Triangle.h
	TurntableCamera.h
	TurntableCamera.cpp
	ViewLayerMask.h
	ViewLayerMask.cpp
	World.h
	World.cpp
	PostEffect.h
	PostEffect.cpp

	bufferFillers.h
	bufferFillers.cpp
)

set(LIBS
	modernglad
	glfw
	imgui
	rapidjson
	glm
	tinyobjloader
	tinyexr
	tinygltf
	nanoflann
	refl-cpp
)

set(SRC
	main.cpp
	${All_SRC}
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCC)

add_executable(GrainViewer ${SRC})
target_include_directories(GrainViewer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GrainViewer LINK_PRIVATE ${LIBS})

target_compile_definitions(GrainViewer PRIVATE -DNOMINMAX)
#target_treat_warnings_as_errors(GrainViewer)
target_set_default_command_line(GrainViewer ${PROJECT_SOURCE_DIR}/share/scenes/nut01-heap.json)

# For IDE projects
group_source_by_folder(${SRC})
# Set as startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GrainViewer)


###############################################################################
# Tools - PointCloudConvert

set(PointCloudConvert_SRC
	Tools/PointCloudConvert.cpp
	Tools/filterPointToPointDistance.h
	Tools/filterPointToPointDistance.cpp

	utils/strutils.h
	utils/strutils.cpp
	utils/fileutils.h
	utils/fileutils.cpp
	utils/debug.h
	utils/debug.cpp
	Logger.h
	Logger.cpp
	PointCloud.h
	PointCloud.cpp

	Ui/Window.h
	Ui/Window.cpp
	Ui/TestGui.h
	Ui/TestGui.cpp
	Ui/Widgets.h
	Ui/Widgets.cpp
)

set(PointCloudConvert_LIBS
	glfw
	modernglad
	glm
	nanoflann
	imgui
)

add_executable(PointCloudConvert ${PointCloudConvert_SRC})
target_include_directories(PointCloudConvert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(PointCloudConvert LINK_PRIVATE ${PointCloudConvert_LIBS})
set_property(TARGET PointCloudConvert PROPERTY FOLDER "Tools")
target_compile_definitions(PointCloudConvert PRIVATE -DNOMINMAX)

group_source_by_folder(${PointCloudConvert_SRC})
