# A custom NetworkManager VPN plugin 101

Have you decided to write your own VPN, just like me?
Or would you just like to integrate your favorite solution with the convenient user interface of Linux network manager?
Anyway, you came to the right place!

[NetworkManager](https://networkmanager.dev/) is a great open source tool, that is already pre-installed on many Linux desktop distributions ([Ubuntu](https://ubuntu.com/), [Fedora](https://fedoraproject.org/), [Arch](https://archlinux.org/) and [others](https://wiki.archlinux.org/title/NetworkManager)).
It manages system network changes and re-configuration on the fly, and that's really helpful especially for devices such as laptops, that might need switching between different networks (such as Wi-Fi's) quickly and automatically.
It also has an interface for VPN plugins, they are started automatically on demand in the background, and the network manager takes care of all the capabilities and routes configuration.
The plugin settings page can appear in the network manager GUI app ([nm-connection-editor](https://manpages.ubuntu.com/manpages/noble/en/man1/nm-connection-editor.1.html)) and sometimes even in the system settings app.

Still, there's one big drawback: since network manager developers apparently invest most of their time into development, none of these interfaces are documented properly and no examples are provided (apart from [this project](https://github.com/manuels/nm-vpn-plugin) that seems to be incomplete and somewhat abandoned to me).
The only source of information about the plugin interface is the network manager [VPN support page](https://networkmanager.dev/docs/vpn/) where existing plugin implementations are listed, but going through their code is a tedious and time-consuming task, especially because the code is not usually commented.
Good news: I made it so you don't, and I'm going to share what I found.

## Credits

This guide is based on reading and comparing the source code of multiple existing NetworkManager VPN plugin implementations, including [NetworkManager-openvpn](https://gitlab.gnome.org/GNOME/NetworkManager-openvpn), [NetworkManager-openconnect](https://gitlab.gnome.org/GNOME/NetworkManager-openconnect) and [NetworkManager-fortisslvpn](https://gitlab.gnome.org/GNOME/NetworkManager-fortisslvpn).
None of this would have been possible without their open source code.
I am also grateful to the NetworkManager developers for their [API reference documentation](https://networkmanager.dev/docs/api/latest/) and [libnm reference manual](https://networkmanager.dev/docs/libnm/latest/).

## Goals and non-goals

Here, I will attempt to implement the most simple NetworkManager plugin, compliant with all the tools that make use of the NetworkManager.
It should provide smooth user experience in both GUI and CLI.

However, not all the features provided by NetworkManager will be used.
Plugin configuration, integration and error handling will be as simple as possible.
Internationalization feature (GUI translation) that is present for many other plugins, will not be implemented (I don't know enough languages).
Also, **secret** feature of the NetworkManager will not be used, all the configs will be kept in plaintext (because it's easier).

I decided to implement my VPN in a form of a cross-platform Rust DLL (a shared library loaded with [`dlopen()`](https://man7.org/linux/man-pages/man3/dlopen.3.html)), that accepts all the configurations together in a form of a single certificate file.
I would strongly recommend this approach in designing your VPN plugins if possible, because managing all the connection properties in platform-specific apps will very soon become a nightmare of validation and error checking, especially if apart from Linux, your VPN should also be available on Windows or any other platforms.
Many NetworkManager plugins prefer launching a standalone VPN executable instead and observing its output, which feels much overcomplicated.

That's why my plugin is truly minimal and contains nothing more than basically receiving VPN configuration file and passing it forward.

## Disclaimer

Disclaimer!
I am in no way affiliated with the network manager developers, please refer to their [official website](https://networkmanager.dev/) for any further comments.
I am either not affiliated with any of the existing plugin developers (except for my own plugin), I could only read and compare their code.
Finally, this is my first time using most of the technologies mentioned below, including [D-Bus](https://www.freedesktop.org/wiki/Software/dbus/), [GLib](https://docs.gtk.org/glib/) and [GTK](https://www.gtk.org/), so I am not going to dive deep into details.
I will only try to explain plugin development from the user perspective.
Don't hesitate to comment on any errors and/or mistakes!

So with all the remarks made, let's start!

## Architecture

A VPN plugin can include multiple binaries, configuration files, etc. and below I will try to mention as many of them as possible.
Still, the main core idea is the following: a plugin consists of a frontend and a backend.

The frontend consists of a DLL, that implements the [`NMVpnEditorPlugin`](https://networkmanager.dev/docs/libnm/latest/NMVpnEditorPlugin.html) interface, describing your plugin, and also a special configuration file.
Some of its parts might be loaded by the NetworkManager background task on startup (and that's why you should restart network manager every time you make changes to it).
It can also optionally provide a plugin settings GUI that should allow user to enter all the information required for connecting to VPN using your plugin (I will cover it later in [Editor GUI](#editor-gui)).
It will be described in [Frontend](#frontend).

The backend is an executable that implements the `org.freedesktop.NetworkManager.VPN.Plugin` [D-Bus interface](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html), providing your VPN functionality.
There's no need to implement complex D-Bus integration yourself: a convenient [`libnm`](https://networkmanager.dev/docs/libnm/latest/) C library does most of it for you.
The backend is run either as a service by D-Bus or it can be run by you manually (for debugging purposes), because restarting D-Bus can lead to unexpected consequences on desktop Linux systems.
It will be described in [Backend](#backend).

Whenever any app that uses `libnm` performs some simple actions with it (like network interface listing, IP address search, VPN plugin discovery, etc.), it uses NetworkManager from user space as a library, since those actions are not intrusive and there's no harm if they are performed by multiple apps in parallel.
In these cases, `libnm` generally uses the code in the frontend DLL from user-space.
However when it comes to some intrusive system-wide tasks and settings, these have to be performed with care.
NetworkManager keeps track of the state of its plugins and protects them from race conditions.
That's why, instead of user-space management, whenever a breaking change should be applied to the system, it is passed to a background elevated task (backend), that listens to commands on D-Bus interface (think of D-Bus as of a message-passing interface, just like a pipe or a socket).
It sounds complex, but in reality it's rather logical even.

Keeping that in mind, let's switch to something more practical!

## Frontend

There are some mysteries in network manager architecture, that will probably never be explained to humanity.
These include network manager internal configuration files location and structure, why certain `nmcli` commands exit successfully but don't do anything, how some of the error messages should be interpreted and many more.
Some of them we will encounter later, but now here's the first one: a network manager VPN plugin is not entirely described by a frontend DLL, but also partially by a special `.name` file.
Why? I wish I knew.

### Name file

Let's start with the `.name` file, because it's just a plain-text INI-formatted file, no code involved.
To make NetworkManager discover your plugin, a special file named `nm-[PLUGIN_NAME]-service.name` should be placed to `/usr/lib/NetworkManager/VPN/` directory.

As it happens usually with network manager, the full allowed structure of the file is never documented, but here's what can be uncovered by comparing `.name` files of different open source plugin implementations:

```ini
[VPN Connection]  # Describes the plugin itself, its features and expected behavior
name=...  # The plugin name, preferably should be consistent across code
service=...  # The name of the D-Bus service, normally be equal to plugin name prefixed with 'org.freedesktop.NetworkManager.'
program=...  # Path to the backend executable, available by D-Bus at runtime
supports-multiple-connections=...  # Boolean value, specifying whether multiple simultaneous connections are possible, optional
...  # Maybe some other properties are possible as well, but I have never seen them in the wild

[libnm]  # Describes plugin properties relevant for frontend
plugin=...  # Path to the properties DLL

[GNOME]  # Contains definitions that will be used by other GNOME apps, including GUI implementations, etc., the whole section is optional
auth-dialog=...  # Path to authentication dialog DLL, will be covered later, optional
properties=...  # Path to a properties file, apparently it's meant to describe the properties DLL internal structure, but never have I ever seen it used :(
supports-external-ui-mode=...  # Boolean value, specifying whether the auth dialog should show textual dialog representation, optional
hints=...  # Boolean value, specifying whether the secret hints will be passed to the auth dialog, optional
```

Here are a few name file implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/blob/main/nm-openvpn-service.name.in)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/blob/main/nm-openconnect-service.name.in)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/blob/main/nm-fortisslvpn-service.name.in)

### Properties DLL

All the user interaction with a VPN plugin starts from the NetworkManager library: it imports the plugin properties DLL.
The code in the DLL will be used for plugin description, as well as VPN connection creation/reconfiguration/saving/loading.
Basically all the user actions regarding the VPN plugin will be processed by this DLL.

The DLL relies heavily on the [`libnm`](https://networkmanager.dev/docs/libnm/latest/) C library, and so probably should also be implemented in C.
Here's an approximate list of functions that should be implemented there:

- `[PLUGIN_NAME]_editor_plugin_class_init` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L151)): fill in the plugin class properties, including `name`, `description`, `service` and `get_property` method.
- `[PLUGIN_NAME]_editor_plugin_init` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L162)): fill in the plugin properties, hardly ever useful, but still has to be present.
- `[PLUGIN_NAME]_editor_plugin_interface_init` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L164)): fill in the plugin interface properties, including `get_editor`, `get_capabilities` and also `import_from_file`, `export_to_file` and `get_suggested_filename` if the corresponding capabilities are enabled.
- `nm_vpn_editor_factory_[PLUGIN_NAME]` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L174)): load the editor GUI GTK object, boilerplate code in most cases.
- `nm_vpn_editor_plugin_factory` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L180)): construct the plugin object, boilerplate code in most cases.
- ... and more `GLib`-related boilerplate code, defined in header ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.h)) and source ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c)).

So what's exactly going on here?
The most basic functionality is provided by the plugin class ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L151)).
These methods will be executed whenever NetworkManager attempts to discover and describe your plugin, and they have nothing to do with real VPN connections.
In case the `get_property` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L132)) method fails, the plugin will never load and will exit silently even before it appears in any NetworkManager GUI or CLI.
The three properties actually matter:

