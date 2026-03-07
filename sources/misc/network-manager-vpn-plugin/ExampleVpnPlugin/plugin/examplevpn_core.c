/*
 * ExampleVpnPlugin: stub VPN core implementation
 *
 * This is a minimal, self-contained implementation of the VPN core ABI defined
 * in examplevpn_core.h.  It does not create real tunnels — it returns hard-
 * coded demo configuration so that the NetworkManager plugin can be exercised
 * end-to-end without any external VPN infrastructure.
 *
 * Replace the body of examplevpn_start() with your actual VPN logic.
 */

#include "examplevpn_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Internals ─────────────────────────────────────────────────────────── */

struct ExampleVpnSession {
  int running; /* 1 while the "tunnel" is up */
};

/* ── Public API ────────────────────────────────────────────────────────── */

void examplevpn_initialize_logging(void) {
  /* In a real implementation you might set up env-driven log levels, etc. */
  fprintf(stderr, "examplevpn_core: logging initialised (stub)\n");
}

bool examplevpn_start(const uint8_t *certificate_data, size_t certificate_len,
                      const char *protocol, ExampleVpnConfig **out_cfg,
                      ExampleVpnSession **out_session, void *user_ctx,
                      examplevpn_error_cb on_error, char **out_error) {
  (void)user_ctx;
  (void)on_error;

  if (!certificate_data || certificate_len == 0) {
    if (out_error)
      *out_error = strdup("certificate data is empty");
    return false;
  }

  if (!protocol || protocol[0] == '\0') {
    if (out_error)
      *out_error = strdup("protocol is empty");
    return false;
  }

  fprintf(stderr,
          "examplevpn_core: start requested (protocol=%s, cert_len=%zu)\n",
          protocol, certificate_len);

  /* Allocate demo tunnel configuration: 10.10.0.2/24 via 10.10.0.1 */
  ExampleVpnConfig *cfg = calloc(1, sizeof(*cfg));
  if (!cfg) {
    if (out_error)
      *out_error = strdup("out of memory");
    return false;
  }

  cfg->remote_address = (10u << 24) | (10u << 16) | (0u << 8) | 1u;
  cfg->tunnel_gateway = (10u << 24) | (10u << 16) | (0u << 8) | 1u;
  cfg->tunnel_address = (10u << 24) | (10u << 16) | (0u << 8) | 2u;
  cfg->tunnel_prefix = 24;
  cfg->dns_address = (1u << 24) | (1u << 16) | (1u << 8) | 1u; /* 1.1.1.1 */
  cfg->tunnel_mtu = 1400;
  cfg->tunnel_name = strdup("examplevpn0");

  ExampleVpnSession *session = calloc(1, sizeof(*session));
  if (!session) {
    free(cfg->tunnel_name);
    free(cfg);
    if (out_error)
      *out_error = strdup("out of memory");
    return false;
  }
  session->running = 1;

  *out_cfg = cfg;
  *out_session = session;
  return true;
}

bool examplevpn_stop(ExampleVpnSession *session, char **out_error) {
  (void)out_error;

  if (!session)
    return true;

  fprintf(stderr, "examplevpn_core: stop requested\n");
  session->running = 0;
  free(session);
  return true;
}

void examplevpn_free_error(char *error_message) { free(error_message); }

void examplevpn_free_config(ExampleVpnConfig *cfg) {
  if (!cfg)
    return;
  free(cfg->tunnel_name);
  free(cfg);
}
