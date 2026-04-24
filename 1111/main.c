/*
 * main.c  -- 校园二手交易平台 主程序
 *
 * 编译: gcc main.c data.c user.c goods.c order.c -o trade
 * 运行: ./trade  (Windows: trade.exe)
 */
#include "utils.h"

int main(void) {
    data_load();

    printf("\n");
    printf("  ╔══════════════════════════════════════╗\n");
    printf("  ║       校园二手交易平台 v1.0           ║\n");
    printf("  ║   Campus Second-hand Trading System  ║\n");
    printf("  ╚══════════════════════════════════════╝\n");

    int ch;
    while (1) {
        print_line(42);
        if (cur_user)
            printf("  当前用户: %s%s\n",
                   cur_user->nickname,
                   cur_user->is_admin ? " [管理员]" : "");
        else
            printf("  [未登录]\n");
        printf("  [1] 用户中心    [2] 商品广场\n");
        printf("  [3] 我的订单    [4] 管理后台\n");
        printf("  [0] 退出程序\n");
        print_line(42);
        ch = input_int("选择");

        if (ch == 1)      user_menu();
        else if (ch == 2) goods_menu();
        else if (ch == 3) order_menu();
        else if (ch == 4) admin_menu();
        else if (ch == 0) {
            data_save();
            printf("  数据已保存，再见！\n");
            break;
        }
    }
    return 0;
}
