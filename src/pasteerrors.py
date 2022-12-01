import os

for root, dirs, files in os.walk(".", topdown=False):
    for file in files:
        if file.endswith(".cpp"):
            hand = open(root+"/"+file, "r", encoding="utf-8")
            buff = hand.read()
            hand.close()
            place = buff.find("/* @pasteenum(errors) */")
            if place == -1: continue
            print(buff[place:])
            l = len("/* @pasteenum(errors) */")

            out = (
                   f"\nenum{{\n"
                   f"\tError_Preprocessor"
                )


            buff = buff[:place+l] + out + buff[place+l:]   

            print(buff)