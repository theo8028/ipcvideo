#include "Python.h"
#include <stdio.h>
#include <vector>
#include <windows.h>
#include <Memoryapi.h>
#include <mutex >
#include "ipcvideo_base.h"
#include "numpy/npy_common.h"
#include "numpy/arrayobject.h"
#include "slogger.h"


using namespace std;

bool seqToString( PyObject* seqKey,char *szKey,int maxBuffer )
{
    PyObject *seq = PyObject_GetIter(seqKey);    
    if (!seq)
    {
        PyTypeObject* type = seqKey->ob_type;
        const char* py_tpname = type->tp_name;
        printf("seqToString error :  is %s \n",py_tpname);
        return false;
    }
    int index=0;
    size_t keyPos=0;
    PyObject* item;

    while ((item = PyIter_Next(seq))) {
        if( PyUnicode_Check(item) ){
            const char *pSrc = PyUnicode_AsUTF8(item);
            size_t   strLen = strlen(pSrc);
            if( (keyPos+strLen+1) < maxBuffer )
            {
                strcpy(&szKey[keyPos],pSrc);
                keyPos+=strLen;
            }
//            printf("    %d : String(%s)\n",index,szKey);            
        }else{
            PyTypeObject* type = item->ob_type;
            const char* py_tpname = type->tp_name;
            printf("    %d : %s\n",index,py_tpname);
        }
        index++;
    }

    /* clean up and return result */
    Py_DECREF(seq);
    return true;
}

bool parseTuple( PyObject* iterItem) {
    PyObject* item;
    PyObject *seq = PyObject_GetIter(iterItem);
    if (!seq)
    {
        PyTypeObject* type = iterItem->ob_type;
        const char* py_tpname = type->tp_name;
        printf("parseTuple error : is %s \n",py_tpname);
        return false;
    }
    printf("parseTuple \n");
    /* process data sequentially */
    int index=0;
    while ((item = PyIter_Next(seq))) {
        index++;
        PyTypeObject* type = item->ob_type;
        const char* py_tpname = type->tp_name;
        if (PyFloat_Check(item)){
            printf("    %d : Float Number\n",index);
            continue;
        }else  if (PyLong_Check(item)){
            printf("    %d : Int Number\n",index);
            continue;
        }if (PyBytes_Check(item)){
            printf("    %d : Bytes\n",index);
            continue;
        }if (PyByteArray_Check(item)){
            printf("    %d : ByteArray\n",index);
            continue;
        }if( PyUnicode_Check(item) ){   // Python3 
            printf("    %d : String(%s)\n",index,PyUnicode_AsUTF8(item));
            continue;
        }else{
            printf("    %d : %s\n",index,py_tpname);
        }        
    }

    /* clean up and return result */
    Py_DECREF(seq);
    return true;
}

static PyObject* open(PyObject* self, PyObject* args) 
{
    if( isOpenedShareVideo())
    {
        slog_warn("open : The ipc video sharing service area is already open.");
        Py_RETURN_FALSE;
    } 

    char *szIpcName=0;
    if (!PyArg_ParseTuple(args, "s", &szIpcName))
        Py_RETURN_NONE;

    if( !openShareVideo(szIpcName) )
    {
        printf("ipcvideo::open fail : Ipc Name(%s) \n",szIpcName);
        Py_RETURN_FALSE;
    }
    printf("ipcvideo::open sucess :  max_channel(%d) \n",MAX_CHANNEL);
    Py_RETURN_TRUE;
}


