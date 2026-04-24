#ifndef TYPES_H
#define TYPES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_USERS     500
#define MAX_GOODS     2000
#define MAX_ORDERS    5000
#define MAX_MSGS      5000
#define MAX_REVIEWS   5000
#define MAX_CHATS     10000
#define MAX_ADDR      5
#define MAX_IMGS      5
#define MAX_FAVORITES 200
#define MAX_HISTORY   50
#define MAX_BLACKLIST 50
#define MAX_REPORTS   500
#define MAX_BANNERS   10
#define MAX_NOTICES   20
#define MAX_CATS      20
#define MAX_CART      50

/* 用户状态 */
#define USER_NORMAL  0
#define USER_BANNED  1

/* 商品状态 */
#define GOODS_DRAFT    0
#define GOODS_ON       1
#define GOODS_OFF      2
#define GOODS_SOLD     3
#define GOODS_PENDING  4  /* 待审核 */
#define GOODS_REJECT   5  /* 审核拒绝 */

/* 订单状态 */
#define ORDER_UNPAID    0
#define ORDER_PAID      1
#define ORDER_SHIPPED   2
#define ORDER_RECEIVED  3
#define ORDER_REVIEWED  4
#define ORDER_DONE      5
#define ORDER_CANCELLED 6
#define ORDER_REFUND    7
#define ORDER_DISPUTE   8

/* 评价类型 */
#define REVIEW_GOOD  1
#define REVIEW_MID   0
#define REVIEW_BAD  -1

/* 消息类型 */
#define MSG_ORDER   0
#define MSG_COMMENT 1
#define MSG_SYSTEM  2

/* 发货方式 */
#define SHIP_EXPRESS 0
#define SHIP_SELFPICK 1

typedef struct {
    int    id;
    char   phone[20];
    char   email[50];
    char   password[64];
    char   nickname[32];
    char   avatar[64];
    char   bio[128];
    int    verified;          /* 实名认证 */
    char   real_name[32];
    char   id_card[20];
    int    status;            /* USER_NORMAL/USER_BANNED */
    int    is_admin;
    double balance;           /* 账户余额（担保） */
    int    favorites[MAX_FAVORITES];
    int    fav_cnt;
    int    history[MAX_HISTORY];
    int    hist_cnt;
    int    blacklist[MAX_BLACKLIST];
    int    black_cnt;
    char   addrs[MAX_ADDR][128];
    int    addr_cnt;
    int    def_addr;
    time_t reg_time;
} User;

typedef struct {
    int    id;
    int    seller_id;
    char   title[64];
    char   desc[256];
    double price;
    double ori_price;
    int    condition;         /* 0全新 1九九新 2九五新 3九成新 4八成以下 */
    int    category;          /* 分类id */
    char   city[32];
    char   region[32];
    char   images[MAX_IMGS][64];
    int    img_cnt;
    int    status;
    int    view_cnt;
    time_t pub_time;
} Goods;

typedef struct {
    int    id;
    int    buyer_id;
    int    seller_id;
    int    goods_id;
    double amount;
    int    ship_type;
    char   address[128];
    char   tracking[32];
    int    status;
    time_t create_time;
    time_t pay_time;
    time_t ship_time;
    time_t recv_time;
    char   refund_reason[128];
    char   dispute_desc[256];
} Order;

typedef struct {
    int  id;
    int  order_id;
    int  from_id;    /* 评价人 */
    int  to_id;      /* 被评价人 */
    int  score;      /* REVIEW_GOOD/MID/BAD */
    char content[256];
    time_t create_time;
} Review;

typedef struct {
    int  id;
    int  from_id;
    int  to_id;
    char content[256];
    int  goods_id;   /* 关联商品，0表示无 */
    int  is_read;
    time_t send_time;
} ChatMsg;

typedef struct {
    int  id;
    int  user_id;
    char content[256];
    int  type;       /* MSG_ORDER/MSG_COMMENT/MSG_SYSTEM */
    int  is_read;
    time_t send_time;
} Notification;

typedef struct {
    int  id;
    int  reporter_id;
    int  target_id;  /* 被举报用户或商品id */
    int  is_goods;   /* 1=商品举报 0=用户举报 */
    char reason[128];
    int  handled;
    time_t report_time;
} Report;

/* 购物车条目 */
typedef struct {
    int user_id;
    int goods_id;
} CartItem;

/* 平台内容 */
typedef struct {
    char banners[MAX_BANNERS][128];
    int  banner_cnt;
    char notices[MAX_NOTICES][256];
    int  notice_cnt;
    char categories[MAX_CATS][32];
    int  cat_cnt;
} Platform;

/* 全局数据库 */
typedef struct {
    User         users[MAX_USERS];
    int          user_cnt;
    Goods        goods[MAX_GOODS];
    int          goods_cnt;
    Order        orders[MAX_ORDERS];
    int          order_cnt;
    Review       reviews[MAX_REVIEWS];
    int          review_cnt;
    ChatMsg      chats[MAX_CHATS];
    int          chat_cnt;
    Notification notifs[MAX_MSGS];
    int          notif_cnt;
    Report       reports[MAX_REPORTS];
    int          report_cnt;
    CartItem     cart[MAX_CART * MAX_USERS];
    int          cart_cnt;
    Platform     platform;
    int          next_user_id;
    int          next_goods_id;
    int          next_order_id;
    int          next_review_id;
    int          next_chat_id;
    int          next_notif_id;
    int          next_report_id;
} DB;

extern DB db;
extern User *cur_user;

#endif
