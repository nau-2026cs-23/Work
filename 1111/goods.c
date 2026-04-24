/*
 * goods.c  -- 商品模块
 * 包含：发布、浏览、搜索、详情、收藏、留言、购物车
 */
#include "utils.h"

static const char *CONDITION[] = {"全新","99新","95新","9成新","8成以下"};

/* ---------- 推送浏览历史 ---------- */
static void push_history(int goods_id) {
    if (!cur_user) return;
    /* 去重 */
    for (int i = 0; i < cur_user->hist_cnt; i++)
        if (cur_user->history[i] == goods_id) return;
    if (cur_user->hist_cnt >= MAX_HISTORY) {
        /* 移除最旧 */
        for (int i = 0; i < MAX_HISTORY-1; i++)
            cur_user->history[i] = cur_user->history[i+1];
        cur_user->hist_cnt = MAX_HISTORY-1;
    }
    cur_user->history[cur_user->hist_cnt++] = goods_id;
}

/* ---------- 显示分类列表 ---------- */
static void show_categories(void) {
    printf("  分类：");
    for (int i = 0; i < db.platform.cat_cnt; i++)
        printf("[%d]%s ", i, db.platform.categories[i]);
    printf("\n");
}

/* ---------- 发布商品 ---------- */
static void publish_goods(void) {
    if (!cur_user) { printf("请先登录\n"); return; }
    if (!cur_user->verified) { printf("请先完成实名认证\n"); return; }
    if (db.goods_cnt >= MAX_GOODS) { printf("商品数量已达上限\n"); return; }

    Goods *g = &db.goods[db.goods_cnt];
    memset(g, 0, sizeof(Goods));

    input_str("商品标题", g->title, 64);
    input_str("商品描述", g->desc, 256);
    g->price = input_double("售价");
    g->ori_price = input_double("原价");

    printf("  成色: [0]全新 [1]99新 [2]95新 [3]9成新 [4]8成以下\n");
    g->condition = input_int("成色");
    if (g->condition < 0 || g->condition > 4) g->condition = 0;

    show_categories();
    g->category = input_int("分类编号");

    input_str("所在城市", g->city, 32);
    input_str("所在区域", g->region, 32);

    /* 图片（模拟文件名） */
    printf("  最多上传%d张图片，输入文件名，空行结束\n", MAX_IMGS);
    while (g->img_cnt < MAX_IMGS) {
        char fn[64]; input_str("图片文件名(回车结束)", fn, 64);
        if (!fn[0]) break;
        strncpy(g->images[g->img_cnt++], fn, 63);
    }

    printf("  [1]直接上架(待审核) [2]保存草稿\n");
    int op = input_int("选择");
    g->status = (op == 1) ? GOODS_PENDING : GOODS_DRAFT;
    g->seller_id = cur_user->id;
    g->id = db.next_goods_id++;
    g->pub_time = time(NULL);
    db.goods_cnt++;
    data_save();
    printf("发布成功！商品ID: %d\n", g->id);
}

/* ---------- 打印商品简要 ---------- */
static void print_goods_brief(Goods *g) {
    User *s = find_user_by_id(g->seller_id);
    printf("  [%d] %s | %s | ¥%.2f(原¥%.2f) | %s/%s | 卖家:%s%s\n",
           g->id, g->title, CONDITION[g->condition],
           g->price, g->ori_price, g->city, g->region,
           s ? s->nickname : "?",
           s && s->verified ? "[认证]" : "");
}

/* ---------- 商品详情 ---------- */
static void goods_detail(int gid) {
    Goods *g = find_goods_by_id(gid);
    if (!g || g->seller_id == 0) { printf("商品不存在\n"); return; }
    if (g->status != GOODS_ON) { printf("商品已下架或待审核\n"); return; }

    g->view_cnt++;
    push_history(gid);

    print_line(40);
    printf("  标题: %s\n  描述: %s\n", g->title, g->desc);
    printf("  价格: ¥%.2f  原价: ¥%.2f  成色: %s\n",
           g->price, g->ori_price, CONDITION[g->condition]);
    printf("  位置: %s %s\n  浏览: %d次\n", g->city, g->region, g->view_cnt);
    if (g->img_cnt) {
        printf("  图片:");
        for (int i = 0; i < g->img_cnt; i++) printf(" %s", g->images[i]);
        printf("\n");
    }
    User *s = find_user_by_id(g->seller_id);
    if (s) {
        int good=0, bad=0;
        for (int i = 0; i < db.review_cnt; i++) {
            Review *r = &db.reviews[i];
            if (r->to_id == s->id) {
                if (r->score == REVIEW_GOOD) good++;
                else if (r->score == REVIEW_BAD) bad++;
            }
        }
        printf("  卖家: %s%s  好评:%d 差评:%d\n",
               s->nickname, s->verified?"[实名]":"", good, bad);
    }
    print_line(40);

    if (!cur_user) return;
    printf("  [1]立即购买 [2]加购物车 [3]收藏 [4]留言 [0]返回\n");
    int op = input_int("操作");
    if (op == 1) {
        /* 跳转到下单流程 */
        extern void place_order(int goods_id);
        place_order(gid);
    } else if (op == 2) {
        /* 加购物车 */
        for (int i = 0; i < db.cart_cnt; i++)
            if (db.cart[i].user_id == cur_user->id && db.cart[i].goods_id == gid) {
                printf("已在购物车中\n"); return;
            }
        if (db.cart_cnt < MAX_CART * MAX_USERS) {
            db.cart[db.cart_cnt].user_id = cur_user->id;
            db.cart[db.cart_cnt].goods_id = gid;
            db.cart_cnt++;
            data_save();
            printf("已加入购物车\n");
        }
    } else if (op == 3) {
        /* 收藏 */
        for (int i = 0; i < cur_user->fav_cnt; i++)
            if (cur_user->favorites[i] == gid) { printf("已收藏\n"); return; }
        if (cur_user->fav_cnt < MAX_FAVORITES) {
            cur_user->favorites[cur_user->fav_cnt++] = gid;
            data_save();
            printf("收藏成功\n");
        }
    } else if (op == 4) {
        /* 留言 */
        char msg[256]; input_str("留言内容", msg, 256);
        if (db.chat_cnt < MAX_CHATS) {
            ChatMsg *cm = &db.chats[db.chat_cnt++];
            cm->id = db.next_chat_id++;
            cm->from_id = cur_user->id;
            cm->to_id = g->seller_id;
            strncpy(cm->content, msg, 255);
            cm->goods_id = gid;
            cm->is_read = 0;
            cm->send_time = time(NULL);
            push_notif(g->seller_id, MSG_COMMENT, msg);
            data_save();
            printf("留言成功\n");
        }
    }
}