static PyObject*  read(PyObject* self, PyObject* args)
{
    if( !isOpenedShareVideo())
        Py_RETURN_FALSE;

    int inputChannel = 0;
    int readChannel=0;

    if (!PyArg_ParseTuple(args, "i", &readChannel))
        Py_RETURN_NONE;

    if (readChannel<0 || readChannel>=MAX_CHANNEL )
    {
        slog_error("read  error : invalid channel\n");
        Py_RETURN_FALSE;
    }

    slog_info("read : channel(%d)\n",readChannel);

    static int frameCount = -2147483646;

    int imgWidth;
    int imgHeight;
    int imgDepth;
    int pixelFormat;
        
    getImageInfo(readChannel,&imgWidth,&imgHeight,&imgDepth,&pixelFormat);
    npy_intp dims[3];
    dims[0]=imgHeight;
    dims[1]=imgWidth;
    dims[2]=imgDepth;

    int bufSize = (int) (imgWidth*imgHeight*imgDepth);
    if( bufSize > 0 )
    {
        PyObject *array = PyArray_SimpleNew(3, dims, NPY_UBYTE);
        char* pDstImg = (char*)PyArray_DATA(array);
        int frameCount;
        double latencySec;

        if( readShareVideo(readChannel,pDstImg,bufSize,&imgWidth,&imgHeight,&imgDepth,&pixelFormat,&frameCount,&latencySec )>0 ){
        	printf("image rx : frameCount(%d), latency(%.3fms) \n", frameCount, latencySec*1000);
            return array;
        }
        Py_DECREF(array);
    }
    Py_RETURN_NONE;
}

static PyObject*  get(PyObject* self, PyObject* args)
{
    if( !isOpenedShareVideo())
        Py_RETURN_FALSE;

    int ch;
    char *strKey=0;

    if (!PyArg_ParseTuple(args, "is", &ch,&strKey))
    {
        slog_error("get : invalid param \n");
        Py_RETURN_NONE;
    }

    if (ch<0 || ch>=MAX_CHANNEL )
    {
        slog_error("get : invalid channel \n");
        Py_RETURN_NONE;
    }

    int imgWidth, imgHeight,imgDepth,pixelFmt;
    getImageInfo(ch,&imgWidth, &imgHeight,&imgDepth,&pixelFmt);
  
    if( strcmp("imgWidth",strKey) == 0 ){
        return PyLong_FromLong(imgWidth);
    }else if( strcmp("imgHeight",strKey) == 0 ){
        return PyLong_FromLong(imgHeight);
    }else if( strcmp("imgDepth",strKey) == 0 ){
        return PyLong_FromLong(imgDepth);
    }else if( strcmp("pixelFmt",strKey) == 0 ){
        return PyLong_FromLong(pixelFmt);
    } 

    Py_RETURN_NONE;
}