- Plugin name: plugin name, can really just be copied from the configuration name file
- Plugin description: longer plugin description that will be displayed in different kinds of GUIs
- Plugin service name: the D-Bus service name, again, can just be copied from the configuration name file

The remaining functionality is defined in the plugin interface implementation ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/editor.c#L164)) methods.
It includes connection importing and exporting (which can be disabled) and GUI provision (which apparently only matters for GUI tools and not for `nmcli`).
The methods in question are:

- `get_editor`: create and return a GTK object for displaying in GUI; this one is tricky and will be discussed later in [Editor GUI](#editor-gui) section.
- `get_capabilities`: return an integer specifying plugin capability flags:
  - `NM_VPN_EDITOR_PLUGIN_CAPABILITY_NONE`: no known capabilities present.
  - `NM_VPN_EDITOR_PLUGIN_CAPABILITY_IMPORT`: plugin can import connections from file (`import_from_file` method is enabled).
  - `NM_VPN_EDITOR_PLUGIN_CAPABILITY_EXPORT`: plugin can export connections to file (`export_to_file` method is enabled).
  - `NM_VPN_EDITOR_PLUGIN_CAPABILITY_IPV6`: plugin can handle IPv6 addressing.
- `import_from_file`: load and verify plugin parameters from a file.
- `export_to_file`: store plugin parameters in a file.
- `get_suggested_filename`: suggest a name for the newly-created export file.

> NB! Apparently, most of the methods of the plugin interface are not required, so in case your plugin doesn't need them, they can be just set to `NULL` in the interface implementation, and it will not cause an error.

The sole purpose of these methods is managing the [`NMVpnEditorPlugin`](https://networkmanager.dev/docs/libnm/latest/NMVpnEditorPlugin.html) and [`NMConnection`](https://networkmanager.dev/docs/libnm/latest/NMConnection.html) objects, either filling them in with appropriate parameters or dumping their values into a file.
It can be done either from/to GUI or from/to file.

> Typically this DLL will be named as `libnm-vpn-plugin-[PLUGIN_NAME].so` and placed to `/usr/lib/NetworkManager/`.

Here are a few properties DLL implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/tree/main/properties)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/tree/main/properties)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/tree/main/properties)

