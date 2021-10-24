/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-26     j       the first version
 */
#ifndef APPLICATIONS_MAILBOX_MSG_H_
#define APPLICATIONS_MAILBOX_MSG_H_

struct msg{
    rt_uint8_t *data_ptr;
    rt_uint32_t data_size;
};

#endif /* APPLICATIONS_MAILBOX_MSG_H_ */
