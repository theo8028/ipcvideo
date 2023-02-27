import cv2
import ipcvideo as ipcv
import numpy as np
import random

if ipcv.open("IPCVIDEO") :    
    imgWidth=1024
    imgHeight=768
    ch = 0
    while True :
        randColor=[random.randrange(0,255),random.randrange(0,255),random.randrange(0,255)]
        imgColor = np.full((imgHeight, imgWidth, 3),randColor,np.uint8)
        ipcv.write(imgColor,ch)
        cv2.imshow("ipcvideo - image tx",imgColor)
        if cv2.waitKey(30) == 27 :
            break
else :
    print("fail : ipc video share service open ")