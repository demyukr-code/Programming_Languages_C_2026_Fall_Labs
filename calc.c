// KRISTINA DEMINSKA 251ADB152
// Arithmetic Parser in C; submission file: calc.c
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c -lm
// Resources used: OpenAI ChatGPT (code generation and testing; history: ADD_CHAT_LINK_HERE); supplied lab instructions.

/* AI (ChatGPT): this implementation was generated with AI assistance.
   Original prompt: "Help me write the challenging parts of an assignment on an arithmetic parser in C: the tokenizer `next()`, expression parsing with `primary()`, `term()`, and `expression()`, exponentiation with `power()`, and evaluation with `calculate()`. The parser must correctly handle operator precedence, evaluate `**` from right to left, distinguish between integers and floating-point numbers, and report the exact position of an error. Also, help me implement `process_file()`: if the file contains an error, the output must contain only `ERROR:<pos>`, without any results from earlier expressions. Follow the assignment requirements.",
   followed by the full Arithmetic Parser in C assignment.
   The AI notes near the functions refer to this original prompt.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define NAME "kristina"
#define LASTNAME "deminska"
#define STUDENT_ID "251adb152"

#define NUMBER 256
#define POWER 257

typedef struct {
    int floating;
    long long integer;
    double real;
} Value;

typedef struct {
    char *line;
    size_t length, index, offset, position, error;
    int token;
    Value number;
} Parser;

static void *resize(void *old, size_t bytes)
{
    void *p = realloc(old, bytes);
    if (!p) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return p;
}

static void fail(Parser *p, size_t position)
{
    /* Keep the first failure: later parser checks must not replace it. */
    if (!p->error) p->error = position;
}

/* AI (ChatGPT), original prompt in the header: tokenizer and positions.
   The offset counts all previous lines, including their
   newlines. Adding index + 1 gives an absolute, one-based token position.
   End of line is a token too: its position is the newline, or EOF + 1. */
static void next(Parser *p)
{
    char *s = p->line;
    while (p->index < p->length &&
           (s[p->index] == ' ' || s[p->index] == '\t')) p->index++;
    p->position = p->offset + p->index + 1;
    if (p->index == p->length) {
        p->token = 0;
        return;
    }
    size_t start = p->index;
    unsigned char c = (unsigned char)s[p->index++];
    p->token = c ? c : '?';
    if (isdigit(c)) {
        while (p->index < p->length &&
               isdigit((unsigned char)s[p->index])) p->index++;
        int floating = 0;
        if (p->index + 1 < p->length && s[p->index] == '.' &&
            isdigit((unsigned char)s[p->index + 1])) {
            floating = 1;
            p->index++;
            while (p->index < p->length &&
                   isdigit((unsigned char)s[p->index])) p->index++;
        }
        /* Temporarily terminate the token so strtod cannot accept an
           exponent such as e3, which this lab does not allow. A trailing
           dot stays a separate invalid token, giving its exact position. */
        char saved = s[p->index];
        s[p->index] = '\0';
        p->number = (Value){0, 0, 0};
        p->number.floating = floating;
        errno = 0;
        if (floating) {
            p->number.real = strtod(s + start, NULL);
            if (!isfinite(p->number.real)) fail(p, p->position);
        } else {
            p->number.integer = strtoll(s + start, NULL, 10);
            if (errno == ERANGE) fail(p, p->position);
        }
        s[p->index] = saved;
        p->token = NUMBER;
    } else if (c == '*' && p->index < p->length && s[p->index] == '*') {
        p->index++;
        p->token = POWER;
    }
}

/* AI (ChatGPT), original prompt in the header: arithmetic and errors.
   A float flag is kept even when a double happens to
   equal an integer, because it changes later division and exponentiation.
   Zero division and negative integer powers belong to the right operand;
   a non-finite floating result belongs to the operator. */
static Value calculate(Parser *p, Value a, Value b, int op,
                       size_t operator_pos, size_t right_pos)
{
    Value result = {0, 0, 0};
    if (p->error) return result;
    if (op == '/' && (b.floating ? b.real == 0.0 : b.integer == 0)) {
        fail(p, right_pos);
        return result;
    }
    if (a.floating || b.floating) {
        double x = a.floating ? a.real : (double)a.integer;
        double y = b.floating ? b.real : (double)b.integer;
        result.floating = 1;
        switch (op) {
            case '+': result.real = x + y; break;
            case '-': result.real = x - y; break;
            case '*': result.real = x * y; break;
            case '/': result.real = x / y; break;
            default: result.real = pow(x, y); break;
        }
        if (!isfinite(result.real)) fail(p, operator_pos);
    } else {
        switch (op) {
            case '+': result.integer = a.integer + b.integer; break;
            case '-': result.integer = a.integer - b.integer; break;
            case '*': result.integer = a.integer * b.integer; break;
            case '/': result.integer = a.integer / b.integer; break;
            default: {
                if (b.integer < 0) {
                    fail(p, right_pos);
                    break;
                }
                long long base = a.integer, exponent = b.integer;
                result.integer = 1;
                /* Squaring makes even large exponents quick. Avoid the
                   unused final square: it could overflow unnecessarily.
                   The assignment guarantees no integer overflow. */
                while (exponent) {
                    if (exponent % 2) result.integer *= base;
                    exponent /= 2;
                    if (exponent) base *= base;
                }
            }
        }
    }
    return result;
}

