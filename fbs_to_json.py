import os

dirs = [
  ('assets/art/actors', 'fb/actor.fbs'),
  ('assets/art/meshes', 'fb/mesh.fbs'),
  ('assets/art/skeletons', 'fb/skeleton.fbs'),
  ('assets/art/terrains', 'fb/terrain.fbs'),
  ('assets/art/textures', 'fb/texture.fbs')
]

def RecrusiveListing(dir, suffix):
  files = []
  for r, _, f in os.walk(dir):
    for file in f:
        if suffix in file:
            files.append((r, file))
  return files

for dir, schema in dirs:
  files = RecrusiveListing(dir, '.fb')
  for r, in_file in files:
    in_file_path = os.path.join(r, in_file)
    os.system(f'flatc --json -o {r} --raw-binary {schema} -- {in_file_path}')