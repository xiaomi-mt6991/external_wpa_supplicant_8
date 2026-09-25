/*
 * MTK gen4m vendor command support for wpa_supplicant.
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "includes.h"

#include <errno.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>

#include "common.h"
#include "driver_nl80211.h"

/*
 * The physical-device wpa_supplicant build sets wpa_supplicant_use_stub_lib
 * to false. That enables the driver_cmd operation in driver_nl80211.c and
 * also means the Android P2P fallback symbols are not supplied by
 * driver_nl80211_android.c. Keep the MTK handler and those fallback symbols
 * together in the product build; the mainline/stub build continues to use
 * the upstream implementations.
 */
#ifndef ANDROID_LIB_STUB

/* Must match PRIV_CMD_SIZE and struct priv_driver_cmd_s in the MTK driver. */
#define MTK_PRIV_CMD_SIZE 512

struct mtk_wifi_priv_cmd {
	char buf[MTK_PRIV_CMD_SIZE];
	int used_len;
	int total_len;
};

static int mtk_priv_cmd(struct i802_bss *bss, const char *cmd)
{
	struct mtk_wifi_priv_cmd priv_cmd;
	struct ifreq ifr;
	size_t cmd_len;
	int ret;

	if (bss == NULL || bss->drv == NULL || bss->drv->global == NULL ||
	    cmd == NULL) {
		errno = EINVAL;
		return -1;
	}

	cmd_len = os_strlen(cmd);
	if (cmd_len == 0 || cmd_len >= MTK_PRIV_CMD_SIZE) {
		errno = EINVAL;
		return -1;
	}

	os_memset(&ifr, 0, sizeof(ifr));
	os_memset(&priv_cmd, 0, sizeof(priv_cmd));
	os_strlcpy(ifr.ifr_name, bss->ifname, IFNAMSIZ);
	os_memcpy(priv_cmd.buf, cmd, cmd_len);
	priv_cmd.buf[cmd_len] = '\0';
	priv_cmd.used_len = (int)cmd_len + 1;
	priv_cmd.total_len = MTK_PRIV_CMD_SIZE;
	ifr.ifr_data = &priv_cmd;

	ret = ioctl(bss->drv->global->ioctl_sock, SIOCDEVPRIVATE + 1, &ifr);
	if (ret < 0) {
		wpa_printf(MSG_ERROR, "mtk_priv_cmd: '%s' failed: %s", cmd,
			   strerror(errno));
		return -1;
	}

	wpa_printf(MSG_DEBUG, "mtk_priv_cmd: '%s' issued", cmd);
	return 0;
}

int wpa_driver_nl80211_driver_cmd(void *priv, char *cmd, char *buf,
				  size_t buf_len)
{
	struct i802_bss *bss = (struct i802_bss *)priv;

	if (cmd == NULL) {
		errno = EINVAL;
		return -1;
	}

	/* The framework sends: SETSUSPENDMODE 1 or SETSUSPENDMODE 0. */
	if (os_strncasecmp(cmd, "SETSUSPENDMODE ", 15) != 0) {
		errno = ENOTSUP;
		return -1;
	}

	if (mtk_priv_cmd(bss, cmd) < 0) {
		if (buf != NULL && buf_len > 0)
			os_snprintf(buf, buf_len, "%s", strerror(errno));
		return -1;
	}

	if (buf != NULL && buf_len > 0)
		buf[0] = '\0';
	return 0;
}

#ifdef ANDROID_P2P
/* Match the upstream libdrivercmdfallback behavior for the physical build. */
int wpa_driver_set_p2p_noa(void *priv, u8 count, int start, int duration)
{
	return 0;
}

int wpa_driver_get_p2p_noa(void *priv, u8 *buf, size_t len)
{
	return 0;
}

int wpa_driver_set_p2p_ps(void *priv, int legacy_ps, int opp_ps, int ctwindow)
{
	return -1;
}

int wpa_driver_set_ap_wps_p2p_ie(void *priv, const struct wpabuf *beacon,
				 const struct wpabuf *resperesp,
				 const struct wpabuf *assocresp)
{
	return 0;
}
#endif /* ANDROID_P2P */

#endif /* !ANDROID_LIB_STUB */
