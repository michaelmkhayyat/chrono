/* =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2019 projectchrono.org
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Asher Elmquist
// =============================================================================
//
// RT kernels for coloring upon ray not intersecting anything
//
// ============================================================================= */

#include "chrono_sensor/optix/shaders/device_utils.h"

extern "C" __global__ void __miss__shader() {
    const MissParameters* miss = (MissParameters*)optixGetSbtDataPointer();

    // figure out type and determine miss parameter for this ray type
    RayType raytype = (RayType)optixGetPayload_2();

    switch (raytype) {
        case CAMERA_RAY_TYPE: {
            const CameraMissParameters& camera_miss = miss->camera_miss;
            PerRayData_camera* prd = getCameraPRD();

            if (camera_miss.mode == BackgroundMode::ENVIRONMENT_MAP && camera_miss.env_map) {
                // evironment map assumes z up
                float3 ray_dir = optixGetWorldRayDirection();
                float theta = atan2f(ray_dir.x, ray_dir.y);
                float phi = asinf(ray_dir.z);
                float tex_x = theta / (2 * CUDART_PI_F);
                float tex_y = phi / CUDART_PI_F + 0.5;
                float4 tex = tex2D<float4>(camera_miss.env_map, tex_x, tex_y);
                prd->color = make_float3(tex.x, tex.y, tex.z) * prd->contrib_to_first_hit;
            } else if (camera_miss.mode == BackgroundMode::GRADIENT) {  // gradient
                // gradient assumes z=up
                float3 ray_dir = optixGetWorldRayDirection();
                float mix = max(0.f, ray_dir.z);
                prd->color = (mix * camera_miss.color_zenith + (1 - mix) * camera_miss.color_horizon) *
                             prd->contrib_to_first_hit;
            } else {  // default to solid color
                prd->color = camera_miss.color_zenith * prd->contrib_to_first_hit;
            }
            break;
        }
        case LIDAR_RAY_TYPE: {
            PerRayData_lidar* prd = getLidarPRD();
            prd->range = 0.f;
            prd->intensity = 0.f;
            break;
        }
        case RADAR_RAY_TYPE: {
            PerRayData_radar* prd = getRadarPRD();
            prd->range = 0.f;
            prd->rcs = 0.f;
            break;
        }
    }
}