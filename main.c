#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char *raw_data;
    int value;
    int is_computed;
    int is_error;
} Cell;

Cell **table;
char **col_names;
int *row_ids;
int rows_count = 0, cols_count = 0;

int get_col_index(const char *name) {
    for (int i = 0; i < cols_count; i++) {
        if (strcmp(col_names[i], name) == 0) return i;
    }
    return -1;
}

int get_row_index(int id) {
    for (int i = 0; i < rows_count; i++) {
        if (row_ids[i] == id) return i;
    }
    return -1;
}

int evaluate(int r, int c);

int get_val(char *arg, int *error) {
    if (isdigit(arg[0]) || (arg[0] == '-' && isdigit(arg[1]))) {
        return atoi(arg);
    }
    char col_name[16];
    int row_id, i = 0;
    while (arg[i] && !isdigit(arg[i])) {
        col_name[i] = arg[i];
        i++;
    }
    col_name[i] = '\0';
    row_id = atoi(&arg[i]);

    int ci = get_col_index(col_name);
    int ri = get_row_index(row_id);

    if (ci == -1 || ri == -1) {
        *error = 1;
        return 0;
    }
    return evaluate(ri, ci);
}

int evaluate(int r, int c) {
    if (table[r][c].is_computed) return table[r][c].value;
    if (table[r][c].is_error) return 0;

    char *data = table[r][c].raw_data;
    if (data[0] != '=') {
        table[r][c].value = atoi(data);
        table[r][c].is_computed = 1;
        return table[r][c].value;
    }

    // Помечаем как ошибку на случай зацикливания
    table[r][c].is_error = 1;

    char arg1[32], arg2[32], op;
    int offset = 1, error = 0;
    
    // Простой парсинг: ARG1 OP ARG2
    int i = 0;
    while (data[offset] && !strchr("+-*/", data[offset])) {
        arg1[i++] = data[offset++];
    }
    arg1[i] = '\0';
    op = data[offset++];
    strcpy(arg2, &data[offset]);

    int v1 = get_val(arg1, &error);
    int v2 = get_val(arg2, &error);

    if (!error) {
        if (op == '+') table[r][c].value = v1 + v2;
        else if (op == '-') table[r][c].value = v1 - v2;
        else if (op == '*') table[r][c].value = v1 * v2;
        else if (op == '/') {
            if (v2 == 0) error = 1;
            else table[r][c].value = v1 / v2;
        }
        if (!error) {
            table[r][c].is_computed = 1;
            table[r][c].is_error = 0;
        }
    }
    return table[r][c].value;
}

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    FILE *f = fopen(argv[1], "r");
    if (!f) return 1;

    char line[1024];
    // Читаем заголовки колонок
    fgets(line, sizeof(line), f);
    char *token = strtok(line, ",\n"); // Пропускаем пустую ячейку [cite: 13]
    while ((token = strtok(NULL, ",\n"))) {
        col_names = realloc(col_names, sizeof(char*) * (cols_count + 1));
        col_names[cols_count++] = strdup(token);
    }

    // Читаем данные
    while (fgets(line, sizeof(line), f)) {
        table = realloc(table, sizeof(Cell*) * (rows_count + 1));
        table[rows_count] = calloc(cols_count, sizeof(Cell));
        row_ids = realloc(row_ids, sizeof(int) * (rows_count + 1));
        
        token = strtok(line, ",\n");
        row_ids[rows_count] = atoi(token);
        
        for (int i = 0; i < cols_count; i++) {
            token = strtok(NULL, ",\n");
            table[rows_count][i].raw_data = strdup(token ? token : "0");
        }
        rows_count++;
    }
    fclose(f);

    // Вычисляем и печатаем
    printf(",");
    for (int i = 0; i < cols_count; i++) printf("%s%c", col_names[i], i == cols_count - 1 ? '\n' : ',');
    
    for (int i = 0; i < rows_count; i++) {
        printf("%d,", row_ids[i]);
        for (int j = 0; j < cols_count; j++) {
            evaluate(i, j);
            if (table[i][j].is_error) printf("#ERR");
            else printf("%d", table[i][j].value);
            printf("%c", j == cols_count - 1 ? '\n' : ',');
        }
    }
    return 0;
}