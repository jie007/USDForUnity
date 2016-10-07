#include "pch.h"
#include "Mono.h"
#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif


// mono functions
void *g_mono_dll;

MonoDomain*     (*mono_domain_get)(void);
MonoAssembly*   (*mono_domain_assembly_open)(MonoDomain *domain, const char *assemblyName);
MonoImage*      (*mono_assembly_get_image)(MonoAssembly *assembly);

MonoThread*     (*mono_thread_current)(void);
MonoThread*     (*mono_thread_attach)(MonoDomain *domain);
void            (*mono_thread_detach)(MonoThread *thread);
void            (*mono_thread_suspend_all_other_threads)();
void            (*mono_thread_abort_all_other_threads)();
void            (*mono_jit_thread_attach)(MonoDomain *domain);

void            (*mono_add_internal_call)(const char *name, void *method);
MonoObject*     (*mono_runtime_invoke)(MonoMethod *method, MonoObject *obj, void **params, void **exc);

MonoClass*      (*mono_class_from_name)(MonoImage *image, const char *namespaceString, const char *classnameString);
MonoType*       (*mono_class_get_type)(MonoClass *klass);
MonoMethod*     (*mono_class_get_method_from_name)(MonoClass *klass, const char *name, int param_count);
MonoClassField* (*mono_class_get_field_from_name)(MonoClass *klass, const char *name);
MonoMethod*     (*mono_class_inflate_generic_method)(MonoMethod *method, MonoGenericContext *context);

guint32         (*mono_field_get_offset)(MonoClassField *field);

MonoObject*     (*mono_object_new)(MonoDomain *domain, MonoClass *klass);
MonoClass*      (*mono_object_get_class)(MonoObject *obj);
gpointer        (*mono_object_unbox)(MonoObject *obj);

MonoArray*      (*mono_array_new)(MonoDomain *domain, MonoClass *eclass, mono_array_size_t n);
char*           (*mono_array_addr_with_size)(MonoArray *array, int size, uintptr_t idx);

MonoString*     (*mono_string_new)(MonoDomain *domain, const char *text);
MonoString*     (*mono_string_new_len)(MonoDomain *domain, const char *text, guint length);
char*           (*mono_string_to_utf8)(MonoString *string_obj);
gunichar2*      (*mono_string_to_utf16)(MonoString *string_obj);

guint32         (*mono_gchandle_new)(MonoObject *obj, gboolean pinned);
void            (*mono_gchandle_free)(guint32 gchandle);

MonoClass* (*mono_get_object_class)();
MonoClass* (*mono_get_byte_class)();
MonoClass* (*mono_get_void_class)();
MonoClass* (*mono_get_boolean_class)();
MonoClass* (*mono_get_sbyte_class)();
MonoClass* (*mono_get_int16_class)();
MonoClass* (*mono_get_uint16_class)();
MonoClass* (*mono_get_int32_class)();
MonoClass* (*mono_get_uint32_class)();
MonoClass* (*mono_get_intptr_class)();
MonoClass* (*mono_get_uintptr_class)();
MonoClass* (*mono_get_int64_class)();
MonoClass* (*mono_get_uint64_class)();
MonoClass* (*mono_get_single_class)();
MonoClass* (*mono_get_double_class)();
MonoClass* (*mono_get_char_class)();
MonoClass* (*mono_get_string_class)();
MonoClass* (*mono_get_enum_class)();
MonoClass* (*mono_get_array_class)();
MonoClass* (*mono_get_thread_class)();
MonoClass* (*mono_get_exception_class)();


void ImportMonoFunctions()
{
#ifdef _WIN32
    auto mono = ::GetModuleHandleA("mono.dll");
    g_mono_dll = mono;
    if (mono) {
#define Import(Name) (void*&)Name = ::GetProcAddress(mono, #Name)

        Import(mono_domain_get);
        Import(mono_domain_assembly_open);
        Import(mono_assembly_get_image);

        Import(mono_thread_current);
        Import(mono_thread_attach);
        Import(mono_thread_detach);
        Import(mono_thread_suspend_all_other_threads);
        Import(mono_thread_abort_all_other_threads);
        Import(mono_jit_thread_attach);

        Import(mono_add_internal_call);
        Import(mono_runtime_invoke);

        Import(mono_class_from_name);
        Import(mono_class_get_type);
        Import(mono_class_get_method_from_name);
        Import(mono_class_get_field_from_name);
        Import(mono_class_inflate_generic_method);

        Import(mono_field_get_offset);

        Import(mono_object_new);
        Import(mono_object_get_class);
        Import(mono_object_unbox);

        Import(mono_array_new);
        Import(mono_array_addr_with_size);

        Import(mono_string_new);
        Import(mono_string_new_len);
        Import(mono_string_to_utf8);
        Import(mono_string_to_utf16);

        Import(mono_gchandle_new);
        Import(mono_gchandle_free);

        Import(mono_get_object_class);
        Import(mono_get_byte_class);
        Import(mono_get_void_class);
        Import(mono_get_boolean_class);
        Import(mono_get_sbyte_class);
        Import(mono_get_int16_class);
        Import(mono_get_uint16_class);
        Import(mono_get_int32_class);
        Import(mono_get_uint32_class);
        Import(mono_get_intptr_class);
        Import(mono_get_uintptr_class);
        Import(mono_get_int64_class);
        Import(mono_get_uint64_class);
        Import(mono_get_single_class);
        Import(mono_get_double_class);
        Import(mono_get_char_class);
        Import(mono_get_string_class);
        Import(mono_get_enum_class);
        Import(mono_get_array_class);
        Import(mono_get_thread_class);
        Import(mono_get_exception_class);
#undef Import
    }
#else
    // todo
#endif
}

static struct _ImportMonoFunctions {
    _ImportMonoFunctions() { ImportMonoFunctions(); }
} g_ImportMonoFunctions;



MObject::MObject() {}
MObject::~MObject() { free(); }

void MObject::allocate(MonoClass *mc)
{
    m_rep = mono_object_new(mono_domain_get(), mc);
    m_gch = mono_gchandle_new(m_rep, 1);
}

void MObject::free()
{
    if (m_rep) {
        mono_gchandle_free(m_gch);
        m_rep = nullptr;
        m_gch = 0;
    }
}

MonoObject* MObject::get() { return m_rep; }
MObject::operator bool() const { return m_rep != nullptr; }


MArray::MArray()
{
}

MArray::~MArray()
{
    free();
}

void MArray::allocate(MonoClass *mc, size_t size)
{
    if (m_size == size) {
        // no need to re-allocate
        return;
    }

    free();

    if (size == 0) { return; }
    m_rep = mono_array_new(mono_domain_get(), mc, (mono_array_size_t)size);
    m_gch = mono_gchandle_new((MonoObject*)m_rep, 1);
    m_size = size;
}

void MArray::free()
{
    if (m_rep) {
        mono_gchandle_free(m_gch);
        m_rep = nullptr;
        m_gch = 0;
        m_size = 0;
    }
}

size_t MArray::size() const
{
    return m_size;
}

void* MArray::data()
{
    return m_rep ? mono_array_addr_with_size(m_rep, 0, 0) : nullptr;
}

const void* MArray::data() const
{
    return const_cast<MArray*>(this)->data();
}

MonoArray* MArray::get() { return m_rep; }
MArray::operator bool() const { return m_rep != nullptr; }
