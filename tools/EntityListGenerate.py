import bpy
import math
from mathutils import Matrix, Vector, Quaternion

# Conversion rotation: -90 degrees around X (Z-up -> Y-up)
conv_rot = Matrix.Rotation(-math.pi / 2.0, 4, 'X')

# Configuration
COLLECTION_NAME = "Entities"
TEXT_NAME = "Entities_Output.ini"

# Get the collection
collection = bpy.data.collections.get(COLLECTION_NAME)
if collection is None:
    raise ValueError(f'Collection "{COLLECTION_NAME}" not found')

# Create or overwrite the text datablock
if TEXT_NAME in bpy.data.texts:
    text = bpy.data.texts[TEXT_NAME]
    text.clear()
else:
    text = bpy.data.texts.new(TEXT_NAME)

# Iterate over objects in the collection
for obj in collection.objects:

    # Skip child objects (only root objects)
    if obj.parent is not None:
        continue

    # Split and trim parts: "id | name | classtype"
    parts = [p.strip() for p in obj.name.split('|')]
    if len(parts) != 3:
        raise ValueError(f'Invalid object name format: "{obj.name}"')

    obj_id, obj_name, obj_classtype = parts

    # World matrix
    world_mat = obj.matrix_world.copy()

    # Decompose
    loc, rot, scale = world_mat.decompose()

    #Convert a quaternion rotation from Blender coordinate system to glTF coordinate system.
    #'w' is still at first position.
    rot = obj.matrix_world.to_quaternion()
    quat = Quaternion((rot[0], rot[1], rot[3], -rot[2]))

    converted_scale = Vector((
        scale.x,   # X stays X
        scale.z,   # Z -> Y
        scale.y    # Y -> Z
    ))
    
    converted_loc = Vector((
        loc.x,   # X -> X
        loc.z,   # Z -> Y
        -loc.y    # -Y -> Z
    ))

    # Visibility
    enabled = obj.visible_get()

    # Write section header
    text.write(f"[{obj_name}]\n\n")

    text.write(f'id = {obj_id}\n')
    text.write(f'class = "{obj_classtype}"\n')
    text.write(f'enabled = {str(enabled).lower()}\n\n')

    # Write transform data
    text.write(
        f'position = "{converted_loc.x:.6f} {converted_loc.y:.6f} {converted_loc.z:.6f}"\n'
    )
    text.write(
        f'rotation = "{quat.w:.6f} {quat.x:.6f} {quat.y:.6f} {quat.z:.6f}"\n'
    )
    text.write(
        f'scale = "{converted_scale.x:.6f} {converted_scale.y:.6f} {converted_scale.z:.6f}"\n\n'
    )

    # Write custom properties
    for prop_name, prop_value in obj.items():
        if prop_name in ("_RNA_UI", "fast64"):
            continue

        if isinstance(prop_value, str):
            text.write(f'{prop_name} = "{prop_value}"\n')
        else:
            text.write(f'{prop_name} = {str(prop_value).lower()}\n')

    text.write("\n\n")