## Backend

Whenever a VPN connection was created and verified by the properties DLL, it will be serialized and passed to the plugin backend.
The backend is an executable, integrated with D-Bus service, it will be automatically started by the NetworkManager in case it is not already running.
It should receive signals, dispatched by the NetworkManager, start and stop VPN according to them.

### Configuration file

First of all, D-Bus won't allow anyone to just connect any services they want.
We have to explicitly allow NetworkManager to start the plugin service.
We will also allow root user to start it manually, for debugging purposes.

In order to do that, we will have to place a special file named `nm-[PLUGIN_NAME]-service.conf` into D-Bus directory `/usr/share/dbus-1/system.d`.
That's how it looks like in most of the open source VPN plugin implementations:

```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN" "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
    <policy user="root">
        <allow own="org.freedesktop.NetworkManager.[PLUGIN_NAME]"/>
        <allow send_destination="org.freedesktop.NetworkManager.[PLUGIN_NAME]"/>
        <allow receive_sender="org.freedesktop.NetworkManager.[PLUGIN_NAME]"/>
    </policy>
    <policy context="default">
        <deny own="org.freedesktop.NetworkManager.[PLUGIN_NAME]"/>
        <deny send_destination="org.freedesktop.NetworkManager.[PLUGIN_NAME]"/>
    </policy>
</busconfig>
```

