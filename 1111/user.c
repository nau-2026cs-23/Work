/*
 * user.c  -- 用户模块
 * 包含：注册/登录、个人中心、地址、收藏、历史、黑名单、聊天、通知、评价
 */
#include "utils.h"

/* ---------- 注册 ---------- */
static void do_register(void) {
    char phone[20], email[50], pwd[64], nick[32], code[8], rcode[8];
    int by_phone;

    print_line(40);
    printf("  [1] 手机号注册  [2] 邮箱注册\n");
    by_phone = (input_int("选择") == 1);

    if (by_phone) {
        input_str("手机号", phone, 20);
        if (!valid_phone(phone)) { printf("手机号格式错误\n"); return; }
        /* 模拟发送验证码 */
        gen_code(rcode);
        printf("【模拟】验证码: %s\n", rcode);
        input_str("输入验证码", code, 8);
        if (strcmp(code, rcode) != 0) { printf("验证码错误\n"); return; }
        /* 检查重复 */
        for (int i = 0; i < db.user_cnt; i++)
            if (strcmp(db.users[i].phone, phone) == 0) { printf("手机号已注册\n"); return; }
        email[0] = '\0';
    } else {
        input_str("邮箱", email, 50);
        if (!valid_email(email)) { printf("邮箱格式错误\n"); return; }
        gen_code(rcode);
        printf("【模拟】验证码: %s\n", rcode);
        input_str("输入验证码", code, 8);
        if (strcmp(code, rcode) != 0) { printf("验证码错误\n"); return; }
        for (int i = 0; i < db.user_cnt; i++)
            if (strcmp(db.users[i].email, email) == 0) { printf("邮箱已注册\n"); return; }
        phone[0] = '\0';
    }

    input_str("设置昵称", nick, 32);
    input_str("设置密码", pwd, 64);

    if (db.user_cnt >= MAX_USERS) { printf("用户已满\n"); return; }
    User *u = &db.users[db.user_cnt++];
    memset(u, 0, sizeof(User));
    u->id = db.next_user_id++;
    strncpy(u->phone, phone, 19);
    strncpy(u->email, email, 49);
    hash_pwd(pwd, u->password);
    strncpy(u->nickname, nick, 31);
    strcpy(u->avatar, "default.png");
    u->status = USER_NORMAL;
    u->reg_time = time(NULL);
    data_save();
    printf("注册成功！用户ID: %d\n", u->id);
}

/* ---------- 登录 ---------- */
static void do_login(void) {
    char input[64], pwd[64], hpwd[64], code[8], rcode[8];
    int mode;

    print_line(40);
    printf("  [1] 密码登录  [2] 短信快捷登录\n");
    mode = input_int("选择");

    input_str("手机号/邮箱", input, 64);
    User *found = NULL;
    for (int i = 0; i < db.user_cnt; i++) {
        User *u = &db.users[i];
        if (strcmp(u->phone, input) == 0 || strcmp(u->email, input) == 0) {
            found = u; break;
        }
    }
    if (!found) { printf("用户不存在\n"); return; }
    if (found->status == USER_BANNED) { printf("账号已被封禁\n"); return; }

    if (mode == 1) {
        input_str("密码", pwd, 64);
        hash_pwd(pwd, hpwd);
        if (strcmp(hpwd, found->password) != 0) { printf("密码错误\n"); return; }
    } else {
        gen_code(rcode);
        printf("【模拟】验证码: %s\n", rcode);
        input_str("输入验证码", code, 8);
        if (strcmp(code, rcode) != 0) { printf("验证码错误\n"); return; }
    }

    cur_user = found;
    printf("欢迎回来，%s！\n", cur_user->nickname);
}

/* ---------- 实名认证 ---------- */
static void do_verify(void) {
    if (cur_user->verified) { printf("已完成实名认证\n"); return; }
    input_str("真实姓名", cur_user->real_name, 32);
    input_str("身份证号", cur_user->id_card, 20);
    cur_user->verified = 1;
    data_save();
    printf("实名认证成功\n");
}

/* ---------- 个人资料 ---------- */
static void edit_profile(void) {
    print_line(40);
    printf("  当前昵称: %s\n  当前简介: %s\n", cur_user->nickname, cur_user->bio);
    input_str("新昵称(回车不改)", cur_user->nickname, 32);
    input_str("新简介(回车不改)", cur_user->bio, 128);
    input_str("头像文件名", cur_user->avatar, 64);
    data_save();
    printf("资料已更新\n");
}

