#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <NetworkManager.h>
#include <glib.h>

#include "common.h"
#include "plugin.h"

// Optional: ABI for a dlopen()-loaded VPN core.
#include "examplevpn_core.h"

#define IP_TEMPLATE "%u.%u.%u.%u"
#define IP(x)                                                                  \
  ((uint8_t *)&(x))[3], ((uint8_t *)&(x))[2], ((uint8_t *)&(x))[1],            \
      ((uint8_t *)&(x))[0]

// ExampleVpnConfig is defined in examplevpn_core.h.

typedef struct {
  NMVpnServicePlugin *plugin;
  ExampleVpnConfig *cfg;
} IdleConfigData;

typedef struct {
  NMVpnServicePlugin *plugin;
  char *message;
} CaptureErrorData;

// We don't store any private state in this minimal example yet, but
// Meson/GObject patterns often use *_WITH_PRIVATE, so we keep an empty private
// struct.
typedef struct {
  int _unused;
} NMExampleVpnPluginPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(NMExampleVpnPlugin, nm_examplevpn_plugin,
                           NM_TYPE_VPN_SERVICE_PLUGIN)

static gboolean capture_error_idle(gpointer data) {
  CaptureErrorData *ce = (CaptureErrorData *)data;

  if (ce->message) {
    nm_vpn_service_plugin_failure(ce->plugin,
                                  NM_VPN_PLUGIN_FAILURE_CONNECT_FAILED);
    g_warning("ExampleVpnPlugin: runtime error: %s", ce->message);
    g_free(ce->message);
  }

  g_free(ce);
  return G_SOURCE_REMOVE;
}

// Call from any thread: schedules reporting to GLib main loop.
__attribute__((unused)) static void capture_error_async(void *plugin_ptr,
                                                        const char *error) {
  CaptureErrorData *ce = g_new0(CaptureErrorData, 1);
  ce->plugin = (NMVpnServicePlugin *)plugin_ptr;
  ce->message = error ? g_strdup(error) : NULL;
  g_idle_add(capture_error_idle, ce);
}

static void examplevpn_config_free(ExampleVpnConfig *cfg) {
  if (!cfg)
    return;
  g_free(cfg->tunnel_name);
  g_free(cfg);
}

static gboolean examplevpn_set_config_idle(gpointer user_data) {
  IdleConfigData *data = (IdleConfigData *)user_data;
  ExampleVpnConfig *cfg = data->cfg;

  GVariantBuilder gen;
  g_variant_builder_init(&gen, G_VARIANT_TYPE_VARDICT);

  if (cfg->tunnel_name && cfg->tunnel_name[0]) {
    g_variant_builder_add(&gen, "{sv}", NM_VPN_PLUGIN_CONFIG_TUNDEV,
                          g_variant_new_string(cfg->tunnel_name));
  }

  if (cfg->tunnel_mtu) {
    g_variant_builder_add(&gen, "{sv}", NM_VPN_PLUGIN_CONFIG_MTU,
                          g_variant_new_uint32(cfg->tunnel_mtu));
  }

  if (cfg->remote_address) {
    g_variant_builder_add(&gen, "{sv}", NM_VPN_PLUGIN_CONFIG_EXT_GATEWAY,
                          g_variant_new_uint32(g_htonl(cfg->remote_address)));
  }

  // This minimal example only reports IPv4.
  g_variant_builder_add(&gen, "{sv}", NM_VPN_PLUGIN_CONFIG_HAS_IP4,
                        g_variant_new_boolean(TRUE));
  nm_vpn_service_plugin_set_config(data->plugin, g_variant_builder_end(&gen));

  GVariantBuilder ip4;
  g_variant_builder_init(&ip4, G_VARIANT_TYPE_VARDICT);

  if (cfg->tunnel_gateway) {
    g_variant_builder_add(&ip4, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_INT_GATEWAY,
                          g_variant_new_uint32(g_htonl(cfg->tunnel_gateway)));
  }
  if (cfg->tunnel_address) {
    g_variant_builder_add(&ip4, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_ADDRESS,
                          g_variant_new_uint32(g_htonl(cfg->tunnel_address)));
  }
  if (cfg->tunnel_prefix) {
    g_variant_builder_add(&ip4, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_PREFIX,
                          g_variant_new_uint32(cfg->tunnel_prefix));
  }

  if (cfg->dns_address) {
    GVariantBuilder dns;
    g_variant_builder_init(&dns, G_VARIANT_TYPE("au"));
    g_variant_builder_add(&dns, "u", g_htonl(cfg->dns_address));
    g_variant_builder_add(&ip4, "{sv}", NM_VPN_PLUGIN_IP4_CONFIG_DNS,
                          g_variant_builder_end(&dns));
  }

  nm_vpn_service_plugin_set_ip4_config(data->plugin,
                                       g_variant_builder_end(&ip4));

  examplevpn_config_free(cfg);
  g_free(data);
  return G_SOURCE_REMOVE;
}

