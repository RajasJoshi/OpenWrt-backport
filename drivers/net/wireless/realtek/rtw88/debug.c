// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/* Copyright(c) 2018-2019  Realtek Corporation
 */

#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include "main.h"
#include "coex.h"
#include "sec.h"
#include "fw.h"
#include "debug.h"
#include "phy.h"
#include "reg.h"
#include "ps.h"
#include "regd.h"

#ifdef CPTCFG_RTW88_DEBUGFS

struct rtw_debugfs_priv {
	struct rtw_dev *rtwdev;
	int (*cb_read)(struct seq_file *m, void *v);
	ssize_t (*cb_write)(struct file *filp, const char __user *buffer,
			    size_t count, loff_t *loff);
	union {
		u32 cb_data;
		u8 *buf;
		struct {
			u32 page_offset;
			u32 page_num;
		} rsvd_page;
		struct {
			u8 rf_path;
			u32 rf_addr;
			u32 rf_mask;
		};
		struct {
			u32 addr;
			u32 len;
		} read_reg;
		struct {
			u8 bit;
		} dm_cap;
	};
};

struct rtw_debugfs {
	struct rtw_debugfs_priv mac_0;
	struct rtw_debugfs_priv mac_1;
	struct rtw_debugfs_priv mac_2;
	struct rtw_debugfs_priv mac_3;
	struct rtw_debugfs_priv mac_4;
	struct rtw_debugfs_priv mac_5;
	struct rtw_debugfs_priv mac_6;
	struct rtw_debugfs_priv mac_7;
	struct rtw_debugfs_priv mac_10;
	struct rtw_debugfs_priv mac_11;
	struct rtw_debugfs_priv mac_12;
	struct rtw_debugfs_priv mac_13;
	struct rtw_debugfs_priv mac_14;
	struct rtw_debugfs_priv mac_15;
	struct rtw_debugfs_priv mac_16;
	struct rtw_debugfs_priv mac_17;
	struct rtw_debugfs_priv bb_8;
	struct rtw_debugfs_priv bb_9;
	struct rtw_debugfs_priv bb_a;
	struct rtw_debugfs_priv bb_b;
	struct rtw_debugfs_priv bb_c;
	struct rtw_debugfs_priv bb_d;
	struct rtw_debugfs_priv bb_e;
	struct rtw_debugfs_priv bb_f;
	struct rtw_debugfs_priv bb_18;
	struct rtw_debugfs_priv bb_19;
	struct rtw_debugfs_priv bb_1a;
	struct rtw_debugfs_priv bb_1b;
	struct rtw_debugfs_priv bb_1c;
	struct rtw_debugfs_priv bb_1d;
	struct rtw_debugfs_priv bb_1e;
	struct rtw_debugfs_priv bb_1f;
	struct rtw_debugfs_priv bb_2c;
	struct rtw_debugfs_priv bb_2d;
	struct rtw_debugfs_priv bb_40;
	struct rtw_debugfs_priv bb_41;
	struct rtw_debugfs_priv rf_dump;
	struct rtw_debugfs_priv tx_pwr_tbl;
	struct rtw_debugfs_priv write_reg;
	struct rtw_debugfs_priv h2c;
	struct rtw_debugfs_priv rf_write;
	struct rtw_debugfs_priv rf_read;
	struct rtw_debugfs_priv read_reg;
	struct rtw_debugfs_priv fix_rate;
	struct rtw_debugfs_priv dump_cam;
	struct rtw_debugfs_priv rsvd_page;
	struct rtw_debugfs_priv phy_info;
	struct rtw_debugfs_priv coex_enable;
	struct rtw_debugfs_priv coex_info;
	struct rtw_debugfs_priv edcca_enable;
	struct rtw_debugfs_priv fw_crash;
	struct rtw_debugfs_priv force_lowest_basic_rate;
	struct rtw_debugfs_priv dm_cap;
};

static const char * const rtw_dm_cap_strs[] = {
	[RTW_DM_CAP_NA] = "NA",
	[RTW_DM_CAP_TXGAPK] = "TXGAPK",
};

static int rtw_debugfs_single_show(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;

	return debugfs_priv->cb_read(m, v);
}

static ssize_t rtw_debugfs_common_write(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct rtw_debugfs_priv *debugfs_priv = filp->private_data;

	return debugfs_priv->cb_write(filp, buffer, count, loff);
}

static ssize_t rtw_debugfs_single_write(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;

	return debugfs_priv->cb_write(filp, buffer, count, loff);
}

static int rtw_debugfs_single_open_rw(struct inode *inode, struct file *filp)
{
	return single_open(filp, rtw_debugfs_single_show, inode->i_private);
}

