// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <dps.h>
#include <driver.h>

#include <spec/dps.h>

static guid_t dps_native[][3] = {
	[DPS_PART_ROOT] = {
		[DPS_TYPE_DATA] = SD_GPT_ROOT_NATIVE,
		[DPS_TYPE_VERITY] = SD_GPT_ROOT_NATIVE_VERITY,
		[DPS_TYPE_VERITY_SIG] = SD_GPT_ROOT_NATIVE_VERITY_SIG,
	},
	[DPS_PART_USR] = {
		[DPS_TYPE_DATA] = SD_GPT_USR_NATIVE,
		[DPS_TYPE_VERITY] = SD_GPT_USR_NATIVE_VERITY,
		[DPS_TYPE_VERITY_SIG] = SD_GPT_USR_NATIVE_VERITY_SIG,
	},
};

struct cdev *dps_get_part(struct cdev *disk, enum dps_part part, enum dps_type type)
{
	guid_t dpsuuid;

	guid_bswap(&dpsuuid, &dps_native[part][type]);

	return cdev_find_child_by_gpt_typeuuid(disk, &dpsuuid);
}

struct cdev *dps_get_part_from_data(struct cdev *data, enum dps_type type)
{
	guid_t dpsuuid;

	if (!data->master || !cdev_is_gpt_partitioned(data->master))
		return ERR_PTR(-EINVAL);

	guid_bswap(&dpsuuid, &dps_native[DPS_PART_ROOT][DPS_TYPE_DATA]);
	if (guid_equal(&data->typeuuid, &dpsuuid))
		return dps_get_part(data->master, DPS_PART_ROOT, type);

	guid_bswap(&dpsuuid, &dps_native[DPS_PART_USR][DPS_TYPE_DATA]);
	if (guid_equal(&data->typeuuid, &dpsuuid))
		return dps_get_part(data->master, DPS_PART_USR, type);

	return ERR_PTR(-EINVAL);
}
