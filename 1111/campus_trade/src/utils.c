#include "server.h"

/* ===== 工具函数实现 ===== */

void get_time_str(char *buf) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
}

/* URL解码 */
char *url_decode(const char *src, char *dst, int dlen) {
    int i = 0, j = 0;
    while (src[i] && j < dlen-1) {
        if (src[i] == '%' && src[i+1] && src[i+2]) {
            char hex[3] = {src[i+1], src[i+2], 0};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' '; i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = 0;
    return dst;
}

/* 从query string或body获取参数 */
char *get_param(const char *query, const char *key, char *val, int vlen) {
    char kbuf[128];
    snprintf(kbuf, sizeof(kbuf), "%s=", key);
    const char *p = strstr(query, kbuf);
    if (!p) { val[0]=0; return val; }
    p += strlen(kbuf);
    int i = 0;
    while (*p && *p != '&' && i < vlen-1) val[i++] = *p++;
    val[i] = 0;
    url_decode(val, val, vlen);
    return val;
}

char *get_body_param(const char *body, const char *key, char *val, int vlen) {
    return get_param(body, key, val, vlen);
}

void resp_json(HttpResp *resp, int status, const char *json) {
    resp->status = status;
    strcpy(resp->content_type, "application/json; charset=utf-8");
    strncpy(resp->body, json, MAX_BUF-1);
    resp->body_len = strlen(resp->body);
}

void resp_ok(HttpResp *resp, const char *msg) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"code\":0,\"msg\":\"%s\"}", msg);
    resp_json(resp, 200, buf);
}

void resp_err(HttpResp *resp, const char *msg) {
    char buf[512];
    snprintf(buf, sizeof(buf), "{\"code\":1,\"msg\":\"%s\"}", msg);
    resp_json(resp, 200, buf);
}

/* ===== 文件数据库 ===== */
/* 每行用|分隔字段，第一个字段为ID */

static char db_path[MAX_PATH];
static void mk_path(const char *file) {
    snprintf(db_path, sizeof(db_path), "%s/%s", DATA_DIR, file);
}

int gen_id(const char *file) {
    mk_path(file);
    FILE *f = fopen(db_path, "r");
    int max_id = 0, id;
    char line[MAX_LINE];
    if (f) {
        while (fgets(line, sizeof(line), f)) {
            id = atoi(line);
            if (id > max_id) max_id = id;
        }
        fclose(f);
    }
    return max_id + 1;
}

int db_append(const char *file, const char *line) {
    mk_path(file);
    FILE *f = fopen(db_path, "a");
    if (!f) return -1;
    fprintf(f, "%s\n", line);
    fclose(f);
    return 0;
}

/* 在file中查找field字段==key的行，返回整行到out */
int db_find_line(const char *file, const char *key, int field, char *out, int olen) {
    mk_path(file);
    FILE *f = fopen(db_path, "r");
    if (!f) return 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char tmp[MAX_LINE];
        strncpy(tmp, line, sizeof(tmp)-1);
        char *p = tmp, *tok;
        int fi = 0;
        while ((tok = strsep(&p, "|")) != NULL) {
            if (fi == field && strcmp(tok, key) == 0) {
                strncpy(out, line, olen-1);
                fclose(f);
                return 1;
            }
            fi++;
        }
    }
    fclose(f);
    return 0;
}

/* 获取行中第n个字段 */
static void get_field(const char *line, int n, char *out, int olen) {
    char tmp[MAX_LINE];
    strncpy(tmp, line, sizeof(tmp)-1);
    char *p = tmp, *tok;
    int fi = 0;
    while ((tok = strsep(&p, "|")) != NULL) {
        if (fi == n) { strncpy(out, tok, olen-1); return; }
        fi++;
    }
    out[0] = 0;
}

int db_update_field(const char *file, int id_field, int id_val,
                     int upd_field, const char *new_val) {
    mk_path(file);
    char tmp_path[MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", db_path);
    FILE *f = fopen(db_path, "r");
    FILE *fw = fopen(tmp_path, "w");
    if (!f || !fw) { if(f)fclose(f); if(fw)fclose(fw); return -1; }
    char line[MAX_LINE], id_str[32];
    snprintf(id_str, sizeof(id_str), "%d", id_val);
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        char fval[64];
        get_field(line, id_field, fval, sizeof(fval));
        if (strcmp(fval, id_str) == 0) {
            /* 重建行 */
            char tmp[MAX_LINE]; strncpy(tmp, line, sizeof(tmp)-1);
            char *p = tmp, *tok;
            int fi = 0, first = 1;
            char newline[MAX_LINE] = "";
            while ((tok = strsep(&p, "|")) != NULL) {
                if (!first) strcat(newline, "|");
                first = 0;
                if (fi == upd_field) strcat(newline, new_val);
                else strcat(newline, tok);
                fi++;
            }
            fprintf(fw, "%s\n", newline);
        } else {
            fprintf(fw, "%s\n", line);
        }
    }
    fclose(f); fclose(fw);
    remove(db_path);
    rename(tmp_path, db_path);
    return 0;
}

int db_count(const char *file) {
    mk_path(file);
    FILE *f = fopen(db_path, "r");
    if (!f) return 0;
    int cnt = 0;
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] > ' ') cnt++;
    }
    fclose(f);
    return cnt;
}

/* 将文件内容以JSON数组形式输出（简化，直接输出行数组） */
void db_list(const char *file, char *out, int olen, int limit, int offset) {
    mk_path(file);
    FILE *f = fopen(db_path, "r");
    strcpy(out, "[");
    if (!f) { strcat(out, "]"); return; }
    char line[MAX_LINE];
    int cnt = 0, idx = 0;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line,"\n")] = 0;
        if (!line[0]) continue;
        if (idx++ < offset) continue;
        if (limit > 0 && cnt >= limit) break;
        if (cnt > 0) strncat(out, ",", olen - strlen(out) - 1);
        char item[MAX_LINE + 4];
        snprintf(item, sizeof(item), "\"%s\"", line);
        strncat(out, item, olen - strlen(out) - 1);
        cnt++;
    }
    fclose(f);
    strncat(out, "]", olen - strlen(out) - 1);
}

/* ===== Session管理 ===== */
static char session_path[MAX_PATH];

int save_session(int user_id, char *sid_out) {
    /* 生成简单session id */
    srand((unsigned)time(NULL) ^ (unsigned)user_id);
    snprintf(sid_out, 64, "%08x%08x", rand(), rand());
    snprintf(session_path, sizeof(session_path), "%s/sessions.dat", DATA_DIR);
    char line[128];
    snprintf(line, sizeof(line), "%s|%d", sid_out, user_id);
    return db_append("sessions.dat", line);
}

int load_session(const char *sid) {
    char line[128];
    if (!db_find_line("sessions.dat", sid, 0, line, sizeof(line))) return 0;
    char uid_str[32];
    get_field(line, 1, uid_str, sizeof(uid_str));
    return atoi(uid_str);
}