static int rtw_debugfs_close(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations file_ops_single_r = {
	.owner = THIS_MODULE,
	.open = rtw_debugfs_single_open_rw,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static const struct file_operations file_ops_single_rw = {
	.owner = THIS_MODULE,
	.open = rtw_debugfs_single_open_rw,
	.release = single_release,
	.read = seq_read,
	.llseek = seq_lseek,
	.write = rtw_debugfs_single_write,
};

static const struct file_operations file_ops_common_write = {
	.owner = THIS_MODULE,
	.write = rtw_debugfs_common_write,
	.open = simple_open,
	.release = rtw_debugfs_close,
};

static int rtw_debugfs_get_read_reg(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 val, len, addr;

	len = debugfs_priv->read_reg.len;
	addr = debugfs_priv->read_reg.addr;
	switch (len) {
	case 1:
		val = rtw_read8(rtwdev, addr);
		seq_printf(m, "reg 0x%03x: 0x%02x\n", addr, val);
		break;
	case 2:
		val = rtw_read16(rtwdev, addr);
		seq_printf(m, "reg 0x%03x: 0x%04x\n", addr, val);
		break;
	case 4:
		val = rtw_read32(rtwdev, addr);
		seq_printf(m, "reg 0x%03x: 0x%08x\n", addr, val);
		break;
	}
	return 0;
}

static int rtw_debugfs_get_rf_read(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 val, addr, mask;
	u8 path;

	path = debugfs_priv->rf_path;
	addr = debugfs_priv->rf_addr;
	mask = debugfs_priv->rf_mask;

	mutex_lock(&rtwdev->mutex);
	val = rtw_read_rf(rtwdev, path, addr, mask);
	mutex_unlock(&rtwdev->mutex);

	seq_printf(m, "rf_read path:%d addr:0x%08x mask:0x%08x val=0x%08x\n",
		   path, addr, mask, val);

	return 0;
}

static int rtw_debugfs_get_fix_rate(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 fix_rate = dm_info->fix_rate;

	if (fix_rate >= DESC_RATE_MAX) {
		seq_printf(m, "Fix rate disabled, fix_rate = %u\n", fix_rate);
		return 0;
	}

	seq_printf(m, "Data frames fixed at desc rate %u\n", fix_rate);
	return 0;
}

static int rtw_debugfs_copy_from_user(char tmp[], int size,
				      const char __user *buffer, size_t count,
				      int num)
{
	int tmp_len;

	memset(tmp, 0, size);

	if (count < num)
		return -EFAULT;

	tmp_len = (count > size - 1 ? size - 1 : count);

	if (copy_from_user(tmp, buffer, tmp_len))
		return -EFAULT;

	tmp[tmp_len] = '\0';

	return 0;
}

static ssize_t rtw_debugfs_set_read_reg(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u32 addr, len;
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 2);
	if (ret)
		return ret;

	num = sscanf(tmp, "%x %x", &addr, &len);

	if (num !=  2)
		return -EINVAL;

	if (len != 1 && len != 2 && len != 4) {
		rtw_warn(rtwdev, "read reg setting wrong len\n");
		return -EINVAL;
	}
	debugfs_priv->read_reg.addr = addr;
	debugfs_priv->read_reg.len = len;

	return count;
}

static int rtw_debugfs_get_dump_cam(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 val, command;
	u32 hw_key_idx = debugfs_priv->cb_data << RTW_SEC_CAM_ENTRY_SHIFT;
	u32 read_cmd = RTW_SEC_CMD_POLLING;
	int i;

	seq_printf(m, "cam entry%d\n", debugfs_priv->cb_data);
	seq_puts(m, "0x0      0x1      0x2     0x3     ");
	seq_puts(m, "0x4     0x5\n");
	mutex_lock(&rtwdev->mutex);
	for (i = 0; i <= 5; i++) {
		command = read_cmd | (hw_key_idx + i);
		rtw_write32(rtwdev, RTW_SEC_CMD_REG, command);
		val = rtw_read32(rtwdev, RTW_SEC_READ_REG);
		seq_printf(m, "%8.8x", val);
		if (i < 2)
			seq_puts(m, " ");
	}
	seq_puts(m, "\n");
	mutex_unlock(&rtwdev->mutex);
	return 0;
}

static int rtw_debugfs_get_rsvd_page(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u16 page_size = rtwdev->chip->page_size;
	u32 buf_size = debugfs_priv->rsvd_page.page_num * page_size;
	u32 offset = debugfs_priv->rsvd_page.page_offset * page_size;
	u8 *buf;
	int i;
	int ret;

	buf = vzalloc(buf_size);
	if (!buf)
		return -ENOMEM;

	ret = rtw_fw_dump_fifo(rtwdev, RTW_FW_FIFO_SEL_RSVD_PAGE, offset,
			       buf_size, (u32 *)buf);
	if (ret) {
		rtw_err(rtwdev, "failed to dump rsvd page\n");
		vfree(buf);
		return ret;
	}

	for (i = 0 ; i < buf_size ; i += 8) {
		if (i % page_size == 0)
			seq_printf(m, "PAGE %d\n", (i + offset) / page_size);
		seq_printf(m, "%8ph\n", buf + i);
	}
	vfree(buf);

	return 0;
}

static ssize_t rtw_debugfs_set_rsvd_page(struct file *filp,
					 const char __user *buffer,
					 size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u32 offset, page_num;
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 2);
	if (ret)
		return ret;

	num = sscanf(tmp, "%d %d", &offset, &page_num);

	if (num != 2) {
		rtw_warn(rtwdev, "invalid arguments\n");
		return -EINVAL;
	}

	debugfs_priv->rsvd_page.page_offset = offset;
	debugfs_priv->rsvd_page.page_num = page_num;

	return count;
}

static ssize_t rtw_debugfs_set_single_input(struct file *filp,
					    const char __user *buffer,
					    size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	u32 input;
	int ret;

	ret = kstrtou32_from_user(buffer, count, 0, &input);
	if (ret)
		return ret;

	debugfs_priv->cb_data = input;

	return count;
}

static ssize_t rtw_debugfs_set_write_reg(struct file *filp,
					 const char __user *buffer,
					 size_t count, loff_t *loff)
{
	struct rtw_debugfs_priv *debugfs_priv = filp->private_data;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u32 addr, val, len;
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 3);
	if (ret)
		return ret;

	/* write BB/MAC register */
	num = sscanf(tmp, "%x %x %x", &addr, &val, &len);

	if (num !=  3)
		return -EINVAL;

	switch (len) {
	case 1:
		rtw_dbg(rtwdev, RTW_DBG_DEBUGFS,
			"reg write8 0x%03x: 0x%08x\n", addr, val);
		rtw_write8(rtwdev, addr, (u8)val);
		break;
	case 2:
		rtw_dbg(rtwdev, RTW_DBG_DEBUGFS,
			"reg write16 0x%03x: 0x%08x\n", addr, val);
		rtw_write16(rtwdev, addr, (u16)val);
		break;
	case 4:
		rtw_dbg(rtwdev, RTW_DBG_DEBUGFS,
			"reg write32 0x%03x: 0x%08x\n", addr, val);
		rtw_write32(rtwdev, addr, (u32)val);
		break;
	default:
		rtw_dbg(rtwdev, RTW_DBG_DEBUGFS,
			"error write length = %d\n", len);
		break;
	}

	return count;
}

