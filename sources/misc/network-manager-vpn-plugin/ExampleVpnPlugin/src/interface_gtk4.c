#include <gtk/gtk.h>

#include "common.h"
#include "interface.h"

// UI WIDGET:

static void examplevpn_editor_interface_init(NMVpnEditorInterface *iface_class);

G_DEFINE_TYPE_WITH_CODE(ExampleVpnEditor, examplevpn_editor, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(ExampleVpnEditor) G_IMPLEMENT_INTERFACE(
                            NM_TYPE_VPN_EDITOR,
                            examplevpn_editor_interface_init))

static void stuff_changed_cb(gpointer user_data) {
  g_signal_emit_by_name(EXAMPLEVPN_EDITOR(user_data), "changed");
}

typedef struct {
  GMainLoop *loop;
  GFile *file;
} DialogData;

static void on_file_dialog_done(GObject *source_object, GAsyncResult *res,
                                gpointer user_data) {
  DialogData *data = (DialogData *)user_data;
  GError *error = NULL;

  data->file =
      gtk_file_dialog_open_finish(GTK_FILE_DIALOG(source_object), res, &error);
  if (error) {
    g_debug("Choosing certificate: File dialog error: %s", error->message);
    g_error_free(error);
  }

  g_main_loop_quit(data->loop);
}

static void choose_certificate_cb(GtkWidget *button __attribute__((unused)),
                                  gpointer user_data) {
  ExampleVpnEditor *self = EXAMPLEVPN_EDITOR(user_data);
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);

  GtkFileDialog *dialog = gtk_file_dialog_new();
  gtk_file_dialog_set_title(dialog, "Select ExampleVpn Certificate");
  gtk_file_dialog_set_modal(dialog, TRUE);
  gtk_file_dialog_set_accept_label(dialog, "_Open");

  GListStore *filters = g_list_store_new(GTK_TYPE_FILE_FILTER);

  GtkFileFilter *filter_cert = gtk_file_filter_new();
  gtk_file_filter_set_name(filter_cert, "ExampleVpn Certificate Files (*.sea)");
  gtk_file_filter_add_pattern(filter_cert, "*.sea");
  g_list_store_append(filters, filter_cert);
  g_object_unref(filter_cert);

  GtkFileFilter *filter_all = gtk_file_filter_new();
  gtk_file_filter_set_name(filter_all, "All Files");
  gtk_file_filter_add_pattern(filter_all, "*");
  g_list_store_append(filters, filter_all);
  g_object_unref(filter_all);

  gtk_file_dialog_set_filters(dialog, G_LIST_MODEL(filters));
  g_object_unref(filters);

  DialogData data = {g_main_loop_new(NULL, FALSE), NULL};
  gtk_file_dialog_open(dialog, NULL, NULL, on_file_dialog_done, &data);
  g_main_loop_run(data.loop);

  g_main_loop_unref(data.loop);
  g_object_unref(dialog);

  if (!data.file) {
    g_debug("Choosing certificate: Error in ExampleVpn certificate choosing "
            "dialog!");
    gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                       "Certificate not changed: error!");
    return;
  }

  char *filename = g_file_get_path(data.file);
  g_debug("Choosing certificate: Updated ExampleVpn certificate to: %s",
          filename);
  g_free(filename);

  GBytes *contents = g_file_load_bytes(data.file, NULL, NULL, NULL);
  if (!contents) {
    g_debug("Choosing certificate: Reading ExampleVpn certificate file "
            "contents failed!");
    gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                       "Certificate not changed: reading error!");
    return;
  }

  g_object_unref(data.file);
  gsize length = 0;
  gpointer raw_contents = g_bytes_unref_to_data(contents, &length);

  gchar *encoded = g_base64_encode((const guchar *)raw_contents, length);
  if (!encoded) {
    g_debug("Choosing certificate: Error encoding contents of the ExampleVpn "
            "certificate file !");
    gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                       "Certificate not changed: embedding error!");
    g_free(raw_contents);
    return;
  }

  g_free(priv->certificate_filedata);
  priv->certificate_filedata = encoded;
  g_debug("Choosing certificate: New ExampleVpn certificate file embedded "
          "value (%lu bytes) is set to: %s",
          length, encoded);

  g_free(raw_contents);
  gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                     "Certificate file updated!");
  stuff_changed_cb(self);
}

