/* driver_upload.c
 * By giper45
 *
 * See LICENSE.md
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <sys/stat.h>
#include "libs/log.h"
#include "libs/memory.h"
#include "libs/select_group.h"
#include "libs/types.h"

#include "driver_upload.h"
#include "command/command_packet.h"

/* I moved some functions into other files for better organization;
 * this includes them. */


#define UPLOAD_LENGTH 8

static command_packet_t *handle_shutdown(driver_upload_t *driver, command_packet_t *in)
{
  if(!in->is_request)
    return NULL;

  controller_kill_all_sessions();

  return command_packet_create_shutdown_response(in->request_id);
}


static command_packet_t *handle_download(driver_upload_t *driver, command_packet_t *in)
{
  struct stat s;
  uint8_t *data;
  FILE *f = NULL;
  command_packet_t *out = NULL;

  if(!in->is_request)
    return NULL;

  if(stat(in->r.request.body.download.filename, &s) != 0)
    return command_packet_create_error_response(in->request_id, -1, "Error opening file for reading");

#ifdef WIN32
  fopen_s(&f, in->r.request.body.download.filename, "rb");
#else
  f = fopen(in->r.request.body.download.filename, "rb");
#endif
  if(!f)
    return command_packet_create_error_response(in->request_id, -1, "Error opening file for reading");

  data = safe_malloc(s.st_size);
  if(fread(data, 1, s.st_size, f) == s.st_size)
    out = command_packet_create_download_response(in->request_id, data, s.st_size);
  else
    out = command_packet_create_error_response(in->request_id, -1, "There was an error reading the file");

  fclose(f);
  safe_free(data);

  return out;
}


void driver_upload_data_received(driver_upload_t* driver, uint8_t *data, size_t length)
{
  command_packet_t *in  = NULL;
  command_packet_t *out = NULL;


  LOG_WARNING("In buyffer add  bytes, uploading");
  buffer_add_bytes(driver->stream, data, length); 

  while((in = command_packet_read(driver->stream)))
  {
    if (in->command_id != COMMAND_DOWNLOAD && in->command_id != COMMAND_SHUTDOWN) 
    {
      LOG_ERROR("NON VALID COMMAND PACKET");

    }
    else if (in->command_id == COMMAND_DOWNLOAD)
    {
      LOG_WARNING("Parse download packet");
      out = handle_download(driver, in);

    } 
    /* SHUTDOWN */
    else if (in->command_id == COMMAND_SHUTDOWN)
    {
      LOG_WARNING("Manage shutdown");
      out = handle_shutdown(driver, in);
    }

      /* Respond if and only if an outgoing packet was created. */
    if(out)
    {
      uint8_t *data;
      size_t   length;

      if(out->command_id != TUNNEL_DATA)
      {
        LOG_WARNING("Response: ");
        command_packet_print(out);
      }

      data = command_packet_to_bytes(out, &length);
      LOG_WARNING("Put outgoing data");
      buffer_add_bytes(driver->outgoing_data, data, length);
      safe_free(data);
      command_packet_destroy(out);
    }
    command_packet_destroy(in);
  }
}
  


uint8_t *driver_upload_get_outgoing(driver_upload_t *driver, size_t *length, size_t max_length)
{
  /* If the driver has been killed and we have no bytes left, return NULL to close the session. */
  if(driver->is_shutdown && buffer_get_remaining_bytes(driver->outgoing_data) == 0)
    return NULL;

  return buffer_read_remaining_bytes(driver->outgoing_data, length, max_length, TRUE);
}

driver_upload_t *driver_upload_create(select_group_t *group, char *file_to_upload)
{
  LOG_WARNING("In driver upload create\n");

  driver_upload_t *driver = (driver_upload_t*) safe_malloc(sizeof(driver_upload_t));
  driver->file_to_upload = file_to_upload;
  driver->stream        = buffer_create(BO_BIG_ENDIAN);
  driver->group         = group;
  driver->is_shutdown   = FALSE;
  driver->outgoing_data = buffer_create(BO_LITTLE_ENDIAN);
  buffer_add_string(driver->outgoing_data, file_to_upload);



  return driver;
}

void driver_upload_destroy(driver_upload_t *driver)
{
  if(!driver->is_shutdown)
    driver_upload_close(driver);
  safe_free(driver);

}

void driver_upload_close(driver_upload_t *driver)
{
  LOG_WARNING("SONO IN DRIVER UPLOAD CLOSE");
  driver->is_shutdown = TRUE;
}