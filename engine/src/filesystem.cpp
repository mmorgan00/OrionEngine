#include "engine/filesystem.h"

#include "engine/logger.h"

// TODO: REFACTOR
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <cstdint>
#include <cstdlib>

bool filesystem_exists(const char *path) {
  struct stat buffer;
  return stat(path, &buffer) == 0;
}

/**
 */
bool filesystem_open(const char *path, file_modes mode, bool binary,
                     file_handle *out_handle) {
  out_handle->is_valid = false;
  out_handle->handle = 0;
  const char *mode_str;

  if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) != 0) {
    mode_str = binary ? "w+b" : "w+";
  } else if ((mode & FILE_MODE_READ) != 0 && (mode & FILE_MODE_WRITE) == 0) {
    mode_str = binary ? "rb" : "r";
  } else if ((mode & FILE_MODE_READ) == 0 && (mode & FILE_MODE_WRITE) != 0) {
    mode_str = binary ? "wb" : "w";
  } else {
    OE_LOG(LOG_LEVEL_ERROR,
           "Invalid mode passed while trying to open file: '%s'", path);
    return false;
  }

  // Attempt to open file
  FILE *file = fopen(path, mode_str);
  if (!file) {
    OE_LOG(LOG_LEVEL_ERROR, "Error opening file: '%s'", path);
    return false;
  }

  out_handle->handle = file;
  out_handle->is_valid = true;

  return true;
}

void filesystem_close(file_handle *handle) {
  if (handle->handle) {
    fclose((FILE *)handle->handle);
    handle->handle = 0;
    handle->is_valid = false;
  }
}

bool filesystem_read_line(file_handle *handle, char **line_buf) {
  if (handle->handle) {
    // Single item we will guess is less than 32000 chars
    char buffer[32000];
    if (fgets(buffer, 32000, (FILE *)handle->handle) != 0) {
      long length = strlen(buffer);
      *line_buf = (char *)malloc((sizeof(char)) * length + 1);
      strcpy(*line_buf, buffer);
      return true;
    }
  }
  return false;
}

bool filesystem_write_line(file_handle *handle, const char *text) {
  if (handle->handle) {
    int32_t result = fputs(text, (FILE *)handle->handle);
    if (result != EOF) {
      result = fputc('\n', (FILE *)handle->handle);
    }

    // Flush the stream so it is written immediately.
    // This prevents data loss in the event of a crash.
    fflush((FILE *)handle->handle);
    return result != EOF;
  }
  return false;
}

bool filesystem_read(file_handle *handle, long data_size, void *out_data,
                     long *out_bytes_read) {
  if (handle->handle && out_data) {
    *out_bytes_read = fread(out_data, 1, data_size, (FILE *)handle->handle);
    if (*out_bytes_read != data_size) {
      return false;
    }
    return true;
  }
  return false;
}

bool filesystem_read_all_bytes(file_handle *handle, char **out_bytes,
                               long *out_bytes_read) {
  if (handle->handle) {
    // File size
    fseek((FILE *)handle->handle, 0, SEEK_END);
    long size = ftell((FILE *)handle->handle);
    rewind((FILE *)handle->handle);

    *out_bytes = (char *)malloc(sizeof(bool) * size);
    *out_bytes_read = fread(*out_bytes, 1, size, (FILE *)handle->handle);
    if (*out_bytes_read != size) {
      return false;
    }
    return true;
  }
  return false;
}

bool filesystem_write(file_handle *handle, long data_size, const void *data,
                      long *out_bytes_written) {
  if (handle->handle) {
    *out_bytes_written = fwrite(data, 1, data_size, (FILE *)handle->handle);
    if (*out_bytes_written != data_size) {
      return false;
    }
    fflush((FILE *)handle->handle);
    return true;
  }
  return false;
}
