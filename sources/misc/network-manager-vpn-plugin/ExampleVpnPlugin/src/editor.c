#include <dlfcn.h>

#include "common.h"
#include "editor.h"

// PLUGIN:

enum { PROP_0, PROP_NAME, PROP_DESC, PROP_SERVICE };

static void examplevpn_editor_plugin_interface_init(
    NMVpnEditorPluginInterface *iface_class);

G_DEFINE_TYPE_EXTENDED(
    ExampleVpnEditorPlugin, examplevpn_editor_plugin, G_TYPE_OBJECT, 0,
    G_IMPLEMENT_INTERFACE(NM_TYPE_VPN_EDITOR_PLUGIN,
                          examplevpn_editor_plugin_interface_init))

typedef enum {
  NM_EXAMPLEVPN_IMPORT_EXPORT_ERROR_UNKNOWN = 0,
  NM_EXAMPLEVPN_IMPORT_EXPORT_ERROR_NOT_READABLE
} NMExampleVpnImportError;

// CODE:

#define NM_EXAMPLEVPN_IMPORT_EXPORT_ERROR                                      \
  nm_examplevpn_import_export_error_quark()

__attribute__((unused)) static GQuark
nm_examplevpn_import_export_error_quark(void) {
  static GQuark quark = 0;
  if (G_UNLIKELY(quark == 0))
    quark =
        g_quark_from_static_string("nm-examplevpn-import-export-error-quark");
  return quark;
}

static guint32 get_capabilities(NMVpnEditorPlugin *iface
                                __attribute__((unused))) {
  g_message("Checking ExampleVpn capabilities...");
  return NM_VPN_EDITOR_PLUGIN_CAPABILITY_IMPORT |
         NM_VPN_EDITOR_PLUGIN_CAPABILITY_EXPORT;
}

static NMVpnEditor *get_editor(NMVpnEditorPlugin *iface __attribute__((unused)),
                               NMConnection *connection, GError **error) {
  g_message("Getting ExampleVpn editor...");

  unsigned int gtk_major = 3;
  void *handle = dlopen(NULL, RTLD_NOW);
  if (handle) {
    dll_function get_major_version_holder = {
        dlsym(handle, "gtk_get_major_version")};
    if (get_major_version_holder.pointer) {
      gtk_major = get_major_version_holder.get_major_version();
    }
    dlclose(handle);
  } else {
    g_warning("Failed to open process handle for symbol lookup: %s", dlerror());
  }

  const char *libname;
  if (gtk_major >= 4) {
    libname = EDITOR_INTERFACE_PATH "-gtk4.so";
  } else {
    libname = EDITOR_INTERFACE_PATH "-gtk3.so";
  }

  void *editor_handle = dlopen(libname, RTLD_LAZY);
  if (!editor_handle) {
    g_warning("Failed to open interface library: %s: %s", libname, dlerror());
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Failed to load editor library %s: %s", libname, dlerror());
    return NULL;
  }

  dll_function create_examplevpn_editor_holder = {
      dlsym(editor_handle, "create_examplevpn_editor")};
  if (!create_examplevpn_editor_holder.pointer) {
    g_warning("Failed to resolve create_examplevpn_editor symbol: %s",
              dlerror());
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Failed to resolve create_examplevpn_editor symbol: %s",
                dlerror());
    dlclose(editor_handle);
    return NULL;
  }

  return create_examplevpn_editor_holder.create_examplevpn_editor(connection,
                                                                  error);
}

static NMConnection *import(NMVpnEditorPlugin *iface __attribute__((unused)),
                            const char *path, GError **error) {
  g_message("Importing ExampleVpn connection...");
  NMConnection *connection = nm_simple_connection_new();

  NMSettingConnection *s_con =
      NM_SETTING_CONNECTION(nm_setting_connection_new());
  nm_connection_add_setting(connection, NM_SETTING(s_con));

  NMSettingVpn *s_vpn = NM_SETTING_VPN(nm_setting_vpn_new());
  g_object_set(s_vpn, NM_SETTING_VPN_SERVICE_TYPE, EXAMPLEVPN_PLUGIN_SERVICE,
               NULL);
  nm_connection_add_setting(connection, NM_SETTING(s_vpn));

  NMSettingIP4Config *s_ip4 =
      NM_SETTING_IP4_CONFIG(nm_setting_ip4_config_new());
  nm_connection_add_setting(connection, NM_SETTING(s_ip4));

  gsize length = 0;
  gchar *contents = NULL;
  if (!g_file_get_contents(path, &contents, &length, error)) {
    g_warning("Failed to open import file: %s", path);
    g_set_error(error, NM_EXAMPLEVPN_IMPORT_EXPORT_ERROR,
                NM_EXAMPLEVPN_IMPORT_EXPORT_ERROR_NOT_READABLE,
                "Failed to open import file: %s", path);
    return NULL;
  }

  gchar *encoded = g_base64_encode((const guchar *)contents, length);
  nm_setting_vpn_add_data_item(s_vpn, NM_EXAMPLEVPN_KEY_CERTIFICATE, encoded);
  g_free(encoded);
  g_free(contents);

  nm_setting_vpn_add_data_item(s_vpn, NM_EXAMPLEVPN_KEY_PROTOCOL,
                               NM_EXAMPLEVPN_PROTOCOL_DEFAULT);
  return connection;
}