static ssize_t rtw_debugfs_set_h2c(struct file *filp,
				   const char __user *buffer,
				   size_t count, loff_t *loff)
{
	struct rtw_debugfs_priv *debugfs_priv = filp->private_data;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u8 param[8];
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 3);
	if (ret)
		return ret;

	num = sscanf(tmp, "%hhx,%hhx,%hhx,%hhx,%hhx,%hhx,%hhx,%hhx",
		     &param[0], &param[1], &param[2], &param[3],
		     &param[4], &param[5], &param[6], &param[7]);
	if (num != 8) {
		rtw_warn(rtwdev, "invalid H2C command format for debug\n");
		return -EINVAL;
	}

	mutex_lock(&rtwdev->mutex);
	rtw_fw_h2c_cmd_dbg(rtwdev, param);
	mutex_unlock(&rtwdev->mutex);

	return count;
}

static ssize_t rtw_debugfs_set_rf_write(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct rtw_debugfs_priv *debugfs_priv = filp->private_data;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u32 path, addr, mask, val;
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 4);
	if (ret)
		return ret;

	num = sscanf(tmp, "%x %x %x %x", &path, &addr, &mask, &val);

	if (num !=  4) {
		rtw_warn(rtwdev, "invalid args, [path] [addr] [mask] [val]\n");
		return -EINVAL;
	}

	mutex_lock(&rtwdev->mutex);
	rtw_write_rf(rtwdev, path, addr, mask, val);
	mutex_unlock(&rtwdev->mutex);
	rtw_dbg(rtwdev, RTW_DBG_DEBUGFS,
		"write_rf path:%d addr:0x%08x mask:0x%08x, val:0x%08x\n",
		path, addr, mask, val);

	return count;
}

static ssize_t rtw_debugfs_set_rf_read(struct file *filp,
				       const char __user *buffer,
				       size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	char tmp[32 + 1];
	u32 path, addr, mask;
	int num;
	int ret;

	ret = rtw_debugfs_copy_from_user(tmp, sizeof(tmp), buffer, count, 3);
	if (ret)
		return ret;

	num = sscanf(tmp, "%x %x %x", &path, &addr, &mask);

	if (num !=  3) {
		rtw_warn(rtwdev, "invalid args, [path] [addr] [mask] [val]\n");
		return -EINVAL;
	}

	debugfs_priv->rf_path = path;
	debugfs_priv->rf_addr = addr;
	debugfs_priv->rf_mask = mask;

	return count;
}

static ssize_t rtw_debugfs_set_fix_rate(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	u8 fix_rate;
	int ret;

	ret = kstrtou8_from_user(buffer, count, 0, &fix_rate);
	if (ret)
		return ret;

	dm_info->fix_rate = fix_rate;

	return count;
}

static int rtw_debug_get_mac_page(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 page = debugfs_priv->cb_data;
	int i, n;
	int max = 0xff;

	rtw_read32(rtwdev, debugfs_priv->cb_data);
	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtw_read32(rtwdev, (page | n)));
	}
	seq_puts(m, "\n");
	return 0;
}

static int rtw_debug_get_bb_page(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 page = debugfs_priv->cb_data;
	int i, n;
	int max = 0xff;

	rtw_read32(rtwdev, debugfs_priv->cb_data);
	for (n = 0; n <= max; ) {
		seq_printf(m, "\n%8.8x  ", n + page);
		for (i = 0; i < 4 && n <= max; i++, n += 4)
			seq_printf(m, "%8.8x    ",
				   rtw_read32(rtwdev, (page | n)));
	}
	seq_puts(m, "\n");
	return 0;
}

static int rtw_debugfs_get_rf_dump(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	u32 addr, offset, data;
	u8 path;

	mutex_lock(&rtwdev->mutex);

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		seq_printf(m, "RF path:%d\n", path);
		for (addr = 0; addr < 0x100; addr += 4) {
			seq_printf(m, "%8.8x  ", addr);
			for (offset = 0; offset < 4; offset++) {
				data = rtw_read_rf(rtwdev, path, addr + offset,
						   0xffffffff);
				seq_printf(m, "%8.8x    ", data);
			}
			seq_puts(m, "\n");
		}
		seq_puts(m, "\n");
	}

	mutex_unlock(&rtwdev->mutex);

	return 0;
}

static void rtw_print_cck_rate_txt(struct seq_file *m, u8 rate)
{
	static const char * const
	cck_rate[] = {"1M", "2M", "5.5M", "11M"};
	u8 idx = rate - DESC_RATE1M;

	seq_printf(m, " CCK_%-5s", cck_rate[idx]);
}

static void rtw_print_ofdm_rate_txt(struct seq_file *m, u8 rate)
{
	static const char * const
	ofdm_rate[] = {"6M", "9M", "12M", "18M", "24M", "36M", "48M", "54M"};
	u8 idx = rate - DESC_RATE6M;

	seq_printf(m, " OFDM_%-4s", ofdm_rate[idx]);
}

static void rtw_print_ht_rate_txt(struct seq_file *m, u8 rate)
{
	u8 mcs_n = rate - DESC_RATEMCS0;

	seq_printf(m, " MCS%-6u", mcs_n);
}

static void rtw_print_vht_rate_txt(struct seq_file *m, u8 rate)
{
	u8 idx = rate - DESC_RATEVHT1SS_MCS0;
	u8 n_ss, mcs_n;

	/* n spatial stream */
	n_ss = 1 + idx / 10;
	/* MCS n */
	mcs_n = idx % 10;
	seq_printf(m, " VHT%uSMCS%u", n_ss, mcs_n);
}