Here are a few D-Bus configuration file implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/blob/main/nm-openvpn-service.conf)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/blob/main/nm-openconnect-service.conf)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/blob/main/nm-fortisslvpn-service.conf)

### Plugin executable

Unlike the frontend DLL, the backend executable does not rely on [`libnm`](https://networkmanager.dev/docs/libnm/latest/) C library that much, still all the examples that I have ever found are in C, so probably it's still the best choice for implementation, especially if in your case the backend is just a proxy between D-Bus and the actual VPN implementation (just like in mine).
Again, in order for this setup to work, some `GLib`-related functions should be implemented:

- `nm_[PLUGIN_NAME]_plugin_init` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L178)): initialize the private payload of your plugin.
- `nm_[PLUGIN_NAME]_plugin_class_init` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L182)): initialize your plugin class with the function implementations that match the ones expected by the D-Bus [interface](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html): `connect`, `disconnect`, `need_secrets`.
- `nm_[PLUGIN_NAME]_plugin_new` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L189)): construct the plugin object, boilerplate code in most cases.
- ... and also main function, termination callback, etc.

So, what's this all about?
Keeping all the fancy `GLib` stuff aside, the goal is rather simple: the backend executable creates an object that implements NetworkManager D-Bus [interface](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html), connects to D-Bus, starts listening to messages coming from it and enters an infinite loop.
Speaking of the interface, don't be afraid of it being huge and complex, we will only need some parts of it.
Still, it's important to understand how exactly it works.

> NB! Apparently, most of the methods of the NetworkManager D-Bus interface are not required, so in case your plugin doesn't need them, they can be just set to `NULL` in the interface implementation, and it will not cause an error.

So the lifecycle of the plugin implementing NetworkManager D-Bus interface can be simplified to this:

1. NetworkManager calls [`ConnectInteractive`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.ConnectInteractive) method of the plugin, if it's implemented, allowing the plugin to start, asking for the secrets interactively.
2. If it's not implemented, NetworkManager calls [`NeedSecrets`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.NeedSecrets) method of the plugin, asking if the plugin needs any secrets to start, and returning this information to the user, calling plugin start.
3. If `NeedSecrets` was called, NetworkManager calls [`Connect`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.Connect) method of the plugin.
4. Plugin calls [`SetConfig`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.SetConfig) method, then [`SetIp4Config`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.SetIp4Config) and [`SetIp6Config`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.SetIp6Config) (if needed, in this [order](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html)), providing NetworkManager with the information about the connection created.
5. Plugin works.
6. Plugin calls [`SetFailure`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.SetFailure) method to indicate VPN connection failure, if an error occurs.
7. NetworkManager calls [`Disconnect`](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html#gdbus-method-org-freedesktop-NetworkManager-VPN-Plugin.Disconnect) method of the plugin to indicate connection termination.

> NB! The methods should be executed **sequentially**, meaning, for instance, that you _can not_ call `SetConfig` before you return from `Connect` (even though it's tempting, I know).

So what exactly will we implement?
For our purpose, just a few methods will be enough:

- `real_connect` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L115)): extract VPN configuration (we'll use _data_ plaintext configuration instead of encrypted _secrets_ cuz we've got nothing to hide), start VPN and pass its configuration to `examplevpn_set_config_idle`.
- `examplevpn_set_config_idle` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L65)): parse VPN configuration object, serialize it to [`GVariant`](https://docs.gtk.org/glib/struct.Variant.html) and send it to NetworkManager.
- `real_disconnect` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L163)): stop VPN connection.
- `empty_need_secrets` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L170)): just indicate that no secrets are needed for the current connection.
- `capture_error_async` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L52)) and `capture_error_idle` ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/plugin.c#L38)): used as callbacks from the VPN, indicating an error.

> NB! D-Bus is not thread-safe, and since I used [tokio](https://tokio.rs/) in my Rust DLL implementation, error callback should be made thread-safe explicitly by scheduling it to the main thread with zero delay (just like we did with `examplevpn_set_config_idle` in order for it to be executed right after `real_connect` terminates).
> GLib provides `g_idle_add()` for scheduling callbacks on the main loop, which is the recommended way to communicate back from background threads.

As for the actual VPN implementation, here I used a DLL, but that's for compatibility with other platforms only (e.g. it's not possible to include a static library into a standard Android Framework app), otherwise it could've easily been a static library, maybe even implemented in plain C (although it would be a pain to maintain).
The example project includes a minimal [stub core DLL](https://github.com/pseusys/personal-blog/tree/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/plugin) that implements the ABI defined in [`examplevpn_core.h`](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/plugin/examplevpn_core.h) with hard-coded demo values instead of a real VPN tunnel — replace it with your actual implementation.
This approach tightly couples the plugin (owning the connection) and the actual connection itself (they live in the same process).
Still, surprisingly, all the other NetworkManager plugins that I have seen prefer using a VPN binary executable and start a connection as a child process.
The parameters are passed either as CLI arguments in that case or even temporary files, the connection configuration is extracted from parsing VPN binary `STDOUT`.
This solution seems to be less robust to me and also introduces big process management and string parsing overhead, but apparently it also has some advantages.

Anyway, here are a few plugin executable implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/tree/main/src)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/tree/main/src)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/tree/main/src)

## Editor GUI

If you made it to this point, that means that you've just created a fully-functional NetworkManager VPN plugin compatible with `nmcli` completely (almost) by yourself!
That's a goal not many people have achieved (at least online) and I congratulate you on that!

