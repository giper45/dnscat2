/* driver_upload.h
 * By giper45
 *
 * See LICENSE.md
 *
 * This is a driver that allows to send files to the
 * server
 */

#ifndef __DRIVER_UPLOAD_H__
#define __DRIVER_UPLOAD_H__

#include "libs/buffer.h"
#include "libs/select_group.h"

typedef struct
{
  select_group_t *group;
  buffer_t       *stream;
  buffer_t       *outgoing_data;
  char* file_to_upload;
  NBBOOL  is_shutdown;
} driver_upload_t;

driver_upload_t *driver_upload_create(select_group_t *group, char *file_to_upload);
void           driver_upload_destroy(driver_upload_t *driver);
void           driver_upload_data_received(driver_upload_t* driver, uint8_t *data, size_t length);
uint8_t       *driver_upload_get_outgoing(driver_upload_t *driver, size_t *length, size_t max_length);
void           driver_upload_close(driver_upload_t *driver);



#endif