static void rtw_print_rate(struct seq_file *m, u8 rate)
{
	switch (rate) {
	case DESC_RATE1M...DESC_RATE11M:
		rtw_print_cck_rate_txt(m, rate);
		break;
	case DESC_RATE6M...DESC_RATE54M:
		rtw_print_ofdm_rate_txt(m, rate);
		break;
	case DESC_RATEMCS0...DESC_RATEMCS15:
		rtw_print_ht_rate_txt(m, rate);
		break;
	case DESC_RATEVHT1SS_MCS0...DESC_RATEVHT2SS_MCS9:
		rtw_print_vht_rate_txt(m, rate);
		break;
	default:
		seq_printf(m, " Unknown rate=0x%x\n", rate);
		break;
	}
}

#define case_REGD(src) \
	case RTW_REGD_##src: return #src

static const char *rtw_get_regd_string(u8 regd)
{
	switch (regd) {
	case_REGD(FCC);
	case_REGD(MKK);
	case_REGD(ETSI);
	case_REGD(IC);
	case_REGD(KCC);
	case_REGD(ACMA);
	case_REGD(CHILE);
	case_REGD(UKRAINE);
	case_REGD(MEXICO);
	case_REGD(CN);
	case_REGD(WW);
	default:
		return "Unknown";
	}
}

static int rtw_debugfs_get_tx_pwr_tbl(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_hal *hal = &rtwdev->hal;
	u8 path, rate, bw, ch, regd;
	struct rtw_power_params pwr_param = {0};

	mutex_lock(&rtwdev->mutex);
	bw = hal->current_band_width;
	ch = hal->current_channel;
	regd = rtw_regd_get(rtwdev);

	seq_printf(m, "channel: %u\n", ch);
	seq_printf(m, "bandwidth: %u\n", bw);
	seq_printf(m, "regulatory: %s\n", rtw_get_regd_string(regd));
	seq_printf(m, "%-4s %-10s %-9s %-9s (%-4s %-4s %-4s) %-4s\n",
		   "path", "rate", "pwr", "base", "byr", "lmt", "sar", "rem");

	mutex_lock(&hal->tx_power_mutex);
	for (path = RF_PATH_A; path <= RF_PATH_B; path++) {
		/* there is no CCK rates used in 5G */
		if (hal->current_band_type == RTW_BAND_5G)
			rate = DESC_RATE6M;
		else
			rate = DESC_RATE1M;

		/* now, not support vht 3ss and vht 4ss*/
		for (; rate <= DESC_RATEVHT2SS_MCS9; rate++) {
			/* now, not support ht 3ss and ht 4ss*/
			if (rate > DESC_RATEMCS15 &&
			    rate < DESC_RATEVHT1SS_MCS0)
				continue;

			rtw_get_tx_power_params(rtwdev, path, rate, bw,
						ch, regd, &pwr_param);

			seq_printf(m, "%4c ", path + 'A');
			rtw_print_rate(m, rate);
			seq_printf(m, " %3u(0x%02x) %4u %4d (%4d %4d %4d) %4d\n",
				   hal->tx_pwr_tbl[path][rate],
				   hal->tx_pwr_tbl[path][rate],
				   pwr_param.pwr_base,
				   min3(pwr_param.pwr_offset,
					pwr_param.pwr_limit,
					pwr_param.pwr_sar),
				   pwr_param.pwr_offset, pwr_param.pwr_limit,
				   pwr_param.pwr_sar,
				   pwr_param.pwr_remnant);
		}
	}

	mutex_unlock(&hal->tx_power_mutex);
	mutex_unlock(&rtwdev->mutex);

	return 0;
}

void rtw_debugfs_get_simple_phy_info(struct seq_file *m)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_hal *hal = &rtwdev->hal;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_traffic_stats *stats = &rtwdev->stats;

	seq_printf(m, "%-40s = %ddBm/ %d\n", "RSSI/ STA Channel",
		   dm_info->rssi[RF_PATH_A] - 100, hal->current_channel);

	seq_printf(m, "TP {Tx, Rx} = {%u, %u}Mbps\n",
		   stats->tx_throughput, stats->rx_throughput);

	seq_puts(m, "[Tx Rate] = ");
	rtw_print_rate(m, dm_info->tx_rate);
	seq_printf(m, "(0x%x)\n", dm_info->tx_rate);

	seq_puts(m, "[Rx Rate] = ");
	rtw_print_rate(m, dm_info->curr_rx_rate);
	seq_printf(m, "(0x%x)\n", dm_info->curr_rx_rate);
}

