/*
 * order.c  -- 交易模块
 * 包含：下单、付款(担保)、发货、收货、评价、退款、投诉、订单管理
 */
#include "utils.h"

/* ---------- 下单 ---------- */
void place_order(int goods_id) {
    if (!cur_user) { printf("请先登录\n"); return; }
    Goods *g = find_goods_by_id(goods_id);
    if (!g || g->status != GOODS_ON) { printf("商品不可购买\n"); return; }
    if (g->seller_id == cur_user->id) { printf("不能购买自己的商品\n"); return; }

    /* 地址选择 */
    print_line(40);
    printf("  发货方式: [1]快递发货 [2]同城自提\n");
    int ship = input_int("选择");
    char addr[128] = {0};
    if (ship == 1) {
        if (cur_user->addr_cnt == 0) { printf("请先添加收货地址\n"); return; }
        for (int i = 0; i < cur_user->addr_cnt; i++)
            printf("  [%d] %s%s\n", i+1, cur_user->addrs[i],
                   i == cur_user->def_addr ? "(默认)" : "");
        int ai = input_int("选择地址序号") - 1;
        if (ai < 0 || ai >= cur_user->addr_cnt) ai = cur_user->def_addr;
        strncpy(addr, cur_user->addrs[ai], 127);
    } else {
        strncpy(addr, "同城自提", 127);
    }

    printf("  确认购买「%s」¥%.2f？[1]确认 [0]取消\n", g->title, g->price);
    if (input_int("") != 1) return;

    if (db.order_cnt >= MAX_ORDERS) { printf("订单数量已达上限\n"); return; }
    Order *o = &db.orders[db.order_cnt++];
    memset(o, 0, sizeof(Order));
    o->id = db.next_order_id++;
    o->buyer_id = cur_user->id;
    o->seller_id = g->seller_id;
    o->goods_id = goods_id;
    o->amount = g->price;
    o->ship_type = (ship == 2) ? SHIP_SELFPICK : SHIP_EXPRESS;
    strncpy(o->address, addr, 127);
    o->status = ORDER_UNPAID;
    o->create_time = time(NULL);

    data_save();
    printf("下单成功！订单ID: %d，请尽快付款\n", o->id);
}

/* ---------- 付款（平台担保） ---------- */
static void do_pay(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o || o->buyer_id != cur_user->id) { printf("订单不存在\n"); return; }
    if (o->status != ORDER_UNPAID) { printf("订单状态不允许付款\n"); return; }

    printf("  【平台担保交易】款项将暂存平台，确认收货后才释放给卖家\n");
    printf("  付款金额: ¥%.2f  [1]确认付款 [0]取消\n", o->amount);
    if (input_int("") != 1) return;

    /* 扣买家余额（模拟） */
    if (cur_user->balance < o->amount) {
        printf("  余额不足(当前¥%.2f)，请充值\n", cur_user->balance);
        printf("  充值金额: ");
        double top = input_double("");
        cur_user->balance += top;
    }
    cur_user->balance -= o->amount;
    /* 款项锁在订单里，不给卖家 */
    o->status = ORDER_PAID;
    o->pay_time = time(NULL);

    /* 标记商品已售 */
    Goods *g = find_goods_by_id(o->goods_id);
    if (g) g->status = GOODS_SOLD;

    push_notif(o->seller_id, MSG_ORDER, "买家已付款，请尽快发货");
    data_save();
    printf("付款成功！等待卖家发货\n");
}

/* ---------- 发货 ---------- */
static void do_ship(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o || o->seller_id != cur_user->id) { printf("订单不存在\n"); return; }
    if (o->status != ORDER_PAID) { printf("订单状态不允许发货\n"); return; }

    if (o->ship_type == SHIP_EXPRESS) {
        input_str("快递单号", o->tracking, 32);
    } else {
        strcpy(o->tracking, "同城自提");
    }
    o->status = ORDER_SHIPPED;
    o->ship_time = time(NULL);
    push_notif(o->buyer_id, MSG_ORDER, "卖家已发货，请注意查收");
    data_save();
    printf("发货成功！快递单号: %s\n", o->tracking);
}

/* ---------- 确认收货 ---------- */
static void do_confirm_recv(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o || o->buyer_id != cur_user->id) { printf("订单不存在\n"); return; }
    if (o->status != ORDER_SHIPPED) { printf("订单状态不允许确认收货\n"); return; }

    printf("  确认收货？款项将释放给卖家。[1]确认 [0]取消\n");
    if (input_int("") != 1) return;

    o->status = ORDER_RECEIVED;
    o->recv_time = time(NULL);
    /* 释放款给卖家 */
    User *seller = find_user_by_id(o->seller_id);
    if (seller) seller->balance += o->amount;
    push_notif(o->seller_id, MSG_ORDER, "买家已确认收货，款项已到账");
    data_save();
    printf("确认收货成功！请对卖家进行评价\n");
}

