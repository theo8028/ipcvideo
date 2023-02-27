import ipcvideo as ipcv
import cv2

if ipcv.open("IPCVIDEO")  :        
    ch = 0
    while cv2.waitKey(1) != 27 :
        imgColor = ipcv.read(ch)    # If python communicates with Terminal Services, it must run in administrator mode to open "Global\ipcname"
        if imgColor is not None:
            cv2.imshow("ipcvideo - image rx",imgColor)
            cv2.waitKey(1)
else :
    print("fail : ipc video share service open ")