static PyObject* write(PyObject* self, PyObject* args, PyObject* argsKey) 
{
    if( !isOpenedShareVideo())
        Py_RETURN_FALSE;

    //printf("ipcvideo::write\n");
    PyObject* oi=NULL;
    int writeChannel=0;
    int pixelFmt=0;

    if (!PyArg_ParseTuple(args, "Oi",&oi,&writeChannel))
    {
        slog_error("write : invalid param \n");
        Py_RETURN_FALSE;
    }
    if (writeChannel<0 || writeChannel>=MAX_CHANNEL )
    {
        slog_error("write : invalid channel \n");
        Py_RETURN_FALSE;
    }

    if( argsKey )
    {
        if( PyDict_Check(argsKey) )
        {
            //printf("is PyDict\n");
            Py_ssize_t j=0;       
            PyObject *key;
            char strKey[100];
            PyObject *value;

            while ( PyDict_Next(argsKey, &j, &key, &value)) {
                seqToString(key,strKey,100);   
                if( strcmp("ch",strKey) == 0 ){
                    writeChannel = PyLong_AsLong(value);
                    //printf(" ch = %d\n",writeChannel);
                }else if( strcmp("pixelFmt",strKey) == 0 ){
                    pixelFmt = PyLong_AsLong(value);
                    //printf(" imgFmt = %d\n",imgFmt);
                }
            }
        }
    }

	if( oi != NULL )
	{        
        if( PyLong_Check(oi) ){
            slog_warn("write :	is Long\n");
        }
        else if( PyFloat_Check(oi) )
        {
             slog_warn("write :	is Float\n");
        }
        else if( PyArray_Check(oi) )
        {
            //printf("	is Array\n");
            PyArrayObject* oarr = (PyArrayObject*) oi;

            bool needcopy = false, needcast = false;
            int typenum = PyArray_TYPE(oarr), new_typenum = typenum;
            //printf("			type : %x\n",typenum);
            //int ndims = PyArray_NDIM(oarr);
            //printf("			ndims : %d\n",ndims);

            const npy_intp* _strides = PyArray_STRIDES(oarr);
            npy_intp* _shape =  PyArray_SHAPE(oarr);

            int ndims = (int)_strides[1];
            int imgWidth  = (int)_shape[1];
            int imgHeight = (int)_shape[0];
            int imgDepth  = ndims;

            slog_info("write :	height(%d)\n", _shape[0]);
            slog_info("write :	width(%d)\n", _shape[1]);
            slog_info("write :	_strides0(%d)\n", _strides[0]);            
            slog_info("write :	_strides1(%d)\n", _strides[1]);
            if (ndims == 3||ndims == 1)
            {
                char* pData = (char*)PyArray_DATA(oarr);
                int bufSize= imgWidth*imgHeight*imgDepth;
                int frameCount=0;                
                if( ndims == 1 )                
                    pixelFmt = IPCV_PixelFormat_Gray;                
                else if( ndims == 3 )
                    pixelFmt = IPCV_PixelFormat_BGR24;

                writeShareVideo(writeChannel,pData,bufSize,imgWidth, imgHeight,imgDepth,pixelFmt, &frameCount);
                slog_trace("			data : %016llx , [%02x][%02x][%02x][%02x]\n",(unsigned __int64)pData,pData[0]&0xFF,(char)pData[1]&0xFF,(char)pData[2]&0xFF,(char)pData[3]&0xFF);
                slog_trace("write : ch(%d), frameCount(%d)\n",writeChannel,frameCount);
                Py_RETURN_TRUE;
            }
            else
            {
                slog_warn("write : unsupported image format\n");
            }
        }else {
            slog_warn("write : is not a numerical tuple\n");
        }
	}
    Py_RETURN_FALSE;
}


static PyObject* close(PyObject* self, PyObject* args) {

    if( closeShareVideo() ){
        slog_info("close : sucess\n");
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}


extern "C" static PyObject* isOpened(PyObject * self, PyObject * args) {
    if( !isOpenedShareVideo())
        Py_RETURN_FALSE;
    Py_RETURN_TRUE;  
}

// Method definition object 
static PyMethodDef pss_methods[] = {
    {
        "open",  open , METH_VARARGS, // open
        "open ipc video service"
    },    
    {
        "read", read, METH_VARARGS,  //read
        "Read, read video frame."
    },     
    {
        "get", get, METH_VARARGS,  //get
        "Get, get info."
    },
    {
        "write", (PyCFunction)(void*)(PyCFunctionWithKeywords)(write) , METH_VARARGS|METH_KEYWORDS, //write
        "Write, write video frame."
    },
    {
        "close", close, METH_VARARGS,
        "close ipc video service"
    },
    {
        "isOpened", isOpened, METH_VARARGS,
        "camera open state "
    },
    {NULL, NULL, 0, NULL}
};

// Module definition
// The arguments of this structure tell Python what to call your extension,
// what it's methods are and where to look for it's method definitions
static struct PyModuleDef ipc_video_definition = {
    PyModuleDef_HEAD_INIT,
    "ipcvideo",
    "Python base ipc video module ",
    -1,
    pss_methods
};

// Module initialization
// Python calls this function when importing your extension. It is important
// that this function is named PyInit_[[your_module_name]] exactly, and matches
// the name keyword argument in setup.py's setup() call.
PyMODINIT_FUNC PyInit_ipcvideo(void) {
    Py_Initialize();
    import_array();
    return PyModule_Create(&ipc_video_definition);
}