static void change_protocol_cb(GtkWidget *button __attribute__((unused)),
                               gpointer user_data) {
  ExampleVpnEditor *self = EXAMPLEVPN_EDITOR(user_data);
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);

  gboolean typhoon_active =
      gtk_check_button_get_active(GTK_CHECK_BUTTON(priv->radio_typhoon));
  gboolean port_active =
      gtk_check_button_get_active(GTK_CHECK_BUTTON(priv->radio_port));

  if (typhoon_active && port_active) {
    g_debug("Choosing protocol: Both protocol radio buttons are active "
            "simultaneously!");
    return;
  }

  g_free(priv->protocol_name);
  if (typhoon_active)
    priv->protocol_name = g_strdup("typhoon");
  else if (port_active)
    priv->protocol_name = g_strdup("port");

  stuff_changed_cb(self);
}

static gboolean check_validity(ExampleVpnEditor *self) {
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);
  if (!priv->certificate_filedata)
    g_debug("Validating connection: Certificate file data is missing!");
  if (!priv->protocol_name)
    g_debug("Validating connection: Connection protocol name is missing!");
  return priv->certificate_filedata != NULL && priv->protocol_name != NULL;
}

static gboolean init_editor_plugin(ExampleVpnEditor *self,
                                   NMConnection *connection, GError **error) {
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);
  NMSettingVpn *s_vpn = nm_connection_get_setting_vpn(connection);

  GtkBuilder *builder = gtk_builder_new_from_resource(
      "/org/freedesktop/network-manager-examplevpn/dialog_gtk4.ui");
  if (!builder) {
    g_warning(
        "Initialising plugin: Error loading ExampleVpn editor interface UI!");
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Error loading ExampleVpn editor interface UI!");
    return FALSE;
  }

  priv->widget = GTK_WIDGET(gtk_builder_get_object(builder, "root_box"));
  if (!priv->widget) {
    g_warning(
        "Initialising plugin: Error building ExampleVpn editor interface UI!");
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Error building ExampleVpn editor interface UI!");
    g_object_unref(builder);
    return FALSE;
  }
  g_object_ref_sink(priv->widget);

  priv->filechooser_widget =
      GTK_WIDGET(gtk_builder_get_object(builder, "filechooser_certificate"));
  priv->label_selected_certificate =
      GTK_WIDGET(gtk_builder_get_object(builder, "label_selected_certificate"));
  priv->radio_typhoon =
      GTK_WIDGET(gtk_builder_get_object(builder, "radio_typhoon"));
  priv->radio_port = GTK_WIDGET(gtk_builder_get_object(builder, "radio_port"));

  if (!priv->filechooser_widget || !priv->label_selected_certificate ||
      !priv->radio_typhoon || !priv->radio_port) {
    g_warning("Initialising plugin: Error checking properties of ExampleVpn "
              "editor interface UI!");
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Error checking properties of ExampleVpn editor interface UI!");
    g_object_unref(priv->widget);
    g_object_unref(builder);
    return FALSE;
  }

  if (s_vpn) {
    const char *cert_value =
        nm_setting_vpn_get_data_item(s_vpn, NM_EXAMPLEVPN_KEY_CERTIFICATE);
    priv->certificate_filedata = g_strdup(cert_value);
    if (cert_value)
      gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                         "Certificate file embedded!");
    else
      gtk_label_set_text(GTK_LABEL(priv->label_selected_certificate),
                         "Certificate file not selected!");

    const char *proto_value =
        nm_setting_vpn_get_data_item(s_vpn, NM_EXAMPLEVPN_KEY_PROTOCOL);
    if (proto_value) {
      if (g_strcmp0(proto_value, "typhoon") == 0)
        gtk_check_button_set_active(GTK_CHECK_BUTTON(priv->radio_typhoon),
                                    TRUE);
      else if (g_strcmp0(proto_value, "port") == 0)
        gtk_check_button_set_active(GTK_CHECK_BUTTON(priv->radio_port), TRUE);
      else {
        g_warning("Initialising plugin: Error checking 'protocol' value of "
                  "ExampleVpn connection settings: %s!",
                  proto_value);
        g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                    "Error checking 'protocol' value of ExampleVpn connection "
                    "settings!");
        g_object_unref(priv->widget);
        g_object_unref(builder);
        return FALSE;
      }
    } else {
      gtk_check_button_set_active(GTK_CHECK_BUTTON(priv->radio_typhoon), TRUE);
      proto_value = NM_EXAMPLEVPN_PROTOCOL_DEFAULT;
    }
    priv->protocol_name = g_strdup(proto_value);
  }

  g_signal_connect(G_OBJECT(priv->filechooser_widget), "clicked",
                   G_CALLBACK(choose_certificate_cb), self);
  g_signal_connect(G_OBJECT(priv->radio_typhoon), "toggled",
                   G_CALLBACK(change_protocol_cb), self);
  g_signal_connect(G_OBJECT(priv->radio_port), "toggled",
                   G_CALLBACK(change_protocol_cb), self);

  g_object_unref(builder);
  return TRUE;
}