/* ---------- 地址管理 ---------- */
static void addr_menu(void) {
    while (1) {
        print_line(40);
        printf("  收货地址管理\n");
        for (int i = 0; i < cur_user->addr_cnt; i++)
            printf("  [%d]%s %s\n", i+1, cur_user->addrs[i],
                   i == cur_user->def_addr ? "(默认)" : "");
        printf("  [A]新增 [D]删除 [S]设为默认 [0]返回\n");
        char op[4]; input_str(">", op, 4);
        if (op[0]=='0') break;
        else if (op[0]=='A'||op[0]=='a') {
            if (cur_user->addr_cnt >= MAX_ADDR) { printf("地址已满\n"); continue; }
            input_str("地址", cur_user->addrs[cur_user->addr_cnt], 128);
            cur_user->addr_cnt++;
            data_save();
        } else if (op[0]=='D'||op[0]=='d') {
            int idx = input_int("删除序号") - 1;
            if (idx < 0 || idx >= cur_user->addr_cnt) continue;
            for (int i = idx; i < cur_user->addr_cnt-1; i++)
                memcpy(cur_user->addrs[i], cur_user->addrs[i+1], 128);
            cur_user->addr_cnt--;
            data_save();
        } else if (op[0]=='S'||op[0]=='s') {
            int idx = input_int("设为默认序号") - 1;
            if (idx >= 0 && idx < cur_user->addr_cnt)
                cur_user->def_addr = idx;
            data_save();
        }
    }
}

/* ---------- 我的收藏 ---------- */
static void my_favorites(void) {
    print_line(40);
    if (cur_user->fav_cnt == 0) { printf("  暂无收藏\n"); return; }
    for (int i = 0; i < cur_user->fav_cnt; i++) {
        Goods *g = find_goods_by_id(cur_user->favorites[i]);
        if (g) printf("  [%d] %s  ¥%.2f  [%s]\n", g->id, g->title, g->price,
                       goods_status_str(g->status));
    }
}

/* ---------- 浏览历史 ---------- */
static void my_history(void) {
    print_line(40);
    if (cur_user->hist_cnt == 0) { printf("  暂无浏览历史\n"); return; }
    for (int i = cur_user->hist_cnt-1; i >= 0; i--) {
        Goods *g = find_goods_by_id(cur_user->history[i]);
        if (g) printf("  [%d] %s  ¥%.2f\n", g->id, g->title, g->price);
    }
}

/* ---------- 消息通知 ---------- */
static void my_notifs(void) {
    print_line(40);
    int cnt = 0;
    for (int i = 0; i < db.notif_cnt; i++) {
        Notification *n = &db.notifs[i];
        if (n->user_id != cur_user->id) continue;
        const char *tp = n->type==MSG_ORDER?"[订单]":n->type==MSG_COMMENT?"[留言]":"[系统]";
        printf("  %s %s %s\n", tp, n->content, n->is_read?"":"[未读]");
        n->is_read = 1;
        cnt++;
    }
    if (!cnt) printf("  暂无消息\n");
    data_save();
}

/* ---------- 我发布的商品 ---------- */
static void my_goods(void) {
    print_line(40);
    int cnt = 0;
    for (int i = 0; i < db.goods_cnt; i++) {
        Goods *g = &db.goods[i];
        if (g->seller_id != cur_user->id) continue;
        printf("  [%d] %s  ¥%.2f  [%s]\n", g->id, g->title, g->price,
               goods_status_str(g->status));
        cnt++;
    }
    if (!cnt) { printf("  暂无商品\n"); return; }

    int gid = input_int("输入商品ID操作(0取消)");
    if (!gid) return;
    Goods *g = find_goods_by_id(gid);
    if (!g || g->seller_id != cur_user->id) { printf("无效商品\n"); return; }

    printf("  [1]上架 [2]下架 [3]编辑标题/价格 [4]删除\n");
    int op = input_int("操作");
    if (op == 1) { g->status = GOODS_PENDING; printf("已提交审核\n"); }
    else if (op == 2) { g->status = GOODS_OFF; printf("已下架\n"); }
    else if (op == 3) {
        input_str("新标题", g->title, 64);
        g->price = input_double("新价格");
        printf("已更新\n");
    } else if (op == 4) {
        g->status = GOODS_OFF;
        /* 标记删除：直接置卖家id=0 */
        g->seller_id = 0;
        printf("已删除\n");
    }
    data_save();
}

/* ---------- 我的订单（买/卖） ---------- */
static void my_orders(int as_buyer) {
    print_line(40);
    printf("  %s订单\n", as_buyer?"买入":"卖出");
    int cnt = 0;
    for (int i = 0; i < db.order_cnt; i++) {
        Order *o = &db.orders[i];
        int match = as_buyer ? (o->buyer_id == cur_user->id) : (o->seller_id == cur_user->id);
        if (!match) continue;
        Goods *g = find_goods_by_id(o->goods_id);
        printf("  订单%d | %s | ¥%.2f | %s\n", o->id,
               g ? g->title : "商品已删除", o->amount,
               order_status_str(o->status));
        cnt++;
    }
    if (!cnt) printf("  暂无订单\n");
}

