#ifndef NM_EXAMPLEVPN_EDITOR_H
#define NM_EXAMPLEVPN_EDITOR_H

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

#include <NetworkManager.h>

#include <glib-object.h>

#define EXAMPLEVPN_EDITOR_PLUGIN_ERROR NM_CONNECTION_ERROR
#define EXAMPLEVPN_EDITOR_PLUGIN_ERROR_INVALID_PROPERTY                        \
  NM_CONNECTION_ERROR_INVALID_PROPERTY

#define EXAMPLEVPN_TYPE_EDITOR_PLUGIN (examplevpn_editor_plugin_get_type())
#define EXAMPLEVPN_EDITOR_PLUGIN(obj)                                          \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLEVPN_TYPE_EDITOR_PLUGIN,            \
                              ExampleVpnEditorPlugin))
#define EXAMPLEVPN_EDITOR_PLUGIN_CLASS(klass)                                  \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLEVPN_TYPE_EDITOR_PLUGIN,             \
                           ExampleVpnEditorPluginClass))
#define EXAMPLEVPN_IS_EDITOR_PLUGIN(obj)                                       \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLEVPN_TYPE_EDITOR_PLUGIN))
#define EXAMPLEVPN_IS_EDITOR_PLUGIN_CLASS(klass)                               \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLEVPN_TYPE_EDITOR_PLUGIN))
#define EXAMPLEVPN_EDITOR_PLUGIN_GET_CLASS(obj)                                \
  (G_TYPE_INSTANCE_GET_CLASS((obj), EXAMPLEVPN_TYPE_EDITOR_PLUGIN,             \
                             ExampleVpnEditorPluginClass))

typedef struct ExampleVpnEditorPluginActual ExampleVpnEditorPlugin;
typedef struct ExampleVpnEditorPluginClassActual ExampleVpnEditorPluginClass;

struct ExampleVpnEditorPluginActual {
  GObject parent;
};

struct ExampleVpnEditorPluginClassActual {
  GObjectClass parent;
};

GType examplevpn_editor_plugin_get_type(void);

#define EXAMPLEVPN_TYPE_EDITOR (examplevpn_editor_get_type())
#define EXAMPLEVPN_EDITOR(obj)                                                 \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), EXAMPLEVPN_TYPE_EDITOR, ExampleVpnEditor))
#define EXAMPLEVPN_EDITOR_CLASS(klass)                                         \
  (G_TYPE_CHECK_CLASS_CAST((klass), EXAMPLEVPN_TYPE_EDITOR,                    \
                           ExampleVpnEditorClass))
#define EXAMPLEVPN_IS_EDITOR(obj)                                              \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), EXAMPLEVPN_TYPE_EDITOR))
#define EXAMPLEVPN_IS_EDITOR_CLASS(klass)                                      \
  (G_TYPE_CHECK_CLASS_TYPE((klass), EXAMPLEVPN_TYPE_EDITOR))
#define EXAMPLEVPN_EDITOR_GET_CLASS(obj)                                       \
  (G_TYPE_INSTANCE_GET_CLASS((obj), EXAMPLEVPN_TYPE_EDITOR,                    \
                             ExampleVpnEditorClass))

typedef struct ExampleVpnEditorActual ExampleVpnEditor;
typedef struct ExampleVpnEditorClassActual ExampleVpnEditorClass;

struct ExampleVpnEditorActual {
  GObject parent;
};

struct ExampleVpnEditorClassActual {
  GObjectClass parent;
};

GType examplevpn_editor_get_type(void);

#define EXAMPLEVPN_PLUGIN_NAME "ExampleVpnPlugin"
#define EXAMPLEVPN_PLUGIN_DESC                                                 \
  "Example VPN plugin for NetworkManager (guide sample)"

#define NM_EXAMPLEVPN_PROTOCOL_DEFAULT "typhoon"

NMVpnEditor *
nm_vpn_editor_factory_examplevpn(NMVpnEditorPlugin * /* editor_plugin */,
                                 NMConnection * /* connection */,
                                 GError ** /* error */);

#endif /* NM_EXAMPLEVPN_EDITOR_H */