Still, there are a few more things that can be done to improve the user experience of your plugin.
You see, NetworkManager is fairly common on GUI-based Linux distributions.
It doesn't have a common single GUI itself though, instead there are a few implementations available out there.
For example, on Ubuntu (which I use) there is a [`nm-connection-editor`](https://manpages.ubuntu.com/manpages/noble/en/man1/nm-connection-editor.1.html) app pre-installed, but also NetworkManager is embedded into system settings.

Of course it would be great to target both of them.
But there's a thing.
Ubuntu system settings already use [GTK4](https://docs.gtk.org/gtk4/), but `nm-connection-editor` app still uses [GTK3](https://docs.gtk.org/gtk3/).
Apparently, it's a common issue across different distributions, so many VPN plugins that include GUI provide both GTK3 and GTK4 support at the same time.

How?
Well, the solution is not really elegant.
Apparently, GTK3 and GTK4 are **really** incompatible, which makes conditional version-dependent compilation hard to impossible.
So instead we're gonna implement all GUI-related logic in two separate files, each corresponding to specific GTK version (version 3 ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/interface_gtk3.c)) and version 4 ([source](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/interface_gtk4.c))).
We will compile them and ship both as DLLs alongside with our properties DLL.
Finally, inside of the properties DLL we're gonna implement the `get_editor` method so that it checks the runtime GTK version, loads corresponding DLL and delegates building GUI to it.

I will not go into detail much about GUI implementation itself, because it's very implementation-specific.
Basically, `get_editor` method accepts a [`NMConnection`](https://networkmanager.dev/docs/libnm/latest/NMConnection.html) object and returns a [`NMVpnEditor`](https://networkmanager.dev/docs/libnm/latest/NMVpnEditorPlugin.html) object, that should have two methods overridden:

- `get_widget`: construct a GTK widget, that will be directly embedded into the NetworkManager GUI implementation window.
- `update_connection`: check validity of the fields of the GTK widget, and if it contains valid set of values, update the given connection with them.

Of course, the user expects the values that are set in the widget to represent the actual connection properties, so it's important to populate the widget fields from the given `NMConnection` properties upon creation.

> NB! Apparently, the NetworkManager implementations usually call `update_connection` right after it's created, so another reason to pay attention to correctly populating its fields; it's also a good reason to pay extra attention to field validation.

One last thing that I would like to mention is resource management: just like many other GUI frameworks GTK supports static widget configuration and creation from a `.ui` XML file.
Sadly, the `.ui` file won't help you configure everything: some filters, callbacks, etc. still will require manual definition, but it does help to reduce the amount of boilerplate code and also provides a rough overview of the widget outlook.
Some plugins ship the `.ui` files, but I prefer bundling them using [GLib resource](https://docs.gtk.org/gio/struct.Resource.html) functionality.

So apart from the source files, I have also created two `.ui` files (for [GTK3](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/res/dialog_gtk3.ui) and for [GTK4](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/res/dialog_gtk4.ui)) and a [resource file](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/res/gresource.xml) for bundling them.
A C source file is created from the resource file at build time, compiled and linked statically with the editor DLLs.

And here are a few editor GUI implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/tree/main/properties)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/tree/main/properties)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/tree/main/properties)

## Credentials manager

One last thing!
If you install our plugin right now and compare it to the other plugins, you can notice one weird thing in its behavior.
Unlike other plugins that load instantly, our plugin experiences a ~30 seconds delay before its GUI even gets rendered.
If you observe the D-Bus logs, you'll notice a stale request to the NetworkManager [secret agent](https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.SecretAgent.html), that times out right in 30 seconds.

That happens because our plugin does not support `auth-dialog` yet.
For some reason, even if plugin backend signals that it doesn't ever need any secrets at all, network manager still requests a secret lookup from the secret agent (e.g. GNOME Keyring or `nm-applet`) for this plugin.
The secret agent, in turn, looks for the auth dialog, can't find it, and then just does nothing instead of reporting back (don't even ask me why at this point), causing request timeout.

Luckily, we can easily solve it by providing the most simple `auth-dialog` executable ever.
Since we don't ever need any secrets, the binary should just exist, accept input arguments, do nothing and then just print two "`\n`" symbols to STDOUT, that's it.