static gboolean export(NMVpnEditorPlugin *iface __attribute__((unused)),
                       const char *path, NMConnection *connection,
                       GError **error) {
  g_message("Exporting ExampleVpn connection...");
  NMSettingVpn *s_vpn = nm_connection_get_setting_vpn(connection);

  const char *cert_value =
      nm_setting_vpn_get_data_item(s_vpn, NM_EXAMPLEVPN_KEY_CERTIFICATE);
  if (!cert_value)
    return FALSE;

  gsize length = 0;
  guchar *decoded = g_base64_decode(cert_value, &length);
  if (!decoded || length == 0)
    return FALSE;

  if (!g_file_set_contents(path, (gchar *)decoded, (gssize)length, error))
    return FALSE;

  g_free(decoded);
  return TRUE;
}

static char *get_suggested_filename(NMVpnEditorPlugin *iface
                                    __attribute__((unused)),
                                    NMConnection *connection) {
  g_message("Suggesting ExampleVpn connection file name...");
  g_return_val_if_fail(connection != NULL, NULL);

  NMSettingConnection *s_con = nm_connection_get_setting_connection(connection);
  g_return_val_if_fail(s_con != NULL, NULL);

  const char *connection_id = nm_setting_connection_get_id(s_con);
  g_return_val_if_fail(connection_id != NULL, NULL);

  return g_strdup_printf("%s.sea", connection_id);
}

static void get_property(GObject *object, guint prop_id, GValue *value,
                         GParamSpec *pspec) {
  g_message("Getting ExampleVpn connection property %u...", prop_id);

  switch (prop_id) {
  case PROP_NAME:
    g_value_set_string(value, EXAMPLEVPN_PLUGIN_NAME);
    break;
  case PROP_DESC:
    g_value_set_string(value, EXAMPLEVPN_PLUGIN_DESC);
    break;
  case PROP_SERVICE:
    g_value_set_string(value, EXAMPLEVPN_PLUGIN_SERVICE);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void
examplevpn_editor_plugin_class_init(ExampleVpnEditorPluginClass *req_class) {
  g_message("Constructing ExampleVpn plugin class interface...");

  GObjectClass *object_class = G_OBJECT_CLASS(req_class);
  object_class->get_property = get_property;

  g_object_class_override_property(object_class, PROP_NAME,
                                   NM_VPN_EDITOR_PLUGIN_NAME);
  g_object_class_override_property(object_class, PROP_DESC,
                                   NM_VPN_EDITOR_PLUGIN_DESCRIPTION);
  g_object_class_override_property(object_class, PROP_SERVICE,
                                   NM_VPN_EDITOR_PLUGIN_SERVICE);
}

static void examplevpn_editor_plugin_init(ExampleVpnEditorPlugin *plugin
                                          __attribute__((unused))) {}

static void examplevpn_editor_plugin_interface_init(
    NMVpnEditorPluginInterface *iface_class) {
  g_message("Constructing ExampleVpn plugin interface interface...");

  iface_class->get_editor = get_editor;
  iface_class->get_capabilities = get_capabilities;
  iface_class->import_from_file = import;
  iface_class->export_to_file = export;
  iface_class->get_suggested_filename = get_suggested_filename;
}

G_MODULE_EXPORT NMVpnEditor *
nm_vpn_editor_factory_examplevpn(NMVpnEditorPlugin *editor_plugin,
                                 NMConnection *connection, GError **error) {
  g_message("ExampleVpn editor factory called...");
  if (error)
    g_return_val_if_fail(*error == NULL, NULL);
  return get_editor(NM_VPN_EDITOR_PLUGIN(editor_plugin), connection, error);
}

G_MODULE_EXPORT NMVpnEditorPlugin *
nm_vpn_editor_plugin_factory(GError **error) {
  g_message("ExampleVpn editor plugin factory called...");
  if (error)
    g_return_val_if_fail(*error == NULL, NULL);
  return g_object_new(EXAMPLEVPN_TYPE_EDITOR_PLUGIN, NULL);
}
