/*
 * data.c  -- 数据持久化 + 工具函数 + 管理后台
 */
#include "utils.h"

/* ===== 全局变量 ===== */
DB    db;
User *cur_user = NULL;

#define DATA_FILE "campus_trade.dat"

/* ===== 工具函数实现 ===== */
void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void pause_screen(void) {
    printf("\n  按回车继续...");
    getchar(); getchar();
}

int input_int(const char *prompt) {
    char buf[32]; int v = 0;
    if (prompt && prompt[0]) printf("  %s: ", prompt);
    if (fgets(buf, 32, stdin)) sscanf(buf, "%d", &v);
    return v;
}

double input_double(const char *prompt) {
    char buf[32]; double v = 0;
    if (prompt && prompt[0]) printf("  %s: ", prompt);
    if (fgets(buf, 32, stdin)) sscanf(buf, "%lf", &v);
    return v;
}

void input_str(const char *prompt, char *buf, int size) {
    if (prompt && prompt[0]) printf("  %s: ", prompt);
    if (fgets(buf, size, stdin)) {
        int len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
    }
}

void print_line(int n) {
    printf("\n");
    for (int i = 0; i < n; i++) printf("-");
    printf("\n");
}

void gen_code(char *buf) {
    srand((unsigned)time(NULL));
    for (int i = 0; i < 6; i++) buf[i] = '0' + rand() % 10;
    buf[6] = '\0';
}

void hash_pwd(const char *in, char *out) {
    /* 简单DJB2哈希，足够演示用 */
    unsigned long h = 5381;
    for (int i = 0; in[i]; i++) h = ((h << 5) + h) + in[i];
    sprintf(out, "%lu", h);
}

