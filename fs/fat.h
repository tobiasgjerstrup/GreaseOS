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
const char* fat_last_error(void);
