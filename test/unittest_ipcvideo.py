import cv2
import ipcvideo as ipcv
import numpy as np
import random

imgWidth=1024
imgHeight=768

if ipcv.open("IPCVIDEO") :
    print("\n\n==================================================")
    print("unittest start")
    print("==================================================")

    randColor=[random.randrange(0,255),random.randrange(0,255),random.randrange(0,255)]
    imgTx = np.full((imgHeight, imgWidth, 3),randColor,np.uint8)
    print("\nunittest 1 : channel 0 tx rx\n")
    print("  :ipcv.write(imgColor,0)")
    ipcv.write(imgTx,0)
    print("  :ipcv.read(0)")
    imgRx=ipcv.read(0)
    if not (imgTx == imgRx).all() :
        print("  ==> error : tx/rx mismatch ")
    else :
        print("  ==> sucess")

    randColor=[random.randrange(0,255),random.randrange(0,255),random.randrange(0,255)]
    imgTx = np.full((imgHeight, imgWidth, 3),randColor,np.uint8)
    print("\nunittest 2 : channel 1 tx/rx")
    print(" : ipcv.write(imgColor,1)")
    ipcv.write(imgTx,1)
    imgRx=ipcv.read(1)
    if not (imgTx == imgRx).all() :
        print("  ==> error : tx/rx mismatch ")
        cv2.imshow('tx',imgTx)
        cv2.imshow('rx',imgRx)
        cv2.waitkey(0)
    else :
        print("  ==> sucess")
 
    print("\nunittest 3 : exception ")
    print("  ==> ipcv.write(imgColor,2)")
    ipcv.write(imgTx,2)
    print("  ==> ipcv.write(imgColor,-1)")  
    ipcv.write(imgTx,-1)

    print("\n==================================================")
    print("unittest end")
    print("==================================================")
else:            
    print("fail : ipc video share service open ")