/* ---------- 聊天 ---------- */
static void chat_menu(void) {
    int peer_id = input_int("对方用户ID(0取消)");
    if (!peer_id) return;
    User *peer = find_user_by_id(peer_id);
    if (!peer) { printf("用户不存在\n"); return; }

    /* 显示历史 */
    print_line(40);
    printf("  与 %s 的聊天记录\n", peer->nickname);
    for (int i = 0; i < db.chat_cnt; i++) {
        ChatMsg *m = &db.chats[i];
        if (!((m->from_id == cur_user->id && m->to_id == peer_id) ||
              (m->from_id == peer_id && m->to_id == cur_user->id))) continue;
        User *fu = find_user_by_id(m->from_id);
        printf("  [%s] %s\n", fu ? fu->nickname : "?", m->content);
    }
    print_line(40);

    char msg[256];
    input_str("发送消息(0取消)", msg, 256);
    if (strcmp(msg, "0") == 0) return;

    if (db.chat_cnt >= MAX_CHATS) { printf("聊天记录已满\n"); return; }
    ChatMsg *cm = &db.chats[db.chat_cnt++];
    cm->id = db.next_chat_id++;
    cm->from_id = cur_user->id;
    cm->to_id = peer_id;
    strncpy(cm->content, msg, 255);
    cm->goods_id = 0;
    cm->is_read = 0;
    cm->send_time = time(NULL);
    push_notif(peer_id, MSG_COMMENT, msg);
    data_save();
    printf("发送成功\n");
}

/* ---------- 举报 ---------- */
static void do_report(void) {
    printf("  [1]举报用户 [2]举报商品\n");
    int tp = input_int("选择");
    int tid = input_int("目标ID");
    char reason[128];
    input_str("举报原因", reason, 128);

    if (db.report_cnt >= MAX_REPORTS) return;
    Report *r = &db.reports[db.report_cnt++];
    r->id = db.next_report_id++;
    r->reporter_id = cur_user->id;
    r->target_id = tid;
    r->is_goods = (tp == 2);
    strncpy(r->reason, reason, 127);
    r->handled = 0;
    r->report_time = time(NULL);
    data_save();
    printf("举报已提交\n");
}

/* ---------- 拉黑 ---------- */
static void do_blacklist(void) {
    printf("  [1]拉黑用户 [2]取消拉黑 [3]查看黑名单\n");
    int op = input_int("选择");
    if (op == 3) {
        for (int i = 0; i < cur_user->black_cnt; i++) {
            User *u = find_user_by_id(cur_user->blacklist[i]);
            printf("  ID:%d %s\n", cur_user->blacklist[i], u ? u->nickname : "?");
        }
        return;
    }
    int uid = input_int("用户ID");
    if (op == 1) {
        if (cur_user->black_cnt >= MAX_BLACKLIST) { printf("黑名单已满\n"); return; }
        cur_user->blacklist[cur_user->black_cnt++] = uid;
        printf("已拉黑\n");
    } else {
        for (int i = 0; i < cur_user->black_cnt; i++) {
            if (cur_user->blacklist[i] == uid) {
                for (int j = i; j < cur_user->black_cnt-1; j++)
                    cur_user->blacklist[j] = cur_user->blacklist[j+1];
                cur_user->black_cnt--;
                printf("已取消拉黑\n");
                return;
            }
        }
        printf("未找到\n");
    }
    data_save();
}

/* ---------- 查看我的评价 ---------- */
static void my_reviews(void) {
    print_line(40);
    int cnt = 0;
    for (int i = 0; i < db.review_cnt; i++) {
        Review *r = &db.reviews[i];
        if (r->to_id != cur_user->id) continue;
        User *fu = find_user_by_id(r->from_id);
        const char *sc = r->score==REVIEW_GOOD?"好评":r->score==REVIEW_BAD?"差评":"中评";
        printf("  [%s] %s -- %s\n", sc, r->content, fu ? fu->nickname : "?");
        cnt++;
    }
    if (!cnt) printf("  暂无评价\n");
}

/* ---------- 用户主菜单 ---------- */
void user_menu(void) {
    int ch;
    while (1) {
        print_line(40);
        if (!cur_user) {
            printf("  [1]注册  [2]登录  [0]退出\n");
            ch = input_int("选择");
            if (ch == 1) do_register();
            else if (ch == 2) do_login();
            else return;
        } else {
            printf("  用户: %s%s  余额: ¥%.2f\n",
                   cur_user->nickname,
                   cur_user->verified ? "[已认证]" : "",
                   cur_user->balance);
            printf("  [1]个人资料  [2]实名认证  [3]地址管理\n");
            printf("  [4]我的商品  [5]买入订单  [6]卖出订单\n");
            printf("  [7]我的收藏  [8]浏览历史  [9]消息通知\n");
            printf("  [A]聊天      [B]我的评价  [C]举报\n");
            printf("  [D]黑名单    [E]注销登录  [0]返回\n");
            char op[4]; input_str(">", op, 4);
            ch = op[0];
            if (ch=='1') edit_profile();
            else if (ch=='2') do_verify();
            else if (ch=='3') addr_menu();
            else if (ch=='4') my_goods();
            else if (ch=='5') my_orders(1);
            else if (ch=='6') my_orders(0);
            else if (ch=='7') my_favorites();
            else if (ch=='8') my_history();
            else if (ch=='9') my_notifs();
            else if (ch=='A'||ch=='a') chat_menu();
            else if (ch=='B'||ch=='b') my_reviews();
            else if (ch=='C'||ch=='c') do_report();
            else if (ch=='D'||ch=='d') do_blacklist();
            else if (ch=='E'||ch=='e') { cur_user = NULL; printf("已注销\n"); }
            else if (ch=='0') return;
        }
    }
}
