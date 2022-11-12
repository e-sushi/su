import os

for path, subdirs, files in os.walk("tests/"):
    for name in files:
        os.rename(os.path.join(path, name), os.path.join(path, name).replace(".su",".amu"))