static gboolean real_connect(NMVpnServicePlugin *plugin,
                             NMConnection *connection, GError **error) {
  NMSettingVpn *s_vpn = nm_connection_get_setting_vpn(connection);
  if (!s_vpn) {
    g_set_error(error, NM_VPN_PLUGIN_ERROR,
                NM_VPN_PLUGIN_ERROR_INVALID_CONNECTION, "Missing VPN setting");
    return FALSE;
  }

  const char *protocol =
      nm_setting_vpn_get_data_item(s_vpn, NM_EXAMPLEVPN_KEY_PROTOCOL);
  const char *certificate =
      nm_setting_vpn_get_data_item(s_vpn, NM_EXAMPLEVPN_KEY_CERTIFICATE);

  if (!protocol) {
    g_set_error(error, NM_VPN_PLUGIN_ERROR, NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
                "Missing '%s'", NM_EXAMPLEVPN_KEY_PROTOCOL);
    return FALSE;
  }

  if (!certificate) {
    g_set_error(error, NM_VPN_PLUGIN_ERROR, NM_VPN_PLUGIN_ERROR_BAD_ARGUMENTS,
                "Missing '%s'", NM_EXAMPLEVPN_KEY_CERTIFICATE);
    return FALSE;
  }

  g_debug(
      "ExampleVpnPlugin: connect requested (protocol=%s, certificate_len=%zu)",
      protocol, strlen(certificate));

  // This guide repository intentionally does not ship an actual VPN core.
  // Instead, we demonstrate how to report config to NetworkManager.
  // If you integrate a real VPN implementation (Rust/C/etc), run it here and
  // call capture_error_async(plugin, "...") on failures.

  ExampleVpnConfig *cfg = g_new0(ExampleVpnConfig, 1);
  cfg->tunnel_name = g_strdup("examplevpn0");
  cfg->tunnel_mtu = 1400;

  // Demo values: 10.10.0.2/24 via 10.10.0.1, DNS 1.1.1.1
  cfg->remote_address = (10u << 24) | (10u << 16) | (0u << 8) | 1u;
  cfg->tunnel_gateway = (10u << 24) | (10u << 16) | (0u << 8) | 1u;
  cfg->tunnel_address = (10u << 24) | (10u << 16) | (0u << 8) | 2u;
  cfg->tunnel_prefix = 24;
  cfg->dns_address = (1u << 24) | (1u << 16) | (1u << 8) | 1u;

  g_debug("ExampleVpnPlugin: reporting config: addr=" IP_TEMPLATE
          "/%u gw=" IP_TEMPLATE,
          IP(cfg->tunnel_address), cfg->tunnel_prefix, IP(cfg->tunnel_gateway));

  IdleConfigData *data = g_new0(IdleConfigData, 1);
  data->plugin = plugin;
  data->cfg = cfg;
  g_idle_add(examplevpn_set_config_idle, data);

  return TRUE;
}

static gboolean real_disconnect(NMVpnServicePlugin *plugin, GError **error) {
  (void)plugin;
  (void)error;
  g_debug("ExampleVpnPlugin: disconnect requested");
  return TRUE;
}

static gboolean empty_need_secrets(NMVpnServicePlugin *plugin,
                                   NMConnection *connection,
                                   const char **setting_name, GError **error) {
  (void)plugin;
  (void)connection;
  (void)setting_name;
  (void)error;
  return FALSE; // no secrets required
}

static void nm_examplevpn_plugin_init(NMExampleVpnPlugin *plugin) {
  (void)plugin;
}

static void nm_examplevpn_plugin_class_init(NMExampleVpnPluginClass *klass) {
  NMVpnServicePluginClass *parent = NM_VPN_SERVICE_PLUGIN_CLASS(klass);
  parent->connect = real_connect;
  parent->need_secrets = empty_need_secrets;
  parent->disconnect = real_disconnect;
}

NMExampleVpnPlugin *nm_examplevpn_plugin_new(void) {
  return g_object_new(NM_TYPE_EXAMPLEVPN_PLUGIN, NULL);
}

int main(int argc, char **argv) {
  g_set_prgname("nm-examplevpn-service");

  GMainLoop *loop = g_main_loop_new(NULL, FALSE);
  // Create the plugin instance and set the D-Bus service name.
  // NetworkManager will D-Bus-activate this service and call methods on it.
  NMExampleVpnPlugin *plugin = g_object_new(
      NM_TYPE_EXAMPLEVPN_PLUGIN, NM_VPN_SERVICE_PLUGIN_DBUS_SERVICE_NAME,
      EXAMPLEVPN_PLUGIN_SERVICE, NULL);

  g_signal_connect(plugin, "quit", G_CALLBACK(g_main_loop_quit), loop);

  // Note: we intentionally keep this example minimal; the NMVpnServicePlugin
  // object handles the D-Bus wiring internally once constructed with the
  // service-name property.
  (void)argc;
  (void)argv;

  g_main_loop_run(loop);

  g_object_unref(plugin);
  g_main_loop_unref(loop);
  return 0;
}
