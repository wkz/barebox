/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DPS_H
#define __DPS_H

enum dps_part {
	DPS_PART_ROOT,
	DPS_PART_USR,
};

enum dps_type {
	DPS_TYPE_DATA,
	DPS_TYPE_VERITY,
	DPS_TYPE_VERITY_SIG,
};

struct cdev *dps_get_part(struct cdev *disk, enum dps_part part, enum dps_type type);
struct cdev *dps_get_part_from_data(struct cdev *data, enum dps_type type);

#endif	/* __DPS_H */