static int rtw_debugfs_get_phy_info(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_traffic_stats *stats = &rtwdev->stats;
	struct rtw_pkt_count *last_cnt = &dm_info->last_pkt_count;
	struct rtw_efuse *efuse = &rtwdev->efuse;
	struct ewma_evm *ewma_evm = dm_info->ewma_evm;
	struct ewma_snr *ewma_snr = dm_info->ewma_snr;
	u8 ss, rate_id;

	seq_puts(m, "==========[Common Info]========\n");
	seq_printf(m, "Is link = %c\n", rtw_is_assoc(rtwdev) ? 'Y' : 'N');
	seq_printf(m, "Current CH(fc) = %u\n", rtwdev->hal.current_channel);
	seq_printf(m, "Current BW = %u\n", rtwdev->hal.current_band_width);
	seq_printf(m, "Current IGI = 0x%x\n", dm_info->igi_history[0]);
	seq_printf(m, "TP {Tx, Rx} = {%u, %u}Mbps\n",
		   stats->tx_throughput, stats->rx_throughput);
	seq_printf(m, "1SS for TX and RX = %c\n\n", rtwdev->hal.txrx_1ss ?
		   'Y' : 'N');

	seq_puts(m, "==========[Tx Phy Info]========\n");
	seq_puts(m, "[Tx Rate] = ");
	rtw_print_rate(m, dm_info->tx_rate);
	seq_printf(m, "(0x%x)\n\n", dm_info->tx_rate);

	seq_puts(m, "==========[Rx Phy Info]========\n");
	seq_printf(m, "[Rx Beacon Count] = %u\n", last_cnt->num_bcn_pkt);
	seq_puts(m, "[Rx Rate] = ");
	rtw_print_rate(m, dm_info->curr_rx_rate);
	seq_printf(m, "(0x%x)\n", dm_info->curr_rx_rate);

	seq_puts(m, "[Rx Rate Count]:\n");
	seq_printf(m, " * CCK = {%u, %u, %u, %u}\n",
		   last_cnt->num_qry_pkt[DESC_RATE1M],
		   last_cnt->num_qry_pkt[DESC_RATE2M],
		   last_cnt->num_qry_pkt[DESC_RATE5_5M],
		   last_cnt->num_qry_pkt[DESC_RATE11M]);

	seq_printf(m, " * OFDM = {%u, %u, %u, %u, %u, %u, %u, %u}\n",
		   last_cnt->num_qry_pkt[DESC_RATE6M],
		   last_cnt->num_qry_pkt[DESC_RATE9M],
		   last_cnt->num_qry_pkt[DESC_RATE12M],
		   last_cnt->num_qry_pkt[DESC_RATE18M],
		   last_cnt->num_qry_pkt[DESC_RATE24M],
		   last_cnt->num_qry_pkt[DESC_RATE36M],
		   last_cnt->num_qry_pkt[DESC_RATE48M],
		   last_cnt->num_qry_pkt[DESC_RATE54M]);

	for (ss = 0; ss < efuse->hw_cap.nss; ss++) {
		rate_id = DESC_RATEMCS0 + ss * 8;
		seq_printf(m, " * HT_MCS[%u:%u] = {%u, %u, %u, %u, %u, %u, %u, %u}\n",
			   ss * 8, ss * 8 + 7,
			   last_cnt->num_qry_pkt[rate_id],
			   last_cnt->num_qry_pkt[rate_id + 1],
			   last_cnt->num_qry_pkt[rate_id + 2],
			   last_cnt->num_qry_pkt[rate_id + 3],
			   last_cnt->num_qry_pkt[rate_id + 4],
			   last_cnt->num_qry_pkt[rate_id + 5],
			   last_cnt->num_qry_pkt[rate_id + 6],
			   last_cnt->num_qry_pkt[rate_id + 7]);
	}

	for (ss = 0; ss < efuse->hw_cap.nss; ss++) {
		rate_id = DESC_RATEVHT1SS_MCS0 + ss * 10;
		seq_printf(m, " * VHT_MCS-%uss MCS[0:9] = {%u, %u, %u, %u, %u, %u, %u, %u, %u, %u}\n",
			   ss + 1,
			   last_cnt->num_qry_pkt[rate_id],
			   last_cnt->num_qry_pkt[rate_id + 1],
			   last_cnt->num_qry_pkt[rate_id + 2],
			   last_cnt->num_qry_pkt[rate_id + 3],
			   last_cnt->num_qry_pkt[rate_id + 4],
			   last_cnt->num_qry_pkt[rate_id + 5],
			   last_cnt->num_qry_pkt[rate_id + 6],
			   last_cnt->num_qry_pkt[rate_id + 7],
			   last_cnt->num_qry_pkt[rate_id + 8],
			   last_cnt->num_qry_pkt[rate_id + 9]);
	}

	seq_printf(m, "[RSSI(dBm)] = {%d, %d}\n",
		   dm_info->rssi[RF_PATH_A] - 100,
		   dm_info->rssi[RF_PATH_B] - 100);
	seq_printf(m, "[Rx EVM(dB)] = {-%d, -%d}\n",
		   dm_info->rx_evm_dbm[RF_PATH_A],
		   dm_info->rx_evm_dbm[RF_PATH_B]);
	seq_printf(m, "[Rx SNR] = {%d, %d}\n",
		   dm_info->rx_snr[RF_PATH_A],
		   dm_info->rx_snr[RF_PATH_B]);
	seq_printf(m, "[CFO_tail(KHz)] = {%d, %d}\n",
		   dm_info->cfo_tail[RF_PATH_A],
		   dm_info->cfo_tail[RF_PATH_B]);

	if (dm_info->curr_rx_rate >= DESC_RATE11M) {
		seq_puts(m, "[Rx Average Status]:\n");
		seq_printf(m, " * OFDM, EVM: {-%d}, SNR: {%d}\n",
			   (u8)ewma_evm_read(&ewma_evm[RTW_EVM_OFDM]),
			   (u8)ewma_snr_read(&ewma_snr[RTW_SNR_OFDM_A]));
		seq_printf(m, " * 1SS, EVM: {-%d}, SNR: {%d}\n",
			   (u8)ewma_evm_read(&ewma_evm[RTW_EVM_1SS]),
			   (u8)ewma_snr_read(&ewma_snr[RTW_SNR_1SS_A]));
		seq_printf(m, " * 2SS, EVM: {-%d, -%d}, SNR: {%d, %d}\n",
			   (u8)ewma_evm_read(&ewma_evm[RTW_EVM_2SS_A]),
			   (u8)ewma_evm_read(&ewma_evm[RTW_EVM_2SS_B]),
			   (u8)ewma_snr_read(&ewma_snr[RTW_SNR_2SS_A]),
			   (u8)ewma_snr_read(&ewma_snr[RTW_SNR_2SS_B]));
	}

	seq_puts(m, "[Rx Counter]:\n");
	seq_printf(m, " * CCA (CCK, OFDM, Total) = (%u, %u, %u)\n",
		   dm_info->cck_cca_cnt,
		   dm_info->ofdm_cca_cnt,
		   dm_info->total_cca_cnt);
	seq_printf(m, " * False Alarm (CCK, OFDM, Total) = (%u, %u, %u)\n",
		   dm_info->cck_fa_cnt,
		   dm_info->ofdm_fa_cnt,
		   dm_info->total_fa_cnt);
	seq_printf(m, " * CCK cnt (ok, err) = (%u, %u)\n",
		   dm_info->cck_ok_cnt, dm_info->cck_err_cnt);
	seq_printf(m, " * OFDM cnt (ok, err) = (%u, %u)\n",
		   dm_info->ofdm_ok_cnt, dm_info->ofdm_err_cnt);
	seq_printf(m, " * HT cnt (ok, err) = (%u, %u)\n",
		   dm_info->ht_ok_cnt, dm_info->ht_err_cnt);
	seq_printf(m, " * VHT cnt (ok, err) = (%u, %u)\n",
		   dm_info->vht_ok_cnt, dm_info->vht_err_cnt);

	return 0;
}

