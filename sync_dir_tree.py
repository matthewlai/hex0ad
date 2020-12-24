# We need a matching skeleton directory tree for src/ and third_party/ in dep/ and obj/
# This script creates those directories if they don't already exist.

import os

def all_directories(root):
	directories = []
	for dir_root, _, _ in os.walk(root):
		if dir_root is not None:
			directories.append(dir_root)
	return directories

def create_if_not_exist(dir_name):
	try:
		os.mkdir(dir_name)
		print("Created {}".format(dir_name))
	except FileExistsError:
		pass
	with open(dir_name + os.sep + ".hidden", 'w') as f:
		f.write("This file allows us to check in an empty directory to git")

directories_to_create = []
directories_to_create.extend(all_directories("third_party"))
directories_to_create.extend(all_directories("src"))

# Nothing under third_party root
directories_to_create = filter(lambda s : s != "third_party", directories_to_create)

# Exclude GLM and GLI (header-only libraries)
directories_to_create = filter(lambda s : not s.startswith("third_party\\glm"), directories_to_create)
directories_to_create = filter(lambda s : not s.startswith("third_party\\gli"), directories_to_create)

# FCollada include dir
directories_to_create = filter(lambda s : not s.startswith("third_party\\fcollada\\include"), directories_to_create)

# We are not building FCollada tests
directories_to_create = filter(lambda s : "FColladaTest" not in s, directories_to_create)

# Don't care about xcodeproj directories
directories_to_create = filter(lambda s : "xcodeproj" not in s, directories_to_create)

for dir_name in directories_to_create:
	create_if_not_exist("dep\\" + dir_name)
	create_if_not_exist("obj\\" + dir_name)