static Value expression(Parser *p);

/* AI (ChatGPT), original prompt in the header: numbers and parentheses.
   A parenthesized expression uses the full grammar again. If an operand
   or closing parenthesis is missing, the current token marks the error. */
static Value primary(Parser *p)
{
    Value result = {0, 0, 0};
    if (p->error) return result;
    if (p->token == NUMBER) {
        result = p->number;
        next(p);
    } else if (p->token == '(') {
        next(p);
        result = expression(p);
        if (!p->error) {
            if (p->token != ')') fail(p, p->position);
            else next(p);
        }
    } else {
        /* This covers unary signs, empty parentheses, invalid characters
           and a missing operand. All point at the current token. */
        fail(p, p->position);
    }
    return result;
}

/* AI (ChatGPT), original prompt in the header: right-associative powers.
   Each level calls the stronger level first, so powers
   bind before multiplication, which binds before addition. Power operands
   are reduced from right to left, giving 2**(3**2). An allocated array
   avoids exhausting the call stack on a very long chain of powers. */
static Value power(Parser *p)
{
    typedef struct { Value value; size_t op, right; } Pending;
    Pending *items = NULL;
    size_t count = 0, capacity = 0;
    Value result = primary(p);
    while (!p->error && p->token == POWER) {
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 8;
            items = resize(items, capacity * sizeof(*items));
        }
        items[count].value = result;
        items[count].op = p->position;
        next(p);
        items[count].right = p->position;
        count++;
        result = primary(p);
    }
    while (count && !p->error) {
        count--;
        result = calculate(p, items[count].value, result, POWER,
                           items[count].op, items[count].right);
    }
    free(items);
    return result;
}

/* AI (ChatGPT), original prompt in the header: multiplication and division.
   Reading a power first gives ** higher precedence. Reducing each pair
   immediately makes * and / group from left to right. */
static Value term(Parser *p)
{
    Value result = power(p);
    while (!p->error && (p->token == '*' || p->token == '/')) {
        int op = p->token;
        size_t pos = p->position;
        next(p);
        size_t right = p->position;
        Value b = power(p);
        result = calculate(p, result, b, op, pos, right);
    }
    return result;
}

/* AI (ChatGPT), original prompt in the header: addition and subtraction.
   Each operand is a complete term, so multiplication and division are
   evaluated before addition and subtraction. */
static Value expression(Parser *p)
{
    Value result = term(p);
    /* Immediate reduction makes +, -, * and / left-associative. */
    while (!p->error && (p->token == '+' || p->token == '-')) {
        int op = p->token;
        size_t pos = p->position;
        next(p);
        size_t right = p->position;
        Value b = term(p);
        result = calculate(p, result, b, op, pos, right);
    }
    return result;
}

/* Read one line at a time and double its capacity when
   needed. This avoids a fixed line limit and repeated copying of the
   whole file. The buffer is reused for all following lines. */
static int read_line(FILE *in, char **line, size_t *capacity,
                     size_t *length, int *newline)
{
    int c;
    *length = 0;
    *newline = 0;
    while ((c = fgetc(in)) != EOF) {
        if (c == '\n') { *newline = 1; break; }
        if (*length + 1 >= *capacity) {
            *capacity = *capacity ? *capacity * 2 : 256;
            *line = resize(*line, *capacity);
        }
        (*line)[(*length)++] = (char)c;
    }
    if (!*capacity) {
        *capacity = 256;
        *line = resize(NULL, *capacity);
    }
    (*line)[*length] = '\0';
    return c != EOF || *length != 0;
}

static char *base_name(const char *path, int strip_txt)
{
    size_t end = strlen(path);
    while (end > 1 && path[end - 1] == '/') end--;
    size_t start = end;
    while (start && path[start - 1] != '/') start--;
    size_t n = end - start;
    if (strip_txt && n >= 4 && !memcmp(path + end - 4, ".txt", 4)) n -= 4;
    char *base = resize(NULL, n + 1);
    memcpy(base, path + start, n);
    base[n] = '\0';
    return base;
}

static char *join(const char *dir, const char *file)
{
    size_t n = strlen(dir) + strlen(file) + 2;
    char *path = resize(NULL, n);
    snprintf(path, n, "%s/%s", dir, file);
    return path;
}

/* AI (ChatGPT), original prompt in the header: file processing.
   Results stay in a growing buffer until the entire input is checked.
   Thus a later error replaces all earlier results. Doubling the buffer
   keeps total copying linear in the amount of output. */