I assume that in most cases you can just copy-paste my [implementation](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/src/auth_dialog.c) for similar no-secret scenario.
You will indeed have to supply it with some logic in case your VPN plugin requires any secrets, but as I mentioned above, this is out of scope of this guide.

Here are a few `auth-dialog` implementations available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/tree/main/auth-dialog)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/tree/main/auth-dialog)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/tree/main/auth-dialog)

## Putting it all together

As far as I understood, most plugins use the same file layout and the same build convention.
I didn't really like it to be honest, as it relies on some of the tools that I would like to omit and lacks some tools that I would like to include.

With all respect, stacking `make`, `autoconf`, shell and python scripts seems a little obscure and too complex to me.
That's why I came up with my own simple project layout, relying on [Meson](https://mesonbuild.com/) as a modern and flexible build system.
Apart from compilation, it handles installation, deployment, code scanning with [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) and formatting with [clang-format](https://clang.llvm.org/docs/ClangFormat.html).

> NB! In the beginning I also wanted to use [Conan](https://conan.io/) for automatic dependency management, but apparently the default Conan package registry lacks most of the open-source GNOME libraries and also it does not allow depending on two versions of GTK simultaneously, so it's not the best fit here.
> That's why you should make sure that the required libraries are installed with the default OS package manager.
> These libraries include: `gcc` (prefer `build-essential` if possible), `pkg-config`, `glib-2.0`, `libnm`, `gtk+-3.0`, `gtk4`, `libsecret-1`, `clang-tidy` and `clang-format` (both optional).

You can either review my setup [here](https://github.com/pseusys/personal-blog/blob/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/meson.build) or use any of the other open-source plugin projects available online:

- [NetworkManager-openvpn](https://github.com/GNOME/NetworkManager-openvpn/blob/main/Makefile.am)
- [NetworkManager-openconnect](https://github.com/GNOME/NetworkManager-openconnect/blob/main/Makefile.am)
- [NetworkManager-fortisslvpn](https://github.com/GNOME/NetworkManager-fortisslvpn/blob/main/Makefile.am)

## Debugging

Here are a few practical tips I discovered while debugging my plugin:

- **Restart NetworkManager** after every change to the `.name` file or the frontend DLL: `sudo systemctl restart NetworkManager`.
- **Check if NetworkManager sees your plugin**: `nmcli connection show` should list your VPN type.
- **Create a test connection**: `nmcli connection add type vpn con-name test-vpn vpn-type [PLUGIN_NAME]`.
- **Run the backend manually** for debugging instead of letting D-Bus start it. Kill any existing instance first, then run the executable directly as root. This way you can see all `g_debug` / `g_warning` / `g_message` output in real time.
- **Monitor D-Bus messages** with `dbus-monitor --system` to see what NetworkManager sends to your plugin and what your plugin responds with.
- **Check the system journal** for NetworkManager logs: `journalctl -u NetworkManager -f`.
- **Common pitfall**: calling `SetConfig` from within the `Connect` handler instead of scheduling it with `g_idle_add`. This causes a deadlock because D-Bus is not re-entrant.

## Notes

- The full example plugin source code is available at [ExampleVpnPlugin](https://github.com/pseusys/personal-blog/tree/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin).
- The example project ships a standalone demo application rather than a real shared library VPN plugin — the [stub core DLL](https://github.com/pseusys/personal-blog/tree/main/sources/misc/netwprk-manager-vpn-plugin/ExampleVpnPlugin/plugin) returns hard-coded values and does not establish an actual VPN tunnel. Replace it with your own implementation.
- A [Containerfile](https://github.com/pseusys/personal-blog/blob/main/sources/misc/network-manager-vpn-plugin/Containerfile) is provided for verifying a clean build and plugin detection on Fedora (based on the NetworkManager CI image).
- The example plugin uses [Meson](https://mesonbuild.com/) + [Ninja](https://ninja-build.org/) as its build system.
- If you find any errors or have suggestions, feel free to open an issue or leave a comment!
