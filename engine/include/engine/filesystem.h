#ifndef FILESYSTEM_H
#define FILESYSTEM_H

// Handle to a file
typedef struct file_handle {
  void *handle;
  bool is_valid;
} file_handle;

typedef enum file_modes {
  FILE_MODE_READ = 0x1,
  FILE_MODE_WRITE = 0x2
} file_modes;

/**
 * Checks if a file with the given path exists
 * @param path The path of the file to check
 * @returns true if exists, false if not
 */
bool filesystem_exists(const char *path);

/**
 */
bool filesystem_open(const char *path, file_modes mode, bool binary,
                     file_handle *out_handle);

void filesystem_close(file_handle *handle);

bool filesystem_read_line(file_handle *handle, char **line_buf);

bool filesystem_write_line(file_handle *handle, const char *text);

bool filesystem_read(file_handle *handle, long data_size, void *out_data,
                     long *out_bytes_read);

bool filesystem_read_all_bytes(file_handle *handle, char **out_bytes,
                               long *out_bytes_read);

bool filesystem_write(file_handle *handle, char data_size, const void *data,
                      char *out_bytes_written);

#endif
