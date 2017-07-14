/*
 * Copyright (C) 2013 Paul Kocialkowski
 * Copyright (C) 2017 The LineageOS Project
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <malloc.h>
#include <ctype.h>

#include <ion/ion.h>
#include <linux/ion.h>

#define LOG_TAG "exynos_ion"
#include <utils/Log.h>

#include "exynos_camera.h"

int exynos_ion_init(struct exynos_camera *exynos_camera)
{
	exynos_camera->ion_fd = -1;

	return 0;
}

int exynos_ion_open(struct exynos_camera *exynos_camera)
{
	exynos_camera->ion_fd = ion_open();

	return 0;
}

void exynos_ion_close(struct exynos_camera *exynos_camera)
{
	if (exynos_camera->ion_fd >= 0)
		ion_close(exynos_camera->ion_fd);

	exynos_camera->ion_fd = -1;
}

struct exynos_ion_data *exynos_ion_alloc(struct exynos_camera *exynos_camera, int size)
{
	int ret;
	struct exynos_ion_data *data = malloc(sizeof(*data));
	if (data == NULL) {
		goto err;
	}

	ret = ion_alloc(exynos_camera->ion_fd, size, getpagesize(), ION_HEAP_SYSTEM_MASK, 0, &data->hnd);
	if (ret < 0) {
		goto err_malloc;
	}

	ret = ion_share(exynos_camera->ion_fd, data->hnd, &data->fd);
	if (ret < 0) {
		goto err_ion;
	}

	data->size = size;

	return data;
err_ion:
	ion_free(exynos_camera->ion_fd, data->hnd);
err_malloc:
	free(data);
err:
	return NULL;
}

int exynos_ion_free(struct exynos_camera *exynos_camera, struct exynos_ion_data *hnd)
{
	ion_free(exynos_camera->ion_fd, hnd->hnd);
	close(hnd->fd);
	free(hnd);
	return 0;
}
