#ifndef NM_EXAMPLEVPN_PLUGIN_H
#define NM_EXAMPLEVPN_PLUGIN_H

#include <glib.h>
#include <nm-vpn-service-plugin.h>

#define NM_DBUS_SERVICE_EXAMPLEVPN EXAMPLEVPN_PLUGIN_SERVICE
#define NM_DBUS_INTERFACE_EXAMPLEVPN EXAMPLEVPN_PLUGIN_SERVICE
#define NM_DBUS_PATH_EXAMPLEVPN EXAMPLEVPN_PLUGIN_PATH

#define NM_TYPE_EXAMPLEVPN_PLUGIN (nm_examplevpn_plugin_get_type())
#define NM_EXAMPLEVPN_PLUGIN(obj)                                              \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), NM_TYPE_EXAMPLEVPN_PLUGIN,                \
                              NMExampleVpnPlugin))

typedef struct {
  NMVpnServicePlugin parent;
} NMExampleVpnPlugin;

typedef struct {
  NMVpnServicePluginClass parent;
} NMExampleVpnPluginClass;

GType nm_examplevpn_plugin_get_type(void);
NMExampleVpnPlugin *nm_examplevpn_plugin_new(void);

#endif /* NM_EXAMPLEVPN_PLUGIN_H */