static int rtw_debugfs_get_coex_info(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;

	mutex_lock(&rtwdev->mutex);
	rtw_coex_display_coex_info(rtwdev, m);
	mutex_unlock(&rtwdev->mutex);

	return 0;
}

static ssize_t rtw_debugfs_set_coex_enable(struct file *filp,
					   const char __user *buffer,
					   size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;
	bool enable;
	int ret;

	ret = kstrtobool_from_user(buffer, count, &enable);
	if (ret)
		return ret;

	mutex_lock(&rtwdev->mutex);
	coex->manual_control = !enable;
	mutex_unlock(&rtwdev->mutex);

	return count;
}

static int rtw_debugfs_get_coex_enable(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_coex *coex = &rtwdev->coex;

	seq_printf(m, "coex mechanism %s\n",
		   coex->manual_control ? "disabled" : "enabled");

	return 0;
}

static ssize_t rtw_debugfs_set_edcca_enable(struct file *filp,
					    const char __user *buffer,
					    size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	bool input;
	int err;

	err = kstrtobool_from_user(buffer, count, &input);
	if (err)
		return err;

	rtw_edcca_enabled = input;
	rtw_phy_adaptivity_set_mode(rtwdev);

	return count;
}

static int rtw_debugfs_get_edcca_enable(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;

	seq_printf(m, "EDCCA %s: EDCCA mode %d\n",
		   rtw_edcca_enabled ? "enabled" : "disabled",
		   dm_info->edcca_mode);
	return 0;
}

static ssize_t rtw_debugfs_set_fw_crash(struct file *filp,
					const char __user *buffer,
					size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	bool input;
	int ret;

	ret = kstrtobool_from_user(buffer, count, &input);
	if (ret)
		return ret;

	if (!input)
		return -EINVAL;

	if (test_bit(RTW_FLAG_RESTARTING, rtwdev->flags))
		return -EINPROGRESS;

	mutex_lock(&rtwdev->mutex);
	rtw_leave_lps_deep(rtwdev);
	set_bit(RTW_FLAG_RESTART_TRIGGERING, rtwdev->flags);
	rtw_write8(rtwdev, REG_HRCV_MSG, 1);
	mutex_unlock(&rtwdev->mutex);

	return count;
}

static int rtw_debugfs_get_fw_crash(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;

	seq_printf(m, "%d\n",
		   test_bit(RTW_FLAG_RESTART_TRIGGERING, rtwdev->flags) ||
		   test_bit(RTW_FLAG_RESTARTING, rtwdev->flags));
	return 0;
}

static ssize_t rtw_debugfs_set_force_lowest_basic_rate(struct file *filp,
						       const char __user *buffer,
						       size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	bool input;
	int err;

	err = kstrtobool_from_user(buffer, count, &input);
	if (err)
		return err;

	if (input)
		set_bit(RTW_FLAG_FORCE_LOWEST_RATE, rtwdev->flags);
	else
		clear_bit(RTW_FLAG_FORCE_LOWEST_RATE, rtwdev->flags);

	return count;
}

static int rtw_debugfs_get_force_lowest_basic_rate(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;

	seq_printf(m, "force lowest basic rate: %d\n",
		   test_bit(RTW_FLAG_FORCE_LOWEST_RATE, rtwdev->flags));

	return 0;
}

static ssize_t rtw_debugfs_set_dm_cap(struct file *filp,
				      const char __user *buffer,
				      size_t count, loff_t *loff)
{
	struct seq_file *seqpriv = (struct seq_file *)filp->private_data;
	struct rtw_debugfs_priv *debugfs_priv = seqpriv->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	int ret, bit;
	bool en;

	ret = kstrtoint_from_user(buffer, count, 10, &bit);
	if (ret)
		return ret;

	en = bit > 0;
	bit = abs(bit);

	if (bit >= RTW_DM_CAP_NUM) {
		rtw_warn(rtwdev, "unknown DM CAP %d\n", bit);
		return -EINVAL;
	}

	if (en)
		dm_info->dm_flags &= ~BIT(bit);
	else
		dm_info->dm_flags |= BIT(bit);

	debugfs_priv->dm_cap.bit = bit;

	return count;
}

static void dump_gapk_status(struct rtw_dev *rtwdev, struct seq_file *m)
{
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	struct rtw_gapk_info *txgapk = &rtwdev->dm_info.gapk;
	int i, path;
	u32 val;

	seq_printf(m, "\n(%2d) %c%s\n\n", RTW_DM_CAP_TXGAPK,
		   dm_info->dm_flags & BIT(RTW_DM_CAP_TXGAPK) ? '-' : '+',
		   rtw_dm_cap_strs[RTW_DM_CAP_TXGAPK]);

	mutex_lock(&rtwdev->mutex);

	for (path = 0; path < rtwdev->hal.rf_path_num; path++) {
		val = rtw_read_rf(rtwdev, path, RF_GAINTX, RFREG_MASK);
		seq_printf(m, "path %d:\n0x%x = 0x%x\n", path, RF_GAINTX, val);

		for (i = 0; i < RF_HW_OFFSET_NUM; i++)
			seq_printf(m, "[TXGAPK] offset %d %d\n",
				   txgapk->rf3f_fs[path][i], i);
		seq_puts(m, "\n");
	}
	mutex_unlock(&rtwdev->mutex);
}

