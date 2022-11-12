import os
import re

for path, dirs, files in os.walk("tests/"):
    for file in files:
        if file.endswith(".amu"):
            f = open(path+"/"+file,"r",encoding="utf8")
            s = f.read()
            s = re.sub(r"s32 main\(\)", r"main() : s32", s)
            s = re.sub(r"(\b(?!return|else|if)\w+) +(\w+)( *=| *;)", r"\2:\1\3", s)
            f.close()
            f = open(path+"/"+file,"w",encoding="utf8")
            f.write(s)