/* ---------- 评价 ---------- */
static void do_review(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o) { printf("订单不存在\n"); return; }
    int is_buyer = (o->buyer_id == cur_user->id);
    int is_seller = (o->seller_id == cur_user->id);
    if (!is_buyer && !is_seller) { printf("无权操作\n"); return; }
    if (o->status < ORDER_RECEIVED) { printf("请先确认收货\n"); return; }

    printf("  评价: [1]好评 [0]中评 [-1]差评\n");
    int sc = input_int("评分");
    char content[256]; input_str("评价内容", content, 256);

    if (db.review_cnt >= MAX_REVIEWS) return;
    Review *r = &db.reviews[db.review_cnt++];
    r->id = db.next_review_id++;
    r->order_id = oid;
    r->from_id = cur_user->id;
    r->to_id = is_buyer ? o->seller_id : o->buyer_id;
    r->score = sc > 0 ? REVIEW_GOOD : (sc < 0 ? REVIEW_BAD : REVIEW_MID);
    strncpy(r->content, content, 255);
    r->create_time = time(NULL);

    o->status = ORDER_REVIEWED;
    data_save();
    printf("评价成功\n");
}

/* ---------- 取消订单 ---------- */
static void do_cancel(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o || o->buyer_id != cur_user->id) { printf("订单不存在\n"); return; }
    if (o->status != ORDER_UNPAID) { printf("只能取消未付款订单\n"); return; }
    o->status = ORDER_CANCELLED;
    /* 恢复商品上架 */
    Goods *g = find_goods_by_id(o->goods_id);
    if (g && g->status == GOODS_SOLD) g->status = GOODS_ON;
    data_save();
    printf("订单已取消\n");
}

/* ---------- 退款申请 ---------- */
static void do_refund(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o || o->buyer_id != cur_user->id) { printf("订单不存在\n"); return; }
    if (o->status != ORDER_SHIPPED && o->status != ORDER_RECEIVED) {
        printf("当前状态不允许申请退款\n"); return;
    }
    input_str("退款原因", o->refund_reason, 128);
    o->status = ORDER_REFUND;
    push_notif(o->seller_id, MSG_ORDER, "买家申请退款，请处理");
    data_save();
    printf("退款申请已提交，等待卖家/平台处理\n");
}

/* ---------- 投诉 ---------- */
static void do_dispute(void) {
    int oid = input_int("订单ID");
    Order *o = find_order_by_id(oid);
    if (!o) { printf("订单不存在\n"); return; }
    if (o->buyer_id != cur_user->id && o->seller_id != cur_user->id) {
        printf("无权操作\n"); return;
    }
    input_str("投诉描述", o->dispute_desc, 256);
    o->status = ORDER_DISPUTE;
    push_notif(0, MSG_SYSTEM, "有新投诉需处理"); /* 0=管理员 */
    data_save();
    printf("投诉已提交，平台将介入处理\n");
}

/* ---------- 订单列表 ---------- */
static void list_orders(void) {
    if (!cur_user) { printf("请先登录\n"); return; }
    print_line(40);
    printf("  筛选: [0]全部 [1]待付款 [2]待发货 [3]待收货 [4]待评价 [5]已完成 [6]已取消\n");
    int filter = input_int("选择");
    static const int status_map[] = {-1, ORDER_UNPAID, ORDER_PAID, ORDER_SHIPPED,
                                     ORDER_RECEIVED, ORDER_DONE, ORDER_CANCELLED};
    int fstatus = (filter >= 1 && filter <= 6) ? status_map[filter] : -1;

    int cnt = 0;
    for (int i = 0; i < db.order_cnt; i++) {
        Order *o = &db.orders[i];
        if (o->buyer_id != cur_user->id && o->seller_id != cur_user->id) continue;
        if (fstatus >= 0 && o->status != fstatus) continue;
        Goods *g = find_goods_by_id(o->goods_id);
        printf("  订单%d | %s | ¥%.2f | %s | %s\n",
               o->id, g ? g->title : "?", o->amount,
               order_status_str(o->status),
               o->tracking[0] ? o->tracking : "");
        cnt++;
    }
    if (!cnt) printf("  暂无订单\n");
}

/* ---------- 订单主菜单 ---------- */
void order_menu(void) {
    int ch;
    while (1) {
        print_line(40);
        printf("  [1]我的订单  [2]付款  [3]发货  [4]确认收货\n");
        printf("  [5]评价      [6]取消  [7]申请退款  [8]投诉\n");
        printf("  [0]返回\n");
        ch = input_int("选择");
        if (ch == 1) list_orders();
        else if (ch == 2) do_pay();
        else if (ch == 3) do_ship();
        else if (ch == 4) do_confirm_recv();
        else if (ch == 5) do_review();
        else if (ch == 6) do_cancel();
        else if (ch == 7) do_refund();
        else if (ch == 8) do_dispute();
        else if (ch == 0) return;
        pause_screen();
    }
}