static int rtw_debugfs_get_dm_cap(struct seq_file *m, void *v)
{
	struct rtw_debugfs_priv *debugfs_priv = m->private;
	struct rtw_dev *rtwdev = debugfs_priv->rtwdev;
	struct rtw_dm_info *dm_info = &rtwdev->dm_info;
	int i;

	switch (debugfs_priv->dm_cap.bit) {
	case RTW_DM_CAP_TXGAPK:
		dump_gapk_status(rtwdev, m);
		break;
	default:
		for (i = 1; i < RTW_DM_CAP_NUM; i++) {
			seq_printf(m, "(%2d) %c%s\n", i,
				   dm_info->dm_flags & BIT(i) ? '-' : '+',
				   rtw_dm_cap_strs[i]);
		}
		break;
	}
	debugfs_priv->dm_cap.bit = RTW_DM_CAP_NA;
	return 0;
}

#define rtw_debug_priv_mac(addr)				\
{								\
	.cb_read = rtw_debug_get_mac_page,			\
	.cb_data = addr,					\
}

#define rtw_debug_priv_bb(addr)					\
{								\
	.cb_read = rtw_debug_get_bb_page,			\
	.cb_data = addr,					\
}

#define rtw_debug_priv_get(name)				\
{								\
	.cb_read = rtw_debugfs_get_ ##name,			\
}

#define rtw_debug_priv_set(name)				\
{								\
	.cb_write = rtw_debugfs_set_ ##name,			\
}

#define rtw_debug_priv_set_and_get(name)			\
{								\
	.cb_write = rtw_debugfs_set_ ##name,			\
	.cb_read = rtw_debugfs_get_ ##name,			\
}

#define rtw_debug_priv_set_single_and_get(name)			\
{								\
	.cb_write = rtw_debugfs_set_single_input,		\
	.cb_read = rtw_debugfs_get_ ##name,			\
}

static const struct rtw_debugfs rtw_debugfs_templ = {
	.mac_0 = rtw_debug_priv_mac(0x0000),
	.mac_1 = rtw_debug_priv_mac(0x0100),
	.mac_2 = rtw_debug_priv_mac(0x0200),
	.mac_3 = rtw_debug_priv_mac(0x0300),
	.mac_4 = rtw_debug_priv_mac(0x0400),
	.mac_5 = rtw_debug_priv_mac(0x0500),
	.mac_6 = rtw_debug_priv_mac(0x0600),
	.mac_7 = rtw_debug_priv_mac(0x0700),
	.mac_10 = rtw_debug_priv_mac(0x1000),
	.mac_11 = rtw_debug_priv_mac(0x1100),
	.mac_12 = rtw_debug_priv_mac(0x1200),
	.mac_13 = rtw_debug_priv_mac(0x1300),
	.mac_14 = rtw_debug_priv_mac(0x1400),
	.mac_15 = rtw_debug_priv_mac(0x1500),
	.mac_16 = rtw_debug_priv_mac(0x1600),
	.mac_17 = rtw_debug_priv_mac(0x1700),
	.bb_8 = rtw_debug_priv_bb(0x0800),
	.bb_9 = rtw_debug_priv_bb(0x0900),
	.bb_a = rtw_debug_priv_bb(0x0a00),
	.bb_b = rtw_debug_priv_bb(0x0b00),
	.bb_c = rtw_debug_priv_bb(0x0c00),
	.bb_d = rtw_debug_priv_bb(0x0d00),
	.bb_e = rtw_debug_priv_bb(0x0e00),
	.bb_f = rtw_debug_priv_bb(0x0f00),
	.bb_18 = rtw_debug_priv_bb(0x1800),
	.bb_19 = rtw_debug_priv_bb(0x1900),
	.bb_1a = rtw_debug_priv_bb(0x1a00),
	.bb_1b = rtw_debug_priv_bb(0x1b00),
	.bb_1c = rtw_debug_priv_bb(0x1c00),
	.bb_1d = rtw_debug_priv_bb(0x1d00),
	.bb_1e = rtw_debug_priv_bb(0x1e00),
	.bb_1f = rtw_debug_priv_bb(0x1f00),
	.bb_2c = rtw_debug_priv_bb(0x2c00),
	.bb_2d = rtw_debug_priv_bb(0x2d00),
	.bb_40 = rtw_debug_priv_bb(0x4000),
	.bb_41 = rtw_debug_priv_bb(0x4100),
	.rf_dump = rtw_debug_priv_get(rf_dump),
	.tx_pwr_tbl = rtw_debug_priv_get(tx_pwr_tbl),
	.write_reg = rtw_debug_priv_set(write_reg),
	.h2c = rtw_debug_priv_set(h2c),
	.rf_write = rtw_debug_priv_set(rf_write),
	.rf_read = rtw_debug_priv_set_and_get(rf_read),
	.read_reg = rtw_debug_priv_set_and_get(read_reg),
	.fix_rate = rtw_debug_priv_set_and_get(fix_rate),
	.dump_cam = rtw_debug_priv_set_single_and_get(dump_cam),
	.rsvd_page = rtw_debug_priv_set_and_get(rsvd_page),
	.phy_info = rtw_debug_priv_get(phy_info),
	.coex_enable = rtw_debug_priv_set_and_get(coex_enable),
	.coex_info = rtw_debug_priv_get(coex_info),
	.edcca_enable = rtw_debug_priv_set_and_get(edcca_enable),
	.fw_crash = rtw_debug_priv_set_and_get(fw_crash),
	.force_lowest_basic_rate = rtw_debug_priv_set_and_get(force_lowest_basic_rate),
	.dm_cap = rtw_debug_priv_set_and_get(dm_cap),
};

#define rtw_debugfs_add_core(name, mode, fopname, parent)		\
	do {								\
		struct rtw_debugfs_priv *priv = &rtwdev->debugfs->name;	\
		priv->rtwdev = rtwdev;					\
		if (IS_ERR(debugfs_create_file(#name, mode,		\
					 parent, priv,			\
					 &file_ops_ ##fopname)))	\
			pr_debug("Unable to initialize debugfs:%s\n",	\
			       #name);					\
	} while (0)

#define rtw_debugfs_add_w(name)						\
	rtw_debugfs_add_core(name, S_IFREG | 0222, common_write, debugfs_topdir)
#define rtw_debugfs_add_rw(name)					\
	rtw_debugfs_add_core(name, S_IFREG | 0666, single_rw, debugfs_topdir)
#define rtw_debugfs_add_r(name)						\
	rtw_debugfs_add_core(name, S_IFREG | 0444, single_r, debugfs_topdir)

static
void rtw_debugfs_add_basic(struct rtw_dev *rtwdev, struct dentry *debugfs_topdir)
{
	rtw_debugfs_add_w(write_reg);
	rtw_debugfs_add_rw(read_reg);
	rtw_debugfs_add_w(rf_write);
	rtw_debugfs_add_rw(rf_read);
	rtw_debugfs_add_rw(fix_rate);
	rtw_debugfs_add_rw(dump_cam);
	rtw_debugfs_add_rw(rsvd_page);
	rtw_debugfs_add_r(phy_info);
	rtw_debugfs_add_r(coex_info);
	rtw_debugfs_add_rw(coex_enable);
	rtw_debugfs_add_w(h2c);
	rtw_debugfs_add_r(rf_dump);
	rtw_debugfs_add_r(tx_pwr_tbl);
	rtw_debugfs_add_rw(edcca_enable);
	rtw_debugfs_add_rw(fw_crash);
	rtw_debugfs_add_rw(force_lowest_basic_rate);
	rtw_debugfs_add_rw(dm_cap);
}

static
void rtw_debugfs_add_sec0(struct rtw_dev *rtwdev, struct dentry *debugfs_topdir)
{
	rtw_debugfs_add_r(mac_0);
	rtw_debugfs_add_r(mac_1);
	rtw_debugfs_add_r(mac_2);
	rtw_debugfs_add_r(mac_3);
	rtw_debugfs_add_r(mac_4);
	rtw_debugfs_add_r(mac_5);
	rtw_debugfs_add_r(mac_6);
	rtw_debugfs_add_r(mac_7);
	rtw_debugfs_add_r(bb_8);
	rtw_debugfs_add_r(bb_9);
	rtw_debugfs_add_r(bb_a);
	rtw_debugfs_add_r(bb_b);
	rtw_debugfs_add_r(bb_c);
	rtw_debugfs_add_r(bb_d);
	rtw_debugfs_add_r(bb_e);
	rtw_debugfs_add_r(bb_f);
}

static
void rtw_debugfs_add_sec1(struct rtw_dev *rtwdev, struct dentry *debugfs_topdir)
{
	rtw_debugfs_add_r(mac_10);
	rtw_debugfs_add_r(mac_11);
	rtw_debugfs_add_r(mac_12);
	rtw_debugfs_add_r(mac_13);
	rtw_debugfs_add_r(mac_14);
	rtw_debugfs_add_r(mac_15);
	rtw_debugfs_add_r(mac_16);
	rtw_debugfs_add_r(mac_17);
	rtw_debugfs_add_r(bb_18);
	rtw_debugfs_add_r(bb_19);
	rtw_debugfs_add_r(bb_1a);
	rtw_debugfs_add_r(bb_1b);
	rtw_debugfs_add_r(bb_1c);
	rtw_debugfs_add_r(bb_1d);
	rtw_debugfs_add_r(bb_1e);
	rtw_debugfs_add_r(bb_1f);
	if (rtwdev->chip->id == RTW_CHIP_TYPE_8822C) {
		rtw_debugfs_add_r(bb_2c);
		rtw_debugfs_add_r(bb_2d);
		rtw_debugfs_add_r(bb_40);
		rtw_debugfs_add_r(bb_41);
	}
}

void rtw_debugfs_init(struct rtw_dev *rtwdev)
{
	struct dentry *debugfs_topdir;

	rtwdev->debugfs = kmemdup(&rtw_debugfs_templ, sizeof(rtw_debugfs_templ),
				  GFP_KERNEL);
	if (!rtwdev->debugfs)
		return;

	debugfs_topdir = debugfs_create_dir("rtw88",
					    rtwdev->hw->wiphy->debugfsdir);

	rtw_debugfs_add_basic(rtwdev, debugfs_topdir);
	rtw_debugfs_add_sec0(rtwdev, debugfs_topdir);
	rtw_debugfs_add_sec1(rtwdev, debugfs_topdir);
}

void rtw_debugfs_deinit(struct rtw_dev *rtwdev)
{
	kfree(rtwdev->debugfs);
}
#endif /* CPTCFG_RTW88_DEBUGFS */

#ifdef CPTCFG_RTW88_DEBUG

void rtw_dbg(struct rtw_dev *rtwdev, enum rtw_debug_mask mask,
	     const char *fmt, ...)
{
	struct va_format vaf = {
		.fmt = fmt,
	};
	va_list args;

	va_start(args, fmt);
	vaf.va = &args;

	if (rtw_debug_mask & mask)
		dev_printk(KERN_DEBUG, rtwdev->dev, "%pV", &vaf);

	va_end(args);
}
EXPORT_SYMBOL(rtw_dbg);

#endif /* CPTCFG_RTW88_DEBUG */