int valid_phone(const char *s) {
    if (strlen(s) != 11) return 0;
    for (int i = 0; s[i]; i++) if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

int valid_email(const char *s) {
    return strchr(s, '@') && strchr(s, '.') ? 1 : 0;
}

const char *goods_status_str(int s) {
    static const char *t[] = {"草稿","上架","下架","已售","待审核","已拒绝"};
    if (s < 0 || s > 5) return "未知";
    return t[s];
}

const char *order_status_str(int s) {
    static const char *t[] = {"待付款","待发货","已发货","待评价","已评价","已完成","已取消","退款中","投诉中"};
    if (s < 0 || s > 8) return "未知";
    return t[s];
}

const char *condition_str(int c) {
    static const char *t[] = {"全新","99新","95新","9成新","8成以下"};
    if (c < 0 || c > 4) return "未知";
    return t[c];
}

void push_notif(int user_id, int type, const char *content) {
    if (db.notif_cnt >= MAX_MSGS) return;
    Notification *n = &db.notifs[db.notif_cnt++];
    n->id = db.next_notif_id++;
    n->user_id = user_id;
    n->type = type;
    strncpy(n->content, content, 255);
    n->is_read = 0;
    n->send_time = time(NULL);
}

User *find_user_by_id(int id) {
    for (int i = 0; i < db.user_cnt; i++)
        if (db.users[i].id == id) return &db.users[i];
    return NULL;
}

Goods *find_goods_by_id(int id) {
    for (int i = 0; i < db.goods_cnt; i++)
        if (db.goods[i].id == id) return &db.goods[i];
    return NULL;
}

Order *find_order_by_id(int id) {
    for (int i = 0; i < db.order_cnt; i++)
        if (db.orders[i].id == id) return &db.orders[i];
    return NULL;
}

/* ===== 数据持久化（二进制文件） ===== */
void data_save(void) {
    FILE *f = fopen(DATA_FILE, "wb");
    if (!f) { perror("保存数据失败"); return; }
    fwrite(&db, sizeof(DB), 1, f);
    fclose(f);
}

void data_load(void) {
    FILE *f = fopen(DATA_FILE, "rb");
    if (!f) {
        /* 初始化默认数据 */
        memset(&db, 0, sizeof(DB));
        db.next_user_id = 1;
        db.next_goods_id = 1;
        db.next_order_id = 1;
        db.next_review_id = 1;
        db.next_chat_id = 1;
        db.next_notif_id = 1;
        db.next_report_id = 1;

        /* 默认分类 */
        Platform *p = &db.platform;
        strcpy(p->categories[p->cat_cnt++], "手机数码");
        strcpy(p->categories[p->cat_cnt++], "电脑配件");
        strcpy(p->categories[p->cat_cnt++], "服饰鞋帽");
        strcpy(p->categories[p->cat_cnt++], "图书教材");
        strcpy(p->categories[p->cat_cnt++], "生活家居");
        strcpy(p->categories[p->cat_cnt++], "运动健身");
        strcpy(p->categories[p->cat_cnt++], "乐器玩具");
        strcpy(p->categories[p->cat_cnt++], "其他");

        /* 默认公告 */
        strcpy(p->notices[p->notice_cnt++], "欢迎使用校园二手交易平台！");
        strcpy(p->notices[p->notice_cnt++], "交易请使用平台担保，谨防诈骗！");

        /* 默认管理员账户 admin/admin123 */
        User *admin = &db.users[db.user_cnt++];
        memset(admin, 0, sizeof(User));
        admin->id = db.next_user_id++;
        strcpy(admin->phone, "10000000000");
        strcpy(admin->nickname, "管理员");
        hash_pwd("admin123", admin->password);
        admin->is_admin = 1;
        admin->verified = 1;
        admin->reg_time = time(NULL);

        data_save();
        printf("  首次运行，已初始化数据（管理员: 手机10000000000 密码admin123）\n");
        return;
    }
    fread(&db, sizeof(DB), 1, f);
    fclose(f);
}

/* ===== 管理员后台 ===== */
static void admin_user_mgr(void) {
    print_line(40);
    printf("  用户管理\n");
    for (int i = 0; i < db.user_cnt; i++) {
        User *u = &db.users[i];
        printf("  ID:%d %s %s %s%s\n",
               u->id, u->nickname, u->phone,
               u->status == USER_BANNED ? "[封禁]" : "",
               u->is_admin ? "[管理员]" : "");
    }
    printf("  总用户数: %d\n", db.user_cnt);
    print_line(40);
    printf("  [1]封禁用户 [2]解封用户 [0]返回\n");
    int op = input_int("操作");
    if (!op) return;
    int uid = input_int("用户ID");
    User *u = find_user_by_id(uid);
    if (!u) { printf("用户不存在\n"); return; }
    u->status = (op == 1) ? USER_BANNED : USER_NORMAL;
    data_save();
    printf("操作成功\n");
}

static void admin_goods_mgr(void) {
    print_line(40);
    printf("  商品管理（待审核）\n");
    int cnt = 0;
    for (int i = 0; i < db.goods_cnt; i++) {
        Goods *g = &db.goods[i];
        if (g->status == GOODS_PENDING) {
            printf("  ID:%d [%s] %s ¥%.2f 卖家:%d\n",
                   g->id, goods_status_str(g->status), g->title, g->price, g->seller_id);
            cnt++;
        }
    }
    if (!cnt) { printf("  无待审核商品\n"); }
    printf("  总商品数: %d\n", db.goods_cnt);
    print_line(40);
    printf("  [1]审核通过 [2]审核拒绝 [3]强制下架 [4]删除 [0]返回\n");
    int op = input_int("操作");
    if (!op) return;
    int gid = input_int("商品ID");
    Goods *g = find_goods_by_id(gid);
    if (!g) { printf("商品不存在\n"); return; }
    if (op == 1) g->status = GOODS_ON;
    else if (op == 2) g->status = GOODS_REJECT;
    else if (op == 3) g->status = GOODS_OFF;
    else if (op == 4) { g->status = GOODS_OFF; g->seller_id = 0; }
    data_save();
    printf("操作成功\n");
}

static void admin_order_mgr(void) {
    print_line(40);
    printf("  订单管理\n");
    int cnt = 0;
    for (int i = 0; i < db.order_cnt; i++) {
        Order *o = &db.orders[i];
        printf("  订单%d 买:%d 卖:%d 商品:%d ¥%.2f [%s]\n",
               o->id, o->buyer_id, o->seller_id, o->goods_id,
               o->amount, order_status_str(o->status));
        cnt++;
    }
    if (!cnt) printf("  暂无订单\n");
    printf("  总订单数: %d\n", db.order_cnt);
    print_line(40);
    /* 处理投诉/退款 */
    printf("  [1]同意退款 [2]拒绝退款/驳回投诉 [0]返回\n");
    int op = input_int("操作");
    if (!op) return;
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o) { printf("订单不存在\n"); return; }
    if (op == 1) {
        /* 退款给买家 */
        User *buyer = find_user_by_id(o->buyer_id);
        if (buyer) buyer->balance += o->amount;
        o->status = ORDER_CANCELLED;
        push_notif(o->buyer_id, MSG_ORDER, "退款已处理，款项已退还");
        push_notif(o->seller_id, MSG_ORDER, "平台已处理退款申请");
        printf("退款成功\n");
    } else {
        o->status = ORDER_RECEIVED;
        push_notif(o->buyer_id, MSG_ORDER, "退款/投诉申请已被驳回");
        printf("已驳回\n");
    }
    data_save();
}

