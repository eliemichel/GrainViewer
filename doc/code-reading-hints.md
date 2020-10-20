Code Reading Hints
==================

*Here are some misc notes about design choices.*

### Properties

Usually when there is a `struct Properties` defined inside of a class, its fields are declared for introspection with `REFL_FIELD`s at the end of the file (that must be in global scope unfortunately).

These properties are used automatically for serialization, UI drawing and shader uniforms. Note that not all parts of the code migrated to this design yet.
