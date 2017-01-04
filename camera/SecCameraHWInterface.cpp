/*
 * Copyright (C) 2008 The Android Open Source Project
 *           (C) 2010 Samsung Electronics Co. LTD
 *           (C) 2017 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "SecCameraHardware"
#include <utils/Log.h>

#include <hardware/camera.h>

#include "SecCameraHWInterface.h"

#include <errno.h>
#include <stdlib.h>

namespace android {

static camera_device_t *g_cam_device = NULL;

static inline CameraHardwareSec *obj(struct camera_device *dev) {
    return reinterpret_cast<CameraHardwareSec *>(dev->priv);
}

static int HAL_camera_device_close(struct hw_device_t *device) {
    ALOGV("%s", __func__);

    if (device) {
        camera_device_t *cam_device = (camera_device_t *)device;
        delete static_cast<CameraHardwareSec *>cam_device->priv;
        free(cam_device);
        g_cam_device = NULL;
    }
    return 0;
}

// wrappers around the actual HAL implementation
static int HAL_getNumberOfCameras() {
    ALOGV("%s", __func__);

    // TODO why don't we do this already? does it actually work?
    // it seems to be what the stock HAL does

    int cam_fd;
    static struct v4l2_input input;

    cam_fd = open(CAMERA_DEV_NAME, O_RDONLY);
    if (cam_fd < 0) {
        ALOGE("%s: Failed to open %s (%d)", __func__, CAMERA_DEV_NAME, cam_fd);
        return -1;
    }

    input.index = 0;
    while (ioctl(cam_fd, VIDIOC_ENUMINPUT, &input) == 0) {
        ALOGI("%s: input channel %d => '%s'", __func__, input.index, input.name);
        input.index++;
    }
    ALOGE("%d", input.index);
    close(cam_fd);
    return --input.index;
}

static int HAL_getCameraInfo(int cameraId, struct camera_info *cameraInfo) {
    ALOGV("%s", __func__);
    // TODO
    return 0;
}


static int HAL_camera_device_open(const struct hw_module_t *module,
        const char *id, struct hw_device_t **device) {
    ALOGV("%s", __func__);

    int cameraId = atoi(id);
    if (cameraId < 0 || cameraId >= HAL_getNumberOfCameras()) {
        ALOGE("Invalid camera ID %d", cameraId);
        return -EINVAL;
    }

    if (g_cam_device) {
        if (obj(g_cam_device)->getCameraId() == cameraId) {
            ALOGV("Camera %d already open, not re-opening", cameraId);
            goto done;
        } else {
            ALOGE("Can't open camera %d, camera %d already open!", cameraId,
                    obj(g_cam_device)->getCameraDevice());
            return -ENOSYS;
        }
    }

    g_cam_device = (camera_device_t *)malloc(sizeof(camera_device_t));
    if (!g_cam_device)
        return -ENOMEM;

    g_cam_device->common.tag = HARDWARE_DEVICE_TAG;
    g_cam_device->common.version = 1;
    g_cam_device->common.module = const_cast<hw_module_t *>(module);
    g_cam_device->common.close = HAL_camera_device_close;

    // FIXME
    //g_cam_device->ops = &camera_device_ops;

    ALOGI("%s: opening camera %d", __func__, cameraId);

    //g_cam_device->priv = new CameraHardwareSec(cameraId, g_cam_device);

done:
    *device = (hw_device_t*)g_cam_device;
    ALOGI("%s: opened camera %s (%p)", __func__, id, *device);
    return 0;
}



static hw_module_methods_t camera_module_methods = {
    .open = HAL_camera_device_open,
};

extern "C" {
    struct camera_module HAL_MODULE_INFO_SYM = {
        .common = {
            .tag = HARDWARE_MODULE_TAG,
            .version_major = 1,
            .version_minor = 0,
            .id = CAMERA_HARDWARE_MODULE_ID,
            .name = "smdk4x12 camera HAL",
            .author = "The LineageOS Project",
            .methods = &camera_module_methods,
        },
        .get_number_of_cameras = &HAL_getNumberOfCameras,
        .get_camera_info = &HAL_getCameraInfo,

} // namespace android
