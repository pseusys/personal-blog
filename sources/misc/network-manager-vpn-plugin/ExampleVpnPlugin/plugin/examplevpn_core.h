/*
 * ExampleVpnPlugin: VPN core shared-library ABI
 *
 * This header defines the interface between the NetworkManager plugin backend
 * (GLib/C adapter) and the actual VPN implementation (the "core").
 *
 * The core is shipped as a shared library loaded with dlopen() at runtime.
 * This makes the VPN logic reusable across platforms (Linux, Android, etc.)
 * without pulling in GLib or NetworkManager dependencies.
 *
 * Architecture:
 *   NetworkManager (D-Bus) <-> plugin backend (GLib) <-> VPN core (this ABI)
 *
 * Notes:
 * - Keep this ABI small and stable.
 * - Prefer passing one "config blob" (certificate file contents) instead of
 *   many per-field parameters.  It keeps GUI validation and cross-platform
 *   support simpler.
 * - All integers are in host byte order.
 * - Strings are UTF-8 null-terminated.
 */

#ifndef EXAMPLEVPN_CORE_H
#define EXAMPLEVPN_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Types ─────────────────────────────────────────────────────────────── */

/* Configuration reported to NetworkManager once a tunnel is established. */
typedef struct ExampleVpnConfig {
  uint32_t remote_address; /* external gateway (host byte order) */
  uint32_t tunnel_gateway; /* internal gateway */
  uint32_t tunnel_address; /* internal address */
  uint32_t tunnel_prefix;  /* CIDR prefix length */
  uint32_t dns_address;    /* optional, 0 = none */
  uint32_t tunnel_mtu;     /* optional, 0 = default */
  char *tunnel_name;       /* e.g. "tun0", NULL = auto */
} ExampleVpnConfig;

/* Opaque handle for a running VPN session (owned by the core). */
typedef struct ExampleVpnSession ExampleVpnSession;

/* Error callback — may be invoked from any thread. */
typedef void (*examplevpn_error_cb)(void *user_ctx, const char *error_message);

/* ── Functions exported by the core DLL ────────────────────────────────── */

/*
 * examplevpn_initialize_logging
 *
 * Optional one-time setup for the core library (e.g. init a logger).
 * Called once before any other function.
 */
void examplevpn_initialize_logging(void);

/*
 * examplevpn_start
 *
 * Start a VPN session.
 *
 * Inputs:
 *   certificate_data / certificate_len — raw config blob (file contents).
 *   protocol       — short string selecting a mode (e.g. "typhoon", "port").
 *   user_ctx       — opaque pointer forwarded to on_error.
 *   on_error       — async error callback (thread-safe; use g_idle_add to
 *                    forward to the GLib main loop).
 *
 * Outputs:
 *   out_cfg        — allocated config; caller frees with
 * examplevpn_free_config. out_session    — opaque session handle; caller stops
 * with examplevpn_stop. out_error      — on failure, an allocated message;
 * caller frees with examplevpn_free_error.
 *
 * Returns true on success, false on failure.
 */
bool examplevpn_start(const uint8_t *certificate_data, size_t certificate_len,
                      const char *protocol, ExampleVpnConfig **out_cfg,
                      ExampleVpnSession **out_session, void *user_ctx,
                      examplevpn_error_cb on_error, char **out_error);

/*
 * examplevpn_stop
 *
 * Tear down a running session.
 * Returns true on success. On failure out_error may be set.
 */
bool examplevpn_stop(ExampleVpnSession *session, char **out_error);

/*
 * examplevpn_free_error / examplevpn_free_config
 *
 * Free memory allocated by the core.
 */
void examplevpn_free_error(char *error_message);
void examplevpn_free_config(ExampleVpnConfig *cfg);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EXAMPLEVPN_CORE_H */
