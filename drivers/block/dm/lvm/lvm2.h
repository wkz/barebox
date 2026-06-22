/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: 2026 Tobias Waldekranz <tobias@waldekranz.com> */

#ifndef _LVM2_H
#define _LVM2_H

#define LVM2_LABEL_ID		"LABELONE"
#define LVM2_LABEL_TYPE		"LVM2 001"
#define LVM2_MDA_MAGIC		" LVM2 x[5A%r0N*>"
#define LVM2_MDA_VERSION	1
#define LVM2_LABEL_SCAN_SECTORS	4
#define LVM2_RAW_LOCN_IGNORED	0x00000001

struct lvm2_label {
	u8     id[8];		/* LVM2_LABEL_ID */
	__le64 sector;		/* Sector number of this label */
	__le32 crc;
	__le32 pv_offset;	/* Byte offset to pv_header within sector */
	u8     type[8];		/* LVM2_LABEL_TYPE */
} __packed;

struct lvm2_area {
	__le64 offset;
	__le64 size;
} __packed;

struct lvm2_pv_header {
	u8     uuid[32];
	__le64 size;

	/* Zero terminated list of data areas, followed by zero
	 * terminated list of metadata areas.
	 */
	struct lvm2_area area[0];
} __packed;

struct lvm2_md_area {
	__le64 offset;		/* Byte offset from start of MDA area */
	__le64 size;		/* Includes trailing NUL */
	__le32 checksum;
	__le32 flags;
} __packed;

struct lvm2_md_header {
	__le32 checksum;
	u8     magic[16];	/* LVM2_MDA_MAGIC */
	__le32 version;		/* LVM2_MDA_VERSION */
	__le64 start;		/* Byte offset of MDA area on device */
	__le64 size;		/* Size of MDA area */

	struct lvm2_md_area area[0];
} __packed;

#endif	/* _LVM2_H */