static void admin_content_mgr(void) {
    Platform *p = &db.platform;
    while (1) {
        print_line(40);
        printf("  内容管理\n");
        printf("  [1]轮播图管理 [2]公告管理 [3]分类管理 [0]返回\n");
        int op = input_int("操作");
        if (!op) return;
        if (op == 1) {
            printf("  当前轮播图:\n");
            for (int i = 0; i < p->banner_cnt; i++)
                printf("  [%d] %s\n", i+1, p->banners[i]);
            printf("  [1]新增 [2]删除\n");
            int a = input_int("操作");
            if (a == 1 && p->banner_cnt < MAX_BANNERS) {
                input_str("图片路径/URL", p->banners[p->banner_cnt++], 128);
            } else if (a == 2) {
                int idx = input_int("序号") - 1;
                if (idx >= 0 && idx < p->banner_cnt) {
                    for (int i = idx; i < p->banner_cnt-1; i++)
                        memcpy(p->banners[i], p->banners[i+1], 128);
                    p->banner_cnt--;
                }
            }
        } else if (op == 2) {
            printf("  当前公告:\n");
            for (int i = 0; i < p->notice_cnt; i++)
                printf("  [%d] %s\n", i+1, p->notices[i]);
            printf("  [1]新增 [2]删除\n");
            int a = input_int("操作");
            if (a == 1 && p->notice_cnt < MAX_NOTICES) {
                input_str("公告内容", p->notices[p->notice_cnt++], 256);
            } else if (a == 2) {
                int idx = input_int("序号") - 1;
                if (idx >= 0 && idx < p->notice_cnt) {
                    for (int i = idx; i < p->notice_cnt-1; i++)
                        memcpy(p->notices[i], p->notices[i+1], 256);
                    p->notice_cnt--;
                }
            }
        } else if (op == 3) {
            printf("  当前分类:\n");
            for (int i = 0; i < p->cat_cnt; i++)
                printf("  [%d] %s\n", i, p->categories[i]);
            printf("  [1]新增 [2]删除\n");
            int a = input_int("操作");
            if (a == 1 && p->cat_cnt < MAX_CATS) {
                input_str("分类名称", p->categories[p->cat_cnt++], 32);
            } else if (a == 2) {
                int idx = input_int("序号");
                if (idx >= 0 && idx < p->cat_cnt) {
                    for (int i = idx; i < p->cat_cnt-1; i++)
                        memcpy(p->categories[i], p->categories[i+1], 32);
                    p->cat_cnt--;
                }
            }
        }
        data_save();
    }
}

static void admin_stats(void) {
    print_line(40);
    int sold = 0, goods_on = 0;
    for (int i = 0; i < db.goods_cnt; i++) {
        if (db.goods[i].status == GOODS_ON) goods_on++;
        if (db.goods[i].status == GOODS_SOLD) sold++;
    }
    int done = 0;
    double gmv = 0;
    for (int i = 0; i < db.order_cnt; i++) {
        if (db.orders[i].status >= ORDER_RECEIVED) { done++; gmv += db.orders[i].amount; }
    }
    printf("  注册用户数: %d\n", db.user_cnt);
    printf("  在售商品数: %d  已售商品数: %d\n", goods_on, sold);
    printf("  成交订单数: %d  成交金额: ¥%.2f\n", done, gmv);
    printf("  举报未处理: ");
    int rep = 0;
    for (int i = 0; i < db.report_cnt; i++) if (!db.reports[i].handled) rep++;
    printf("%d\n", rep);
}

static void admin_report_mgr(void) {
    print_line(40);
    printf("  举报列表\n");
    for (int i = 0; i < db.report_cnt; i++) {
        Report *r = &db.reports[i];
        printf("  ID:%d 举报人:%d 目标%s:%d 原因:%s %s\n",
               r->id, r->reporter_id,
               r->is_goods ? "商品" : "用户",
               r->target_id, r->reason,
               r->handled ? "[已处理]" : "[未处理]");
    }
    int rid = input_int("标记已处理的举报ID(0跳过)");
    if (rid) {
        for (int i = 0; i < db.report_cnt; i++) {
            if (db.reports[i].id == rid) { db.reports[i].handled = 1; break; }
        }
        data_save();
        printf("已处理\n");
    }
}

/* ---------- 管理后台主菜单 ---------- */
void admin_menu(void) {
    if (!cur_user || !cur_user->is_admin) {
        printf("无管理员权限\n"); return;
    }
    int ch;
    while (1) {
        print_line(40);
        printf("  ===== 管理后台 =====\n");
        printf("  [1]用户管理  [2]商品管理  [3]订单管理\n");
        printf("  [4]内容管理  [5]数据统计  [6]举报处理\n");
        printf("  [0]返回\n");
        ch = input_int("选择");
        if (ch == 1) admin_user_mgr();
        else if (ch == 2) admin_goods_mgr();
        else if (ch == 3) admin_order_mgr();
        else if (ch == 4) admin_content_mgr();
        else if (ch == 5) admin_stats();
        else if (ch == 6) admin_report_mgr();
        else if (ch == 0) return;
        pause_screen();
    }
}
