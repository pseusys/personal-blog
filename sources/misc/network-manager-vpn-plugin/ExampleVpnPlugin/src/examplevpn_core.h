/*
 * Compatibility shim: the canonical header now lives in plugin/.
 * This file exists so that src/plugin.c can keep including
 * "examplevpn_core.h" without changing its #include path.
 */

#include "../plugin/examplevpn_core.h"
