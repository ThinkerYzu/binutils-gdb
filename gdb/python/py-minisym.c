#include "defs.h"
#include "block.h"
#include "exceptions.h"
#include "frame.h"
#include "symtab.h"
#include "symfile.h"
#include "python-internal.h"
#include "objfiles.h"

typedef struct sympy_minisym_object {
  PyObject_HEAD
  /* The GDB symbol structure this object is wrapping.  */
  struct minimal_symbol *msym;
  /* A MiniSymbol object is associated with an objfile, so keep track with
     doubly-linked list, rooted in the objfile.  This lets us
     invalidate the underlying struct minimal_symbol when the objfile is
     deleted.  */
  struct sympy_minisym_object *prev;
  struct sympy_minisym_object *next;
} msym_object;


#define MSYMPY_REQUIRE_VALID(msym_obj, msym)		\
  do {							\
    msym = msym_object_to_msym (msym_obj);		\
    if (msym == NULL)					\
      {							\
	PyErr_SetString (PyExc_RuntimeError,		\
			 _("MiniSymbol is invalid."));	\
	return NULL;					\
      }							\
  } while (0)

/* Return the minimal symbol that is wrapped by this MiniSymbol object.  */
static struct minimal_symbol *
msym_object_to_msym (PyObject *obj)
{
  if (! PyObject_TypeCheck (obj, &msym_object_type))
    return NULL;
  return ((msym_object *) obj)->msym;
}


static PyObject *
msym_to_msym_object (struct minimal_symbol *msym)
{
  msym_object *msym_obj;

  msym_obj = PyObject_New (msym_object, &msym_object_type);
  if (msym_obj) {
    msym_obj->msym = msym;
  }

  return (PyObject *) msym_obj;
}

static void
msympy_dealloc (PyObject *obj)
{
}

static PyObject *
msympy_str (PyObject *self)
{
  PyObject *result;
  struct minimal_symbol *msym = NULL;

  MSYMPY_REQUIRE_VALID (self, msym);

  result = PyString_FromString (SYMBOL_PRINT_NAME (msym));

  return result;
}

static PyObject *
msympy_get_name (PyObject *self, void *closure)
{
  struct minimal_symbol *msym = NULL;
  const char *msym_name;

  MSYMPY_REQUIRE_VALID(self, msym);

  msym_name = SYMBOL_PRINT_NAME (msym);

  return PyString_FromString (msym_name);
}

static PyObject *
msympy_get_address (PyObject *self, void *closure)
{
  struct minimal_symbol *msym = NULL;
  CORE_ADDR addr;

  MSYMPY_REQUIRE_VALID(self, msym);

  addr = SYMBOL_VALUE_ADDRESS(msym);

  return PyInt_FromLong (addr);
}

PyObject *
gdbpy_lookup_symbol_addr (PyObject *self, PyObject *args)
{
  struct obj_section *osect;
  CORE_ADDR addr, sect_addr;
  struct minimal_symbol *msym;
  PyObject *msym_object;

  if (! PyArg_ParseTuple(args, "l", &addr))
    return NULL;

  osect = find_pc_section(addr);
  if (osect == NULL)
    return NULL;

  sect_addr = overlay_mapped_address (addr, osect);

  if (obj_section_addr (osect) > addr ||
      sect_addr >= obj_section_endaddr (osect) ||
      !(msym =
	lookup_minimal_symbol_by_pc_section(sect_addr, osect).minsym)) {
    return NULL;
  }

  msym_object = msym_to_msym_object(msym);
  return msym_object;
}

int
gdbpy_initialize_minisyms (void)
{
  if (PyType_Ready (&msym_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "MiniSymbol",
				 (PyObject *) &symbol_object_type);
}

static PyGetSetDef msym_object_getset[] = {
  { "name", msympy_get_name, NULL,
    "Name of the minimal symbol.", NULL },
  { "address", msympy_get_address, NULL,
    "Address of the minimal symbol.", NULL },
  { NULL }			/* Sentinel */
};

PyTypeObject msym_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.MiniSymbol",		  /*tp_name*/
  sizeof (msym_object),		  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  msympy_dealloc,		  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  0,				  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  0,				  /*tp_as_mapping*/
  0,				  /*tp_hash */
  0,				  /*tp_call*/
  msympy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,		  /*tp_flags*/
  "GDB minimal symbol object",	  /*tp_doc */
  0,				  /*tp_traverse */
  0,				  /*tp_clear */
  0,				  /*tp_richcompare */
  0,				  /*tp_weaklistoffset */
  0,				  /*tp_iter */
  0,				  /*tp_iternext */
  NULL,				  /*tp_methods */
  0,				  /*tp_members */
  msym_object_getset		  /*tp_getset */
};