static int process_file(const char *input, const char *outdir)
{
    FILE *in = fopen(input, "rb");
    if (!in) { perror(input); return 1; }
    char *results = NULL;
    size_t used = 0, result_capacity = 0;
    char *line = NULL;
    size_t capacity = 0, length, offset = 0, error = 0;
    int newline, bad = 0;
    while (read_line(in, &line, &capacity, &length, &newline)) {
        size_t first = 0;
        while (first < length && (line[first] == ' ' || line[first] == '\t')) first++;
        if (first < length && line[first] != '#') {
            Parser p = {0};
            p.line = line;
            p.length = length;
            p.offset = offset;
            next(&p);
            Value value = expression(&p);
            if (!p.error && p.token != 0) fail(&p, p.position);
            if (p.error) { error = p.error; break; }
            char number[128];
            int written = value.floating ? snprintf(number, sizeof(number), "%.15g\n", value.real)
                                         : snprintf(number, sizeof(number), "%lld\n", value.integer);
            if (written < 0 || (size_t)written >= sizeof(number)) { bad = 1; break; }
            if (used + (size_t)written > result_capacity) {
                result_capacity = result_capacity ? result_capacity * 2 : 256;
                results = resize(results, result_capacity);
            }
            memcpy(results + used, number, (size_t)written);
            used += (size_t)written;
        }
        offset += length + (size_t)newline;
    }
    if (ferror(in)) bad = 1;
    if (fclose(in) != 0) bad = 1;
    free(line);
    if (bad) {
        fprintf(stderr, "Could not read input or store results: %s\n", input);
        free(results);
        return 1;
    }
    char *base = base_name(input, 1);
    size_t n = strlen(base) + strlen(NAME) + strlen(LASTNAME) + strlen(STUDENT_ID) + 8;
    char *filename = resize(NULL, n);
    snprintf(filename, n, "%s_%s_%s_%s.txt", base, NAME, LASTNAME, STUDENT_ID);
    char *path = join(outdir, filename);
    free(base);
    free(filename);
    FILE *out = fopen(path, "wb");
    if (!out) { perror(path); free(path); free(results); return 1; }
    if (error) {
        if (fprintf(out, "ERROR:%zu\n", error) < 0) bad = 1;
    } else if (used && fwrite(results, 1, used, out) != used) {
        bad = 1;
    }
    free(results);
    if (fclose(out) != 0) bad = 1;
    if (bad) fprintf(stderr, "Could not write output: %s\n", path);
    free(path);
    return bad;
}

/* Snapshot directory filenames before creating results:
   if input and output folders are the same, new outputs must not become
   additional inputs during this run. Only regular .txt files qualify. */
int main(int argc, char **argv)
{
    const char *input = NULL, *directory = NULL, *outdir = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--dir")) {
            if (directory || i + 1 == argc) goto usage;
            directory = argv[++i];
        } else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output-dir")) {
            if (outdir || i + 1 == argc) goto usage;
            outdir = argv[++i];
        } else if (argv[i][0] == '-' || input) {
            goto usage;
        } else input = argv[i];
    }
    if ((!input && !directory) || (input && directory)) goto usage;
    struct stat info;
    const char *source = directory ? directory : input;
    if (stat(source, &info) != 0) { perror(source); return 1; }
    if (directory ? !S_ISDIR(info.st_mode) : !S_ISREG(info.st_mode)) {
        fprintf(stderr, "Wrong input type: %s\n", source);
        return 1;
    }
    char **files = NULL;
    size_t count = 0, capacity = 0;
    int bad = 0;
    if (directory) {
        DIR *dir = opendir(directory);
        if (!dir) { perror(directory); return 1; }
        struct dirent *entry;
        for (;;) {
            errno = 0;
            entry = readdir(dir);
            if (!entry) { if (errno) { perror(directory); bad = 1; } break; }
            size_t len = strlen(entry->d_name);
            if (len < 4 || strcmp(entry->d_name + len - 4, ".txt")) continue;
            char *path = join(directory, entry->d_name);
            if (stat(path, &info) != 0) {
                perror(path); free(path); bad = 1; break;
            }
            if (!S_ISREG(info.st_mode)) { free(path); continue; }
            if (count == capacity) {
                capacity = capacity ? capacity * 2 : 16;
                files = resize(files, capacity * sizeof(*files));
            }
            files[count++] = path;
        }
        if (closedir(dir) != 0) { perror(directory); bad = 1; }
    }
    char *default_dir = NULL;
    if (!outdir) {
        char *base = base_name(source, !directory);
        size_t n = strlen(base) + strlen(NAME) + strlen(STUDENT_ID) + 3;
        default_dir = resize(NULL, n);
        snprintf(default_dir, n, "%s_%s_%s", base, NAME, STUDENT_ID);
        free(base);
        outdir = default_dir;
    }
    if (!bad && mkdir(outdir, 0777) != 0) {
        if (errno != EEXIST || stat(outdir, &info) != 0 || !S_ISDIR(info.st_mode)) {
            fprintf(stderr, "Cannot create output directory: %s\n", outdir);
            bad = 1;
        }
    }
    if (!bad) {
        if (input) bad = process_file(input, outdir);
        else for (size_t i = 0; i < count; i++) {
            if (process_file(files[i], outdir)) bad = 1;
        }
    }
    for (size_t i = 0; i < count; i++) free(files[i]);
    free(files);
    free(default_dir);
    return bad ? 1 : 0;

usage:
    fprintf(stderr, "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] [input.txt]\n", argv[0]);
    return 1;
}
