Grain Viewer
============

*End User Manual*

GrainViewer is the official implementation of the research paper *"Real-time multiscale rendering of dense dynamic stackings"* published in Computer Graphics forum (Pacific Graphics) by Élie Michel and Tamy Boubekeur.

Basic Usage
-----------

If you downloaded a release zip, for a first test, you can simply run the GrainViewerDemo shortcut. This will run GrainViewer.exe with the scene file data/ball.json. For other examples, just run GrainViewer with the other json files of this directory (you can for instance drag and drop the json scene file onto GrainViewer.exe in the file explorer.)

If you are using the git repository, example scenes can be found in [share/scenes](share/scenes). To use these, you will need to download additional binary data using the `download-minimal-data.bat` or `download-minimal-data.sh` script (or the `full` version for additionnal examples). Run for instance:

    build/src/GrainViewer/GrainViewer share/scenes/not01-heap.json

For a more advanced use, you can edit the json files with a text editor. See "Scene files" section bellow.


User Interface
--------------

Try left, middle and right mouse buttons to navigate in the 3D view.

The UI in the right-hand sidebar is organized in sections (Deferred Shading, World, atc. and one section per object). Each section contains one or several panels that are shown when the section is selected. You can select several sections by holding the control key while clicking on them.

Each panel contains a set of options to play with. In sections related to scene objects, a section corresponds to a "Behavior" as listed in the json scene file.


Runtime Shortcuts
-----------------

R: Reload shaders

Ctrl+R: Reload the whole scene

A: Tilt left

E: Tilt right

U: Show/hide UI

P: Show/hide sidebar

ESCAPE: Quit

SPACEBAR: Play/pause animation

C: Display in the console current viewport info in a format that can be copy-pasted into the json files to save it, in camera definitions: `"turntable": { ... }`

Ctrl+C: Clear the scene, unload everything.


Scene Files
-----------

Scene files are standard json files. You can look at existing json files to follow this section more easily. All root keys are held in a global object name "augen". The following keys can be found in this global object: `cameras`, `deferredShader`, `scene`, `lights`, `shaders`, `world` and `objects`.

Options in `deferredShader`, `scene` and `world` directly map to the similarly named sections in UI. The other are lists of items:

### Cameras

A camera is first defined by its `projection`, which is either `perspective` or `orthographic`. Other options include `fov` or `orthographicScale`, `near` (near plane), `far` (far plane). The camera position can be given by copy-pasting the `turntable` object output in console when pressing C. Some other options about animation and recording can be found in section Recording bellow.

The first camera is used for the viewport. Other cameras are currently not used. An extra camera is used internally when the 'Freeze Occlusion Camera' option of the scene is turned on.

### Lights

Lights are point lights, unless they use a shadow map (`hasShadowMap` is set to true) in which case they are spot lights oriented toward the origin `(0,0,0)`. The spot aperture is given by `shadowMapFov`.

### Shaders

The shader list is a key-value store where keys are arbitrary names used in objects to refer to them. The shader object binds these names to actual filenames with entries like:

	"Name": "filename"

The filename is a prefix, the engine looks for files at share/shaders/filename.{frag,vert,geo}.glsl. Alternatively, an object with more details can be provided:

	"Name": {
		"baseFile": "filename",
		"defines": [ "SOME_DEFINE" ],
		"type": "compute" or "render",
		"snippets": { "snippet_identifier": "some code();" }
	},

This is a way to add preprocessor definitions to test different variants of a shader. This notation is also required to load compute shader (from filename.comp.glsl), because the default is render shader. Snippets can be inserted into shaders if they use #include "sys:snippet_identifier".

### Objects

Objects are the core of scene files. Each object is a stack of "Behavior" component implementing different features such as data loading, rendering etc. Each component has its own set of options that links directly with the UI, with the mapping that "A Given Name" in the UI will be "aGivenName" in the json file. Enum can be given either as their numerical id or using their name, e.g. "samplingMode": "Mixed" for ImpostorSandRenderer.

**NB** *All relative paths in the json file are given wrt the location of the json file itself.*


Recording
---------

GrainViewer can output sequences of images in PNG files if `isRecordEnabled` is set to true in the camera settings. The output prefix name is given by the `outputFrameBase` option. It is appended with a 6-digits frame number and the .png extension. For instance, if its value is "render/frame" then output file will be named render/frame000000.png, render/frame000001.png, etc. Output directories are created if they don't already exist.

In order to automatically stop the program after a given frame, you can set in `scene` settings the `quitAfterFrame` option. If you are using some animation, also set `realTime` to true to ensure that animation playback is based on frame number rather than the clock. This might slow down the program while recording, but ensures that all frames are output. You may also want to turn UI off using "ui": false in scene settings.


Animation
---------

There is no consistent way of animating properties. Some properties though can take buffer files instead of values. It is the case for instance of camera view matrix:

	"viewMatrix": {
		"buffer": "Anim/turnaround.bin",
		"startFrame": 0
	}


The .bin file format is a succession of raw 32-bit floats, 16 by frame. It can be generated in Blender. The same apply for the "Transform" component for instance.


Exporting from Blender
----------------------

### Meshes

When exporting OBJ files from Blender, make sure they use the following axis convention: Z up, Y forward.

### Animations

In share/scripts, there are Blender operators for outputing camera location or object's transform matrix in our ad-hoc .bin files. Some of these scripts are also useful to generate impostors.


Impostors
---------

There are two ways of generating the impostor atlases. One is to generate them in an external program, e.g. Blender. The other is to generate them in engine, at load time. The former is more flexible and allows advanced supersampling, while the former ensure consistency between impostors and instances.

When the `bake` option of impostors is turned on, impostors are baked in engine. Additionally, the `save` option output them to files, but beware of not overriding existing impostors. The same filename option is used to either load atlas from precomputed images or save images baked in engine.

Baking depends on the following options:
 - angularDefinition: Number of precomputed views, must be in the form 2n²
 - spatialDefinition: Number of pixel per side of an impostor map

When using Blender to precompute impostor, first generate an octahedron with the appropriate resolution n using make_octahedron.py. Then load it with the blender script, it will apply to the current camera. Don't forget to set color management to None to avoid bad gamma surprises.
