#ifndef NM_EXAMPLEVPN_COMMON_H
#define NM_EXAMPLEVPN_COMMON_H

#include <NetworkManager.h>

// Keys stored under NMSettingVpn (s_vpn) data items.
#define NM_EXAMPLEVPN_KEY_CERTIFICATE "certificate"
#define NM_EXAMPLEVPN_KEY_PROTOCOL "protocol"

typedef unsigned int (*get_major_version_fn)(void);
typedef NMVpnEditor *(*create_examplevpn_editor_fn)(NMConnection *, GError **);

typedef union {
  void *pointer;
  get_major_version_fn get_major_version;
  create_examplevpn_editor_fn create_examplevpn_editor;
} dll_function;

#endif /* NM_EXAMPLEVPN_COMMON_H */
