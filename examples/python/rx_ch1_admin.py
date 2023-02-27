import ipcvideo as ipcv
import cv2

ch = 1
if ipcv.open("Global\IPCVIDEO") :     # 터미널 서비스 통신시에는 관리자 모드로 실행하고 Global 모드로 오픈 
    while cv2.waitKey(1) != 27 :
        imgColor = ipcv.read(ch)
        if imgColor is not None:
            cv2.imshow("ipcvideo - image rx",imgColor)
            cv2.waitKey(1)
else :
    print("ipc video open fail : run in admin mode")