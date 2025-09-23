// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires

#include <command.h>
#include <device-mapper.h>
#include <libfile.h>
#include <stdio.h>

static int veritysetup_dump(int argc, char *argv[])
{
	char *config;

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	config = dm_verity_config_from_sb("<data-dev>", argv[0], "<root-hash>");
	if (IS_ERR(config)) {
		printf("Invalid or missing superblock: %pe\n", config);
		return COMMAND_ERROR;
	}

	puts(config);
	free(config);
	return COMMAND_SUCCESS;
}

static struct dm_device *veritysetup_find(const char *name)
{
	struct dm_device *dm;

	dm = dm_find_by_name(name);
	if (IS_ERR_OR_NULL(dm)) {
		printf("Found no device named \"%s\"\n", name);
		return NULL;
	}

	return dm;
}

static int veritysetup_close(int argc, char *argv[])
{
	struct dm_device *dm;

	if (argc != 1)
		return COMMAND_ERROR_USAGE;

	dm = veritysetup_find(argv[0]);
	if (!dm)
		return COMMAND_ERROR;

	dm_destroy(dm);

	printf("Removed %s\n", argv[0]);
	return COMMAND_SUCCESS;
}

static int veritysetup_open(int argc, char *argv[])
{
	struct dm_device *dm;
	char *config;

	if (argc != 4)
		return COMMAND_ERROR_USAGE;

	config = dm_verity_config_from_sb(argv[0], argv[2], argv[3]);
	if (IS_ERR(config)) {
		printf("Invalid or missing superblock: %pe\n", config);
		return COMMAND_ERROR;
	}

	dm = dm_create(argv[1], config);
	free(config);
	if (IS_ERR_OR_NULL(dm)) {
		printf("Failed to create %s: %pe\n", argv[1], dm);
		return COMMAND_ERROR;
	}

	printf("Created %s\n", argv[1]);
	return COMMAND_SUCCESS;
}

#if defined(CONFIG_DM_BLK_VERITY_SIG)
static int veritysetup_dps_open(int argc, char *argv[])
{
	char *config, *errmsg;
	struct dm_device *dm;

	switch (argc) {
	case 2:
		config = dm_verity_config_from_dps(argv[0], NULL, NULL, &errmsg);
		break;
	case 4:
		config = dm_verity_config_from_dps(argv[0], argv[2], argv[3], &errmsg);
		break;
	default:
		return COMMAND_ERROR_USAGE;
	}

	if (IS_ERR(config)) {
		puts(errmsg); putchar('\n');
		free(errmsg);
		return COMMAND_ERROR;
	}

	dm = dm_create(argv[1], config);
	free(config);
	if (IS_ERR_OR_NULL(dm)) {
		printf("Failed to create %s: %pe\n", argv[1], dm);
		return COMMAND_ERROR;
	}

	printf("Created %s\n", argv[1]);
	return COMMAND_SUCCESS;
}
#else
static int veritysetup_dps_open(int argc, char *argv[])
{
	printf("CONFIG_DM_BLK_VERITY_SIG not enabled\n");
	return COMMAND_ERROR;
}
#endif

static int do_veritysetup(int argc, char *argv[])
{
	const char *cmd;

	if (argc < 2)
		return COMMAND_ERROR_USAGE;

	cmd = argv[1];
	argc -= 2;
	argv += 2;

	if (!strcmp(cmd, "open"))
		return veritysetup_open(argc, argv);
	else if (!strcmp(cmd, "dps-open"))
		return veritysetup_dps_open(argc, argv);
	else if (!strcmp(cmd, "close"))
		return veritysetup_close(argc, argv);
	else if (!strcmp(cmd, "dump"))
		return veritysetup_dump(argc, argv);

	printf("Unknown command: %s\n", cmd);
	return -EINVAL;
}

BAREBOX_CMD_HELP_START(veritysetup)
BAREBOX_CMD_HELP_TEXT("veritysetup - manage dm-verity volumes")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Layers a transparent integrity layer on top of an existing")
BAREBOX_CMD_HELP_TEXT("device, backed by a Merkle tree whose root hash must be")
BAREBOX_CMD_HELP_TEXT("verified by an externally provided signature")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("commands:")
#if defined(CONFIG_DM_BLK_VERITY_SIG)
BAREBOX_CMD_HELP_OPT("dps-open <data-dev> <name> <hash-dev> <sig-dev>", "Create new device")
#endif
BAREBOX_CMD_HELP_OPT("open <data-dev> <name> <hash-dev> <root-hash>", "Create new device")
BAREBOX_CMD_HELP_OPT("close <name>", "Remove device")
BAREBOX_CMD_HELP_OPT("dump <hash-dev>", "Dump superblock information")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(veritysetup)
	.cmd = do_veritysetup,
	BAREBOX_CMD_DESC("manage dm-verity volumes")
	BAREBOX_CMD_OPTS("<command> [args...]")
	BAREBOX_CMD_GROUP(CMD_GRP_PART)
	BAREBOX_CMD_HELP(cmd_veritysetup_help)
BAREBOX_CMD_END
