/*
 * 校园二手交易平台 - 主入口
 * 编译: gcc -o server main.c server.c user.c product.c order.c chat.c admin.c utils.c -lws2_32
 * 运行: ./server 8080
 */
#include "server.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int port = 8080;
    if (argc > 1) port = atoi(argv[1]);

    /* 初始化数据目录 */
    init_data_dir();

    printf("校园二手交易平台启动，端口: %d\n", port);
    printf("访问: http://localhost:%d\n", port);

    start_server(port);
    return 0;
}
