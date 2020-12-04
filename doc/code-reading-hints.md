Code Reading Hints
==================

*Here are some misc notes about design choices.*

### Properties

Usually when there is a `struct Properties` defined inside of a class, its fields are declared for introspection with `REFL_FIELD`s at the end of the file (that must be in global scope unfortunately).

These properties are used automatically for serialization, UI drawing and shader uniforms. Note that not all parts of the code migrated to this design yet.

### Grain Rendering

Core code bits specific to grain rendering is located within the [`Behavior`](src/GrainViewer/Behavior) directory. The remaining is more of a basic generic engine. A grain stacking object will typically follow this pattern in the json scene file:

```json
{
	"type": "RuntimeObject",
	"behaviors": [
		{ "type": "TransformBehavior", ... },
		{ "type": "MeshDataBehavior", ... },
		{ "type": "PointCloudDataBehavior", ... },
		{ "type": "GrainBehavior", ... },
		{ "type": "PointCloudSplitter", ... },
		{ "type": "InstanceGrainRenderer", ... },
		{ "type": "ImpostorGrainRenderer", ... },
		{ "type": "FarGrainRenderer", ... },
	]
},
```

Each of these behaviors is defined in a separate class, the last four correspond to the four steps of the rendering pipeline as described in Figure 2 of the reference paper. The `MeshDataBehavior` defines the geometry of a single grain, and the `PointCloudDataBehavior` tells where the grains are. The `SandBehavior` defines the grain size and may either load or bake the impostor atlases at init time. And the `Transform` is just a global affine transform applyed to the whole object.

You may omit the `Transform` and some of the `Renderers`. You can also omit the `PointCloudSplitter`, in which case all renderers will render all grains so in this case you'll likely want to set up only one renderer.


