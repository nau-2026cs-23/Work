#ifndef UTILS_H
#define UTILS_H

#include "types.h"

/* ===== 工具函数 ===== */
void clear_screen(void);
void pause_screen(void);
int  input_int(const char *prompt);
double input_double(const char *prompt);
void input_str(const char *prompt, char *buf, int size);
void print_line(int n);
void gen_code(char *buf);         /* 生成6位验证码 */
void hash_pwd(const char *in, char *out); /* 简单哈希密码 */
int  valid_phone(const char *s);
int  valid_email(const char *s);
const char *goods_status_str(int s);
const char *order_status_str(int s);
const char *condition_str(int c);
void push_notif(int user_id, int type, const char *content);
User  *find_user_by_id(int id);
Goods *find_goods_by_id(int id);
Order *find_order_by_id(int id);

/* ===== 模块入口 ===== */
void user_menu(void);
void goods_menu(void);
void order_menu(void);
void admin_menu(void);

/* ===== 数据持久化 ===== */
void data_load(void);
void data_save(void);

#endif
