# ExampleVpnPlugin (NetworkManager VPN plugin example)

This repository contains two documents:

- **README.md** (this file): how to build and install the **ExampleVpnPlugin** sample implementation.
- **TUTORIAL.md**: the guide / explanation of NetworkManager VPN plugin architecture (written in a more “human” tone).

The example implementation lives in:

- `ExampleVpnPlugin/`

It is intentionally **self-contained** and does not depend on any private repositories.

---

## Quick start (Ubuntu)

### 1) Install build dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  meson ninja-build pkg-config \
  clang-format clang-tidy \
  libglib2.0-dev libnm-dev \
  libgtk-3-dev libgtk-4-dev \
  libsecret-1-dev
```

Notes:

- We use **Meson + Ninja**.
- `clang-format` / `clang-tidy` are optional for building, but this project’s Meson file expects them to exist.

### 2) Configure and build

```bash
cd ExampleVpnPlugin

# Configure
meson setup build \
  --buildtype=debugoptimized

# Build
meson compile -C build
```

What these options mean:

- `--buildtype=debugoptimized`: optimized build with debug symbols (nice for debugging without being painfully slow).
- The project also enables a fairly strict warning set and treats warnings as errors (see `meson.build`).

### 3) Install (system-wide)

This installs files into `/usr` paths by default (because the Meson project sets `prefix=/usr`).

```bash
sudo meson install -C build
```

If you want to stage the install into a directory (packaging / inspection), use `DESTDIR`:

```bash
DESTDIR="$PWD/_dest" meson install -C build
find "$PWD/_dest" -maxdepth 4 -type f | sort
```

### 4) Restart NetworkManager

```bash
sudo systemctl restart NetworkManager
```

---

## What gets installed (file layout)

The Meson project installs (paths may vary slightly by distro/libdir):

- `.name` descriptor:
  - `/usr/lib/NetworkManager/VPN/nm-examplevpn-service.name`
- D-Bus policy:
  - `/usr/share/dbus-1/system.d/nm-examplevpn-service.conf`
- Backend service:
  - `/usr/libexec/nm-examplevpn-service`
- Auth-dialog helper:
  - `/usr/libexec/nm-examplevpn-auth-dialog`
- Frontend/editor plugin:
  - `/usr/lib/<arch>/NetworkManager/libnm-vpn-plugin-examplevpn.so`
- GTK editor helpers:
  - `/usr/lib/<arch>/NetworkManager/libnm-vpn-editor-examplevpn-gtk3.so`
  - `/usr/lib/<arch>/NetworkManager/libnm-vpn-editor-examplevpn-gtk4.so`

The authoritative list is in `ExampleVpnPlugin/meson.build`.

---

## Running / verifying

### Check that NM sees the plugin

```bash
nmcli -f NAME,TYPE connection show
```

### Create a connection (rough sketch)

Exact `nmcli` keys depend on what your editor/plugin expects.
For this example we store a few values under `NMSettingVpn` data items.

You can start by creating a VPN connection and then editing it in a GUI editor.

```bash
nmcli connection add type vpn con-name examplevpn-demo vpn-type examplevpn
```

---

## Code quality tools

### clang-format

Format the example sources:

```bash
cd ExampleVpnPlugin
clang-format -i src/*.[ch]
```

### clang-tidy

`clang-tidy` needs compile flags. The easiest way is to use the `compile_commands.json` from Meson:

```bash
cd ExampleVpnPlugin
meson setup build --buildtype=debugoptimized
ninja -C build -t compdb c cpp > compile_commands.json

# Example run (adjust checks as you like)
clang-tidy -p . src/plugin.c
```

---

## Building in a container (Podman/Docker)

This repo ships a `Dockerfile` that builds the example on a clean Ubuntu image:

```bash
podman build -t nm-plugin-buildtest:latest .
```

If your host Podman uses netavark and your server’s PATH does not include `/usr/sbin`, netavark may fail to find `iptables`.
A quick workaround is:

```bash
sudo ln -s /usr/sbin/iptables /usr/bin/iptables
sudo ln -s /usr/sbin/iptables-save /usr/bin/iptables-save
sudo ln -s /usr/sbin/iptables-restore /usr/bin/iptables-restore
```

---

## Where to read the guide

- See **TUTORIAL.md**.
