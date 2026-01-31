#pragma once

#include <stddef.h>
#include <stdint.h>

int fat_init(void);
int fat_ls(void);
int fat_cd(const char* name);
const char* fat_pwd(void);
int fat_mkdir(const char* name);
int fat_touch(const char* name);
int fat_cat(const char* name);
int fat_write(const char* name, const char* data);
int fat_write_data(const char* name, const char* data, size_t data_len);
int fat_read(const char* name, char* out, size_t max, size_t* out_size);
const char* fat_last_error(void);
int fat_df(void);
int fat_rm(const char* name);
int fat_rmdir(const char* name);