static GObject *get_widget(NMVpnEditor *iface) {
  ExampleVpnEditor *self = EXAMPLEVPN_EDITOR(iface);
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);
  return G_OBJECT(priv->widget);
}

static gboolean update_connection(NMVpnEditor *iface, NMConnection *connection,
                                  GError **error) {
  ExampleVpnEditor *self = EXAMPLEVPN_EDITOR(iface);
  ExampleVpnEditorPrivate *priv = examplevpn_editor_get_instance_private(self);

  if (!check_validity(self)) {
    g_debug("Updating connection: Aborting because validation failed!");
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Aborting because validation failed!");
    return FALSE;
  }

  NMSettingVpn *s_vpn = NM_SETTING_VPN(nm_setting_vpn_new());
  g_object_set(s_vpn, NM_SETTING_VPN_SERVICE_TYPE, EXAMPLEVPN_PLUGIN_SERVICE,
               NULL);

  nm_setting_vpn_add_data_item(s_vpn, NM_EXAMPLEVPN_KEY_CERTIFICATE,
                               priv->certificate_filedata);
  nm_setting_vpn_add_data_item(s_vpn, NM_EXAMPLEVPN_KEY_PROTOCOL,
                               priv->protocol_name);

  nm_connection_add_setting(connection, NM_SETTING(s_vpn));
  return TRUE;
}

static void dispose(GObject *object) {
  ExampleVpnEditor *self = EXAMPLEVPN_EDITOR(object);
  ExampleVpnEditorPrivate *priv =
      examplevpn_editor_get_instance_private(EXAMPLEVPN_EDITOR(self));

  g_free(priv->certificate_filedata);
  g_free(priv->protocol_name);

  if (priv->group)
    g_object_unref(priv->group);
  if (priv->widget)
    g_object_unref(priv->widget);

  G_OBJECT_CLASS(examplevpn_editor_parent_class)->dispose(object);
}

static void examplevpn_editor_class_init(ExampleVpnEditorClass *req_class) {
  GObjectClass *object_class = G_OBJECT_CLASS(req_class);
  object_class->dispose = dispose;
}

static void examplevpn_editor_init(ExampleVpnEditor *plugin
                                   __attribute__((unused))) {}

static void
examplevpn_editor_interface_init(NMVpnEditorInterface *iface_class) {
  iface_class->get_widget = get_widget;
  iface_class->update_connection = update_connection;
}

G_MODULE_EXPORT NMVpnEditor *create_examplevpn_editor(NMConnection *connection,
                                                      GError **error) {
  if (error)
    g_return_val_if_fail(*error == NULL, NULL);

  NMVpnEditor *object = g_object_new(EXAMPLEVPN_TYPE_EDITOR, NULL);
  if (!object) {
    g_warning(
        "Constructing interface: Error creating ExampleVpn editor interface!");
    g_set_error(error, EXAMPLEVPN_EDITOR_PLUGIN_ERROR, 0,
                "Error creating ExampleVpn editor interface");
    return NULL;
  }

  if (!init_editor_plugin(EXAMPLEVPN_EDITOR(object), connection, error)) {
    g_warning("Constructing interface: Error initializing ExampleVpn editor "
              "interface UI!");
    g_object_unref(object);
    return NULL;
  }
  return object;
}
