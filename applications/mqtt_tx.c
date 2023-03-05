/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-02-06     yuanyu       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <string.h>

#include "aiot_state_api.h"
#include "aiot_sysdep_api.h"
#include "aiot_mqtt_api.h"
#include "aiot_subdev_api.h"
#include "cJSON.h"

#define LOG_TAG    "APP.tx"
#define LOG_LVL    DBG_LOG
#include <rtdbg.h>

#include <param.h>
#include <mqtt_adapter.h>

static struct physio_param param = {0};
static cJSON *payload_json = RT_NULL, *params_json = RT_NULL;

extern rt_mq_t param_mq_handle;

void mqtt_tx_thread(void *parameter)
{
    rt_err_t result;
    char *payload = RT_NULL;
    RT_ASSERT(param_mq_handle != RT_NULL);
    cJSON_AddItemToObject(payload_json, "id", cJSON_CreateString("1"));
    cJSON_AddItemToObject(payload_json, "version", cJSON_CreateString("1.0"));
    params_json = cJSON_CreateObject();
    cJSON_AddItemToObject(params_json, "BodyTemp", cJSON_CreateNumber(param.temperature));
    cJSON_AddItemToObject(params_json, "HeartRate", cJSON_CreateNumber(param.heart_rate));
    cJSON_AddItemToObject(params_json, "SystolicBp", cJSON_CreateNumber(param.systolic));
    cJSON_AddItemToObject(params_json, "DiastolicBp", cJSON_CreateNumber(param.diastolic));
    cJSON_AddItemToObject(params_json, "BloodOxygen", cJSON_CreateNumber(param.blood_oxygen));
    cJSON_AddItemToObject(payload_json, "params", params_json);
    if(mqtt_wrapper.mqtt_connect != RT_NULL)
    {
        result = mqtt_wrapper.mqtt_connect(&mqtt_wrapper);
        if(result != RT_EOK)
        {
            LOG_E("mqtt connect failed: %d", result);
            work_flag = -1;
            return;
        }
    }
    while(1)
    {
        if(rt_mq_recv(param_mq_handle, &param, sizeof(param), RT_WAITING_FOREVER) == RT_EOK)
        {
            LOG_D("node_id: %d", param.node_id);
            LOG_D("systolic: %d", param.systolic);
            LOG_D("diastolic: %d", param.diastolic);
            LOG_D("heart_rate: %d", param.heart_rate);
            LOG_D("blood_oxygen: %d", param.blood_oxygen);
            LOG_D("temperature: %f", param.temperature);
            cJSON_ReplaceItemInObject(params_json, "BodyTemp", cJSON_CreateNumber(param.temperature));
            cJSON_ReplaceItemInObject(params_json, "HeartRate", cJSON_CreateNumber(param.heart_rate));
            cJSON_ReplaceItemInObject(params_json, "SystolicBp", cJSON_CreateNumber(param.systolic));
            cJSON_ReplaceItemInObject(params_json, "DiastolicBp", cJSON_CreateNumber(param.diastolic));
            cJSON_ReplaceItemInObject(params_json, "BloodOxygen", cJSON_CreateNumber(param.blood_oxygen));
            payload = cJSON_PrintUnformatted(payload_json);
            LOG_D("payload: %s", payload);
            if(mqtt_wrapper.mqtt_publish != RT_NULL)
            {
                result = mqtt_wrapper.mqtt_publish(&mqtt_wrapper, "/sys/hcixG5BeXXR/node0/thing/event/property/post", payload, strlen(payload));
                if(result != RT_EOK)
                {
                    LOG_E("mqtt publish failed: %d", result);
                }
            }
        }
    }
}