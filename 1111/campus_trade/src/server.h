#ifndef SERVER_H
#define SERVER_H

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_BUF     65536
#define MAX_PATH    512
#define MAX_LINE    1024
#define DATA_DIR    "../data"
#define WWW_DIR     "../www"

/* ===== 数据结构 ===== */
typedef struct {
    int  id;
    char username[64];
    char password[64];   /* MD5简化为明文+盐 */
    char phone[20];
    char email[64];
    char nickname[64];
    char avatar[128];
    char bio[256];
    int  is_admin;
    int  is_banned;
    int  verified;       /* 实名认证 */
    char real_name[64];
    char id_card[20];
    char reg_time[32];
} User;

typedef struct {
    int  id;
    int  seller_id;
    char title[128];
    char desc[512];
    float price;
    float orig_price;
    int  condition;      /* 0=全新 1=99新 2=95新 3=9成新 4=8成新 */
    int  category;       /* 0=手机 1=数码 2=服饰 3=图书 4=家居 5=其他 */
    char location[64];
    char images[512];    /* 逗号分隔的图片路径 */
    int  status;         /* 0=草稿 1=上架 2=下架 3=已售 */
    int  views;
    int  favs;
    char create_time[32];
} Product;

typedef struct {
    int  id;
    int  buyer_id;
    int  seller_id;
    int  product_id;
    float amount;
    int  status;  /* 0=待付款 1=待发货 2=待收货 3=待评价 4=完成 5=取消 6=退款中 */
    char address[256];
    char express_no[64];
    char create_time[32];
    char pay_time[32];
    char ship_time[32];
    char recv_time[32];
} Order;

typedef struct {
    int  id;
    int  from_id;
    int  to_id;
    int  product_id;  /* 0=系统消息 */
    char content[512];
    int  is_read;
    char send_time[32];
} Message;

typedef struct {
    int  id;
    int  order_id;
    int  reviewer_id;
    int  target_id;
    int  score;    /* 1=差评 2=中评 3=好评 */
    char comment[256];
    char time[32];
} Review;

typedef struct {
    int  id;
    int  user_id;
    int  product_id;  /* 0=举报用户 */
    int  target_user_id;
    char reason[256];
    int  status;  /* 0=待处理 1=已处理 */
    char time[32];
} Report;

/* ===== HTTP请求/响应 ===== */
typedef struct {
    char method[8];
    char path[256];
    char query[512];
    char body[MAX_BUF];
    int  body_len;
    char session_id[64];
    int  user_id;   /* 0=未登录 */
} HttpReq;

typedef struct {
    int  status;
    char content_type[64];
    char body[MAX_BUF];
    int  body_len;
} HttpResp;

/* ===== 函数声明 ===== */
void start_server(int port);
void init_data_dir(void);
void handle_request(int sock, HttpReq *req);

/* user.c */
void handle_user(HttpReq *req, HttpResp *resp);

/* product.c */
void handle_product(HttpReq *req, HttpResp *resp);

/* order.c */
void handle_order(HttpReq *req, HttpResp *resp);

/* chat.c */
void handle_chat(HttpReq *req, HttpResp *resp);

/* admin.c */
void handle_admin(HttpReq *req, HttpResp *resp);

/* utils.c */
int   gen_id(const char *file);
void  get_time_str(char *buf);
char *url_decode(const char *src, char *dst, int dlen);
char *get_param(const char *query, const char *key, char *val, int vlen);
char *get_body_param(const char *body, const char *key, char *val, int vlen);
void  json_str(char *out, int olen, const char *key, const char *val);
void  json_int(char *out, int olen, const char *key, int val);
int   save_session(int user_id, char *sid_out);
int   load_session(const char *sid);
void  resp_json(HttpResp *resp, int status, const char *json);
void  resp_ok(HttpResp *resp, const char *msg);
void  resp_err(HttpResp *resp, const char *msg);

/* file db */
int   db_append(const char *file, const char *line);
int   db_find_line(const char *file, const char *key, int field, char *out, int olen);
int   db_update_field(const char *file, int id_field, int id_val,
                       int upd_field, const char *new_val);
int   db_count(const char *file);
void  db_list(const char *file, char *out, int olen, int limit, int offset);

#endif
