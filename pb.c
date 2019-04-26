#include <Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "deviceapps.pb-c.h"

#define MAGIC  0xFFFFFFFF
#define DEVICE_APPS_TYPE 1

typedef struct pbheader_s {
    uint32_t magic;
    uint16_t type;
    uint16_t length;
} pbheader_t;
#define PBHEADER_INIT {MAGIC, 0, 0}

static DeviceApps get_message_from_pyobj(PyObject *obj){
    PyObject *py_device, *py_type, *py_id, *py_lat, *py_lon, *py_apps;
    //PyListObject* py_apps;

    DeviceApps msg = DEVICE_APPS__INIT;
    DeviceApps__Device device = DEVICE_APPS__DEVICE__INIT;

    unsigned i;
    char *device_id, *device_type;

    py_device = PyDict_GetItemString(obj, "device"); // PyDict
    py_type = PyDict_GetItemString(py_device, "type"); // char*
    py_id = PyDict_GetItemString(py_device, "id"); // long
    py_lat = PyDict_GetItemString(obj, "lat"); // double
    py_lon = PyDict_GetItemString(obj, "lon"); // double
    py_apps = PyDict_GetItemString(obj, "apps"); // list of int

    device_type = PyString_AsString(py_type);
    device_id = PyString_AsString(py_id);

    device.has_id = 1;
    device.id.data = (uint8_t*)device_id;
    device.id.len = strlen(device_id);
    device.has_type = 1;
    device.type.data = (uint8_t*)device_type;
    device.type.len = strlen(device_type);
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
    msg.apps = malloc(sizeof(uint32_t) * msg.n_apps);
    
    for(i=0; i<=msg.n_apps; i++){
       msg.apps[i] = (uint32_t) PyInt_AsLong(PyList_GetItem(py_apps, i)); 
    }

    return msg;

}

// Read iterator of Python dicts
// Pack them to DeviceApps protobuf and write to file with appropriate header
// Return number of written bytes as Python integer
static PyObject* py_deviceapps_xwrite_pb(PyObject* self, PyObject* args) {
    PyObject *obj;
    DeviceApps msg;
    pbheader_t header = PBHEADER_INIT;

    const char *path;
    void *buf;
    unsigned len, total_length=0;

    if (!PyArg_ParseTuple(args, "Os", &obj, &path))
        return NULL;

    if (!PyIter_Check(obj)){
        obj = PyObject_GetIter(obj);
    } 
    FILE *fout = fopen(path, "w");

    while (true) {
      PyObject *next = PyIter_Next(obj);
      if (!next) {
        // nothing left in the iterator
        break;
      }

      msg = get_message_from_pyobj(next);
      len = device_apps__get_packed_size(&msg);

      buf = malloc(len);
      device_apps__pack(&msg, buf);

      header.type = DEVICE_APPS_TYPE; 
      header.length = len;

      fwrite(&header, sizeof(pbheader_t), 1, fout);
      fwrite(buf, len, 1, fout);
      total_length += len;

      //Py_DECREF();

      free(msg.apps);
      free(buf);

     
    }

    printf("Write to: %s\n", path);
    // TODO gzip file
    fclose(fout);
    return  Py_BuildValue("i", total_length); 
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
