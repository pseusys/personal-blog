# A custom NetworkManager VPN plugin 101

Have you decided to write your own VPN, just like me?
Or would you just like to integrate your favourite solution with the convenient user interface of Linux **NetworkManager**?
Anyway, you came to the right place.

NetworkManager is that omnipresent Linux networking daemon that:

- happily switches between Wi‑Fi networks,
- keeps routes and DNS from turning into a Jackson Pollock painting,
- and (crucially for us) has a **VPN plugin interface**.

The good news: once you plug into NetworkManager, you get all the desktop UX for free: `nmcli`, GNOME connection editors, auto‑connect, routing, DNS, secrets prompts, etc.

The bad news: the plugin interfaces are… not exactly a tutorial-friendly wonderland.
Most of the time you end up reading other plugins’ source code, squinting at GLib macros, and quietly questioning your life choices.

So let’s do the squinting once, write it down, and move on.

> Disclaimer: I’m not affiliated with NetworkManager or GNOME. I learned most of this by reading existing plugins and the official reference docs.

---

## What you build (high level)

A NetworkManager VPN plugin is usually split into two pieces:

### 1) Frontend (user space, loaded by libnm)

This is a shared library that GUI tools (and other `libnm` clients) load to:

- show your VPN type in the UI,
- create/edit the connection,
- optionally import/export profiles,
- optionally show the editor widget.

### 2) Backend (system, activated by NetworkManager via D‑Bus)

This is an executable that implements the D‑Bus interface:

- `org.freedesktop.NetworkManager.VPN.Plugin`

NetworkManager runs it (usually as root), calls `Connect()` / `Disconnect()`, and expects you to report back the resulting configuration (`SetConfig`, `SetIp4Config`, `SetIp6Config`).

Reference:

- VPN plugin D‑Bus API: <https://networkmanager.dev/docs/api/latest/gdbus-org.freedesktop.NetworkManager.VPN.Plugin.html>

---

## Files you typically ship

A *minimal* plugin distribution usually contains:

- A **`.name` file** (plugin discovery)
- A **frontend `.so`** (editor/plugin)
- A **backend executable** (VPN service)
- A **D‑Bus policy** file (so NM can activate/talk to your service)
- Optionally: an **auth-dialog helper** (secrets UI integration)

You can see a working example layout in this repo under `ExampleVpnPlugin/`.

---

## Frontend

### The `.name` file

To make NetworkManager discover the plugin, you install:

- `nm-<PLUGIN>-service.name` into `/usr/lib/NetworkManager/VPN/`

This file is an INI-ish descriptor. The exact full schema is not nicely documented, but the common keys look like this:

```ini
[VPN Connection]
name=ExampleVpnPlugin
service=org.freedesktop.NetworkManager.examplevpn
program=/usr/lib/NetworkManager/nm-examplevpn-service
supports-multiple-connections=false

[libnm]
plugin=/usr/lib/NetworkManager/libnm-vpn-plugin-examplevpn.so

[GNOME]
auth-dialog=/usr/lib/NetworkManager/nm-examplevpn-auth-dialog
supports-external-ui-mode=false
supports-hints=false
```

Notes:

- `service` must match the D‑Bus service name your backend will own.
- `program` is the backend executable.
- `[libnm].plugin` is the frontend plugin `.so`.

After you edit/install these files, restart NetworkManager:

```bash
sudo systemctl restart NetworkManager
```

### The editor plugin

This is the part that tends to be… GLib-heavy.

You implement an `NMVpnEditorPlugin` that returns:

- metadata (name/description/service),
- capabilities (import/export, etc.),
- and optionally an editor widget.

In this repo, the example lives in:

- `ExampleVpnPlugin/src/editor.c`

### GTK3 vs GTK4 editor UI

Welcome to 2026, where some systems still run GTK3 connection editors while others embed VPN config into GTK4 settings apps.

A practical approach is:

- ship two editor UI implementations (GTK3 and GTK4),
- choose at runtime.

Example:

- `ExampleVpnPlugin/src/interface_gtk3.c`
- `ExampleVpnPlugin/src/interface_gtk4.c`

GTK docs:

- GTK 3: <https://docs.gtk.org/gtk3/>
- GTK 4: <https://docs.gtk.org/gtk4/>

---

## Backend

### D‑Bus policy

System D‑Bus won’t let random binaries claim random well-known names.
So you typically ship a policy file to allow NetworkManager to activate and talk to your service.

Usually installed as:

- `/usr/share/dbus-1/system.d/nm-<PLUGIN>-service.conf`

Skeleton:

```xml
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <policy user="root">
    <allow own="org.freedesktop.NetworkManager.examplevpn"/>
    <allow send_destination="org.freedesktop.NetworkManager.examplevpn"/>
    <allow receive_sender="org.freedesktop.NetworkManager.examplevpn"/>
  </policy>
  <policy context="default">
    <deny own="org.freedesktop.NetworkManager.examplevpn"/>
    <deny send_destination="org.freedesktop.NetworkManager.examplevpn"/>
  </policy>
</busconfig>
```

### Lifecycle: what NM calls

NetworkManager calls your backend roughly like this:

1. `Connect()` or `ConnectInteractive()`
2. (Optional) secrets request via `SecretsRequired` / `NewSecrets`
3. once connected, you report:
   - `SetConfig()`
   - `SetIp4Config()`
   - `SetIp6Config()` (if needed)
4. later: `Disconnect()`

Yes, the order matters.
No, you can’t just call `SetConfig()` “whenever”. NetworkManager is grumpy.

---

## GLib: the 20% you need for the 80% of the job

If GLib feels like ritual magic — good, you’re reading it correctly.

Here’s the part that actually matters:

### You have a main loop

NetworkManager and most GNOME-ish things are built around an event loop.
D‑Bus calls arrive on that loop.
UI events run on that loop.
If you do something long/blocking on that loop, everyone suffers.

### Don’t call NM / D‑Bus APIs from random threads

If your VPN core uses threads (or async runtimes), you usually:

1) do work elsewhere,
2) then schedule the result back onto the main loop.

Two simplest tools:

- `g_idle_add(fn, data)` — run soon on the main loop
- `g_timeout_add(ms, fn, data)` — run later

This pattern is used by lots of plugins to avoid re-entrancy headaches.

GLib refs:

- Main loop: <https://docs.gtk.org/glib/main-loop.html>
- `g_idle_add`: <https://docs.gtk.org/glib/func.idle_add.html>
- GObject basics: <https://docs.gtk.org/gobject/>

---

## Next steps / TODO

- Fill in missing doc links (I’ll happily accept your curated list).
- Add a second example that is *even more minimal* (no GTK, just nmcli).
- Add a section on secrets (`libsecret`) once we decide on the UX.
