#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int fila;
    int col;
    double valor;
} ElementoEsparso;

static void generar_txt(const char *nom, int f, int c, double dens) {
    FILE *fp = fopen(nom, "w");
    if (fp == NULL) return;

    fprintf(fp, "%d %d\n", f, c);
    int n_elem = (int)(f * c * dens);

    for (int k = 0; k < n_elem; k++) {
        int i = rand() % f;
        int j = rand() % c;
        double v = (rand() % 100) + 1.0;
        fprintf(fp, "%d %d %.2f\n", i, j, v);
    }

    fclose(fp);
}

static void txt_a_bin(const char *txt, const char *bin) {
    FILE *f_txt = fopen(txt, "r");
    FILE *f_bin = fopen(bin, "wb");
    if (f_txt == NULL || f_bin == NULL) {
        if (f_txt) fclose(f_txt);
        if (f_bin) fclose(f_bin);
        return;
    }

    int f = 0, c = 0;
    if (fscanf(f_txt, "%d %d", &f, &c) == 2) {
        fwrite(&f, sizeof(int), 1, f_bin);
        fwrite(&c, sizeof(int), 1, f_bin);

        ElementoEsparso elem;
        while (fscanf(f_txt, "%d %d %lf", &elem.fila, &elem.col, &elem.valor) == 3) {
            fwrite(&elem, sizeof(ElementoEsparso), 1, f_bin);
        }
    }

    fclose(f_txt);
    fclose(f_bin);
}

static void multiplicar_bin(const char *bin_a, const char *bin_b, const char *bin_tmp) {
    FILE *fa = fopen(bin_a, "rb");
    FILE *fb = fopen(bin_b, "rb");
    FILE *ft = fopen(bin_tmp, "wb");
    if (fa == NULL || fb == NULL || ft == NULL) {
        if (fa) fclose(fa);
        if (fb) fclose(fb);
        if (ft) fclose(ft);
        return;
    }

    int fa_rows = 0, fa_cols = 0;
    int fb_rows = 0, fb_cols = 0;

    fread(&fa_rows, sizeof(int), 1, fa);
    fread(&fa_cols, sizeof(int), 1, fa);
    fread(&fb_rows, sizeof(int), 1, fb);
    fread(&fb_cols, sizeof(int), 1, fb);

    fwrite(&fa_rows, sizeof(int), 1, ft);
    fwrite(&fb_cols, sizeof(int), 1, ft);

    ElementoEsparso ea;
    while (fread(&ea, sizeof(ElementoEsparso), 1, fa) == 1) {
        fseek(fb, 2 * sizeof(int), SEEK_SET);
        ElementoEsparso eb;
        while (fread(&eb, sizeof(ElementoEsparso), 1, fb) == 1) {
            if (ea.col == eb.fila) {
                ElementoEsparso res;
                res.fila = ea.fila;
                res.col = eb.col;
                res.valor = ea.valor * eb.valor;
                fwrite(&res, sizeof(ElementoEsparso), 1, ft);
            }
        }
    }

    fclose(fa);
    fclose(fb);
    fclose(ft);
}

static void consolidar_y_exportar(const char *bin_tmp, const char *txt_out) {
    FILE *fb = fopen(bin_tmp, "rb");
    if (fb == NULL) return;

    int rows = 0, cols = 0;
    fread(&rows, sizeof(int), 1, fb);
    fread(&cols, sizeof(int), 1, fb);

    long pos_inicio = ftell(fb);
    fseek(fb, 0, SEEK_END);
    long pos_fin = ftell(fb);
    size_t n_total = (size_t)(pos_fin - pos_inicio) / sizeof(ElementoEsparso);

    fseek(fb, pos_inicio, SEEK_SET);

    ElementoEsparso *arr = (ElementoEsparso *)malloc(n_total * sizeof(ElementoEsparso));
    if (arr == NULL) {
        fclose(fb);
        return;
    }

    fread(arr, sizeof(ElementoEsparso), n_total, fb);
    fclose(fb);

    ElementoEsparso *cons = (ElementoEsparso *)calloc(n_total, sizeof(ElementoEsparso));
    if (cons == NULL) {
        free(arr);
        return;
    }

    size_t n_cons = 0;
    for (size_t i = 0; i < n_total; i++) {
        int enc = 0;
        for (size_t j = 0; j < n_cons; j++) {
            if (cons[j].fila == arr[i].fila && cons[j].col == arr[i].col) {
                cons[j].valor += arr[i].valor;
                enc = 1;
                break;
            }
        }
        if (!enc) {
            cons[n_cons].fila = arr[i].fila;
            cons[n_cons].col = arr[i].col;
            cons[n_cons].valor = arr[i].valor;
            n_cons++;
        }
    }

    FILE *ft = fopen(txt_out, "w");
    if (ft != NULL) {
        fprintf(ft, "%d %d\n", rows, cols);
        for (size_t i = 0; i < n_cons; i++) {
            fprintf(ft, "%d %d %.2f\n", cons[i].fila, cons[i].col, cons[i].valor);
        }
        fclose(ft);
    }

    free(arr);
    free(cons);
}

int main(void) {
    srand((unsigned int)time(NULL));

    int m = 100, n = 100, p = 100;
    double dens = 0.05;

    generar_txt("matriz_a.txt", m, n, dens);
    generar_txt("matriz_b.txt", n, p, dens);

    txt_a_bin("matriz_a.txt", "matriz_a.bin");
    txt_a_bin("matriz_b.txt", "matriz_b.bin");

    multiplicar_bin("matriz_a.bin", "matriz_b.bin", "acumulado.bin");

    consolidar_y_exportar("acumulado.bin", "matriz_axb.txt");

    printf("Procesamiento completado con exito.\n");

    return 0;
}