/* ---------- 首页/最新 ---------- */
static void browse_latest(void) {
    print_line(40);
    printf("  最新上架\n");
    int cnt = 0;
    for (int i = db.goods_cnt-1; i >= 0 && cnt < 20; i--) {
        Goods *g = &db.goods[i];
        if (g->status != GOODS_ON || g->seller_id == 0) continue;
        print_goods_brief(g);
        cnt++;
    }
    if (!cnt) printf("  暂无商品\n");
}

/* ---------- 分类浏览 ---------- */
static void browse_by_cat(void) {
    show_categories();
    int cat = input_int("分类编号");
    print_line(40);
    int cnt = 0;
    for (int i = 0; i < db.goods_cnt; i++) {
        Goods *g = &db.goods[i];
        if (g->status != GOODS_ON || g->seller_id == 0) continue;
        if (g->category != cat) continue;
        print_goods_brief(g);
        cnt++;
    }
    if (!cnt) printf("  该分类暂无商品\n");
}

/* ---------- 搜索 ---------- */
static void search_goods(void) {
    char kw[64]; input_str("关键词(空=不限)", kw, 64);
    double pmin = input_double("最低价(0=不限)");
    double pmax = input_double("最高价(0=不限)");
    char city[32]; input_str("城市(空=不限)", city, 32);
    printf("成色筛选: -1不限 0全新 1九九新 2九五新 3九成新 4八成以下\n");
    int cond = input_int("成色");

    print_line(40);
    int cnt = 0;
    for (int i = 0; i < db.goods_cnt; i++) {
        Goods *g = &db.goods[i];
        if (g->status != GOODS_ON || g->seller_id == 0) continue;
        if (kw[0] && !strstr(g->title, kw) && !strstr(g->desc, kw)) continue;
        if (pmin > 0 && g->price < pmin) continue;
        if (pmax > 0 && g->price > pmax) continue;
        if (city[0] && strcmp(g->city, city) != 0) continue;
        if (cond >= 0 && g->condition != cond) continue;
        print_goods_brief(g);
        cnt++;
    }
    if (!cnt) printf("  未找到匹配商品\n");
}

/* ---------- 我的购物车 ---------- */
static void my_cart(void) {
    if (!cur_user) { printf("请先登录\n"); return; }
    print_line(40);
    printf("  我的购物车\n");
    int cnt = 0;
    for (int i = 0; i < db.cart_cnt; i++) {
        CartItem *ci = &db.cart[i];
        if (ci->user_id != cur_user->id) continue;
        Goods *g = find_goods_by_id(ci->goods_id);
        if (!g) continue;
        printf("  [%d] %s ¥%.2f [%s]\n", g->id, g->title, g->price,
               goods_status_str(g->status));
        cnt++;
    }
    if (!cnt) { printf("  购物车为空\n"); return; }
    int gid = input_int("输入商品ID购买(0取消)");
    if (!gid) return;
    extern void place_order(int goods_id);
    place_order(gid);
}

/* ---------- 公告栏 ---------- */
static void show_notices(void) {
    print_line(40);
    printf("  平台公告\n");
    for (int i = 0; i < db.platform.notice_cnt; i++)
        printf("  %d. %s\n", i+1, db.platform.notices[i]);
}

/* ---------- 商品主菜单 ---------- */
void goods_menu(void) {
    int ch;
    while (1) {
        print_line(40);
        printf("  [1]首页最新  [2]分类浏览  [3]搜索商品\n");
        printf("  [4]查看详情  [5]发布商品  [6]购物车\n");
        printf("  [7]平台公告  [0]返回\n");
        ch = input_int("选择");
        if (ch == 1) browse_latest();
        else if (ch == 2) browse_by_cat();
        else if (ch == 3) search_goods();
        else if (ch == 4) { int id = input_int("商品ID"); goods_detail(id); }
        else if (ch == 5) publish_goods();
        else if (ch == 6) my_cart();
        else if (ch == 7) show_notices();
        else if (ch == 0) return;
        pause_screen();
    }
}
