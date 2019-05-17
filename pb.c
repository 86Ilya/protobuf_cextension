#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "deviceapps.pb-c.h"
#include <zlib.h>

#define MAGIC  0xFFFFFFFF
#define DEVICE_APPS_TYPE 1

typedef struct pbheader_s {
    uint32_t magic;
    uint16_t type;
    uint16_t length;
} pbheader_t;
#define PBHEADER_INIT {MAGIC, 0, 0}


// Read iterator of Python dicts
// Pack them to DeviceApps protobuf and write to file with appropriate header
// Return number of written bytes as Python integer
static PyObject* py_deviceapps_xwrite_pb(PyObject* self, PyObject* args) {
    size_t i;
    char *device_id = NULL, *device_type = NULL;
    const char *path;
    void *buf = NULL;
    uint32_t len, total_length=0;
    PyObject *obj = NULL, *py_device = NULL, *py_type = NULL,
             *py_id = NULL, *py_lat = NULL, *py_lon = NULL,
             *py_app = NULL, *py_apps = NULL, *messages = NULL, *next_message = NULL;
    gzFile fout = NULL;
    DeviceApps msg = DEVICE_APPS__INIT;
    DeviceApps__Device device = DEVICE_APPS__DEVICE__INIT;
    pbheader_t header = PBHEADER_INIT;

    if (!PyArg_ParseTuple(args, "Os", &obj, &path))
        return NULL;

    messages = PyObject_GetIter(obj);
	if (!messages) {
		PyErr_SetString(PyExc_TypeError, "messages must be iterable");
		goto error;
	}

    fout = gzopen(path, "wb");

    while (true) {
      next_message = PyIter_Next(messages);
      if (!next_message) {
        // nothing left in the iterator
        break;
      }

    // starting to parse python variables
    py_device = PyDict_GetItemString(next_message, "device"); // PyDict
    if (py_device == NULL || !PyDict_Check(py_device)) {
        PyErr_SetString(PyExc_TypeError, "Device should be dict");
        goto error;
    }

    // py_type -> device_type is char*
    if ((py_type = PyDict_GetItemString(py_device, "type"))){
        device_type = PyString_AsString(py_type);
        if (!device_type) {
            goto error;
        }
        // Set device type
        device.has_type = 1;
        device.type.data = (uint8_t*)device_type;
        device.type.len = strlen(device_type);

    }
    else{
        // If device type doesn't exist
        device.has_type = 0;
    }

    // py_id converts to device_id char* 
    if ((py_id = PyDict_GetItemString(py_device, "id"))){
        device_id = PyString_AsString(py_id);
        if (!device_id) {
            goto error;
        }
        device.id.data = (uint8_t*)device_id;
        device.id.len = strlen(device_id);
        device.has_id = 1;
    }
    else{
        // If device id doesn't exist
        device.has_id = 0;
        }
    
    // py_lat converts to float 
    if((py_lat = PyDict_GetItemString(next_message, "lat"))){
        msg.has_lat = true;
        msg.lat = PyFloat_AsDouble(py_lat);
        if (msg.lat == -1 && PyErr_Occurred()){
            goto error;
        }
    }
    else{
        msg.has_lat = false;
    }
    
    // py_lon converts to float 
    if((py_lon = PyDict_GetItemString(next_message, "lon"))){
        msg.has_lon = true;
        msg.lon = PyFloat_AsDouble(py_lon);
        if (msg.lon == -1 && PyErr_Occurred()){
            goto error;
        }
    }
    else{
        msg.has_lon = false;
    }

    // py_apps converts to list of ints
    if((py_apps = PyDict_GetItemString(next_message, "apps"))){
        if (!PyList_Check(py_apps)) {
                    PyErr_SetString(PyExc_TypeError, "Device apps should be list");
                    goto error;
            }
            msg.n_apps = (size_t)PyList_Size(py_apps);
            msg.apps = (uint32_t *) malloc(sizeof(uint32_t) * msg.n_apps);
            
            for(i=0; i<msg.n_apps; i++){
               py_app = PyList_GetItem(py_apps, i);
               // py_app converts to int
               if(!PyInt_Check(py_app)){
                    PyErr_SetString(PyExc_TypeError, "app should be of type int");
                    goto error;
               }
               msg.apps[i] = (uint32_t) PyInt_AsLong(py_app); 
               if(msg.apps[i] == -1 && PyErr_Occurred()){
                   goto error;
               }
            }
    }

    msg.device = &device;
    
    if(py_lat != NULL){
        msg.has_lat = true;
        msg.lat = PyFloat_AsDouble(py_lat);
    }
    else{
        msg.has_lat = false;
    }
    
    if(py_lon != NULL){
        msg.has_lon = true;
        msg.lon = PyFloat_AsDouble(py_lon);
    }
    else{
        msg.has_lon = false;
    }

    msg.n_apps = (size_t)PyList_Size(py_apps);
    msg.apps = (uint32_t *) malloc(sizeof(uint32_t) * msg.n_apps);
    
    for(i=0; i<msg.n_apps; i++){
       msg.apps[i] = (uint32_t) PyInt_AsLong(PyList_GetItem(py_apps, i)); 
    }

    len = device_apps__get_packed_size(&msg);

    buf = malloc(len);
    device_apps__pack(&msg, (uint8_t *)buf);

    header.type = DEVICE_APPS_TYPE; 
    header.length = len;

    gzwrite(fout, &header, sizeof(pbheader_t));
    gzwrite(fout, buf, len);
    total_length += len;

    free(msg.apps);
    free(buf);

    Py_CLEAR(next_message);
     
    }
    Py_CLEAR(messages);

    printf("Write to: %s\n", path);
    gzclose(fout);
    
    Py_XDECREF(messages);
	Py_XDECREF(next_message);
    return Py_BuildValue("i", total_length); 
error:
    if(messages){
        Py_XDECREF(messages);
    }

    if(next_message){
        Py_XDECREF(next_message);
    }
    if(msg.apps){
      free(msg.apps);
    }
    if(buf){
      free(buf);
    }
    return NULL;
}

// Unpack only messages with type == DEVICE_APPS_TYPE
// Return iterator of Python dicts
static PyObject* py_deviceapps_xread_pb(PyObject* self, PyObject* args) {
    const char* path;

    if (!PyArg_ParseTuple(args, "s", &path))
        return NULL;

    printf("Read from: %s\n", path);
    Py_RETURN_NONE;
}


static PyMethodDef PBMethods[] = {
     {"deviceapps_xwrite_pb", py_deviceapps_xwrite_pb, METH_VARARGS, "Write serialized protobuf to file fro iterator"},
     {"deviceapps_xread_pb", py_deviceapps_xread_pb, METH_VARARGS, "Deserialize protobuf from file, return iterator"},
     {NULL, NULL, 0, NULL}
};


PyMODINIT_FUNC initpb(void) {
     (void) Py_InitModule("pb", PBMethods);
}
