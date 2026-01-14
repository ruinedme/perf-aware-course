#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

#include "sax_json.h"

#define READ_BUF_SIZE 4096
#define STRING_BUF_INIT 256
#define STACK_INIT 64

typedef enum
{
    CTX_ROOT = 0,
    CTX_OBJECT,
    CTX_ARRAY
} ctx_type_t;
typedef struct
{
    ctx_type_t *data;
    size_t cap;
    size_t len;
} ctx_stack_t;

static bool ctx_stack_init(ctx_stack_t *s)
{
    s->cap = STACK_INIT;
    s->len = 0;
    s->data = malloc(sizeof(ctx_type_t) * s->cap);

    return s->data != NULL;
}

static void ctx_stack_free(ctx_stack_t *s)
{
    free(s->data);
    s->data = NULL;
    s->cap = s->len = 0;
}

static bool ctx_stack_push(ctx_stack_t *s, ctx_type_t v)
{
    if (s->len >= s->cap)
    {
        size_t ncap = s->cap * 2;
        ctx_type_t *n = realloc(s->data, sizeof(ctx_type_t) * ncap);
        if (!n)
            return false;
        s->data = n;
        s->cap = ncap;
    }

    s->data[s->len++] = v;
    return true;
}

static ctx_type_t ctx_stack_pop(ctx_stack_t *s)
{
    if (s->len == 0)
        return CTX_ROOT;
    return s->data[--s->len];
}

static ctx_type_t ctx_stack_top(ctx_stack_t *s)
{
    if (s->len == 0)
        return CTX_ROOT;
    return s->data[s->len - 1];
}

typedef struct
{
    char *buf;
    size_t len;
    size_t cap;
} sbuf_t;

static bool sbuf_init(sbuf_t *s, size_t initcap)
{
    if (initcap == 0)
        initcap = STRING_BUF_INIT;
    s->buf = malloc(initcap);

    if (!s->buf)
        return false;

    s->len = 0;
    s->cap = initcap;
    s->buf[0] = '\0';
    return true;
}

static void sbuf_free(sbuf_t *s)
{
    free(s->buf);
    s->buf = NULL;
    s->len = s->cap = 0;
}

static bool sbuf_append_char(sbuf_t *s, char c)
{
    if (s->len + 1 >= s->cap)
    {
        size_t ncap = s->cap * 2;
        char *n = realloc(s->buf, ncap);
        if (!n)
            return false;
        s->buf = n;
        s->cap = ncap;
    }
    s->buf[s->len++] = c;
    s->buf[s->len] = '\0';
    return true;
}

static bool sbuf_append_bytes(sbuf_t *s, const char *bytes, size_t n)
{
    if (s->len + n >= s->cap)
    {
        size_t ncap = s->cap;
        while (s->len + n >= ncap)
            ncap *= 2;

        char *arr = realloc(s->buf, ncap);
        if (!arr)
            return false;
        s->buf = arr;
        s->cap = ncap;
    }

    memcpy(s->buf + s->len, bytes, n);
    s->len += n;
    s->buf[s->len] = '\0';
    return true;
}

static bool sbuf_append_utf8_codepoint(sbuf_t *s, uint32_t cp)
{
    char buf[4];
    size_t n = 0;
    if (cp <= 0x7f)
    {
        buf[0] = (char)cp;
        n = 1;
    }
    else if (cp <= 0x7ff)
    {
        buf[0] = (char)(0xe0 | ((cp >> 6) & 0x1f));
        buf[1] = (char)(0x80 | (cp & 0x3f));
        n = 2;
    }
    else if (cp <= 0xffff)
    {
        buf[0] = (char)(0xe0 | ((cp >> 12) & 0x0f));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3f));
        buf[2] = (char)(0x80 | (cp & 0x3f));
        n = 3;
    }
    else if (cp <= 0x10ffff)
    {
        buf[0] = (char)(0xf0 | ((cp >> 18) & 0x07));
        buf[1] = (char)(0x80 | ((cp >> 12) & 0x3f));
        buf[2] = (char)(0x80 | ((cp >> 6) & 0x3f));
        buf[3] = (char)(0x80 | (cp & 0x3f));
        n = 4;
    }
    else
    {
        return false;
    }
    return sbuf_append_bytes(s, buf, n);
}

typedef enum
{
    ST_WS,
    ST_VALUE,
    ST_OBJECT_KEY,
    ST_AFTER_KEY,
    ST_AFTER_COLON,
    ST_ARRAY_ELEM,
    ST_STRING,
    ST_STRING_ESC,
    ST_NUMBER,
    ST_TRUE,
    ST_FALSE,
    ST_NULL,
    ST_DONE,
    ST_ERROR
} parse_state_t;

typedef struct
{
    json_sax_handler_t handlers;
    void *user_data;
    ctx_stack_t stack;
    sbuf_t strbuf;
    sbuf_t numbuf;

    parse_state_t state;
    size_t position;

    // For \uXXXX sequences
    int u_remaining;
    uint16_t u_value;
    int expecting_surrogate;
} json_sax_parser_t;

static bool parser_init(json_sax_parser_t *p, const json_sax_handler_t *h, void *ud)
{
    memset(p, 0, sizeof(*p));
    if (!ctx_stack_init(&p->stack))
        return false;
    if (!sbuf_init(&p->strbuf, STRING_BUF_INIT))
    {
        ctx_stack_free(&p->stack);
        return false;
    }
    if (!sbuf_init(&p->numbuf, 64))
    {
        sbuf_free(&p->strbuf);
        ctx_stack_free(&p->stack);
        return false;
    }

    if (h)
        p->handlers = *h;
    p->user_data = ud;
    p->state = ST_WS;
    p->position = 0;
    p->u_remaining = 0;
    p->expecting_surrogate = 0;
    return true;
}

static void parser_free(json_sax_parser_t *p)
{
    sbuf_free(&p->strbuf);
    sbuf_free(&p->numbuf);
    ctx_stack_free(&p->stack);
}

static void call_error(json_sax_parser_t *p, const char *msg)
{
    p->state = ST_ERROR;
    if (p->handlers.error)
        p->handlers.error(p->user_data, msg, p->position);
}

// NOTE
// pretty sure these can just be voids?
// static int emit_string(json_sax_parser_t *p)
// {
//     if (p->handlers.string)
//         p->handlers.string(p->user_data, p->strbuf.buf);
//     p->strbuf.len = 0;
//     if (p->strbuf.cap > 0)
//         p->numbuf.buf[0] = '\0';
//     return 0;
// }

static int emit_number(json_sax_parser_t *p)
{
    if (p->handlers.number)
        p->handlers.number(p->user_data, p->numbuf.buf, p->numbuf.len);
    p->numbuf.len = 0;
    if (p->numbuf.cap > 0)
        p->numbuf.buf[0] = '\0';
    return 0;
}

static int hex_val(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('a' <= c && c <= 'f')
        return 10 + (c - 'a');
    if ('A' <= c && c <= 'F')
        return 10 + (c - 'A');
    return -1;
}

static inline bool iswhitespace(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static bool process_chunk(json_sax_parser_t *parser, const char *buf, size_t buflen, int is_final)
{
    size_t i = 0;
    while (i < buflen)
    {
        char c = buf[i];
        parser->position++;

        switch (parser->state)
        {
        case ST_WS:
        case ST_VALUE:
        {
            if (iswhitespace(c))
            {
                i++;
                continue;
            }
            if (c == '{')
            {
                if (parser->handlers.start_object)
                    parser->handlers.start_object(parser->user_data);
                if (!ctx_stack_push(&parser->stack, CTX_OBJECT))
                {
                    call_error(parser, "stack push failed");
                    return false;
                }
                parser->state = ST_OBJECT_KEY;
                i++;
                continue;
            }
            else if (c == '[')
            {
                if (parser->handlers.start_object)
                    parser->handlers.start_object(parser->user_data);
                if (!ctx_stack_push(&parser->stack, CTX_ARRAY))
                {
                    call_error(parser, "stack push failed");
                    return false;
                }
                parser->state = ST_ARRAY_ELEM;
                i++;
                continue;
            }
            else if (c == '"')
            {
                parser->strbuf.len = 0;
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 't')
            {
                parser->state = ST_TRUE;
                parser->numbuf.len = 0;

                if(!sbuf_append_char(&parser->numbuf, 't')){
                    call_error(parser, "alloc failure");
                    return false;
                }
                i++;
                continue;
            }
            else if (c == 'f')
            {
                parser->state = ST_FALSE;
                parser->numbuf.len = 0;
                if(!sbuf_append_char(&parser->numbuf, 'f')){
                    call_error(parser, "alloc failure");
                    return false;
                }
                i++;
                continue;
            }
            else if (c == 'n')
            {
                parser->state = ST_NULL;
                parser->numbuf.len = 0;
                if(!sbuf_append_char(&parser->numbuf, 'n')){
                    call_error(parser, "alloc failure");
                    return false;
                }
                i++;
                continue;
            }
            else if (c == '-' || (c >= '0' && c <= '9'))
            {
                parser->numbuf.len = 0;
                if (!sbuf_append_char(&parser->numbuf, c))
                {
                    call_error(parser, "alloc failure");
                    return false;
                }
                parser->state = ST_NUMBER;
                i++;
                continue;
            }
            else if (c == ']')
            {
                if (ctx_stack_top(&parser->stack) != CTX_ARRAY)
                {
                    call_error(parser, "unexpected ']'");
                    return false;
                }
                ctx_stack_pop(&parser->stack);
                if (parser->handlers.end_array)
                    parser->handlers.end_array(parser->user_data);
                if (parser->stack.len == 0)
                    parser->state = ST_DONE;
                else
                {
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    parser->state = (top == CTX_ARRAY) ? ST_ARRAY_ELEM : ST_AFTER_COLON;
                }
                i++;
                continue;
            }
            else if (c == '}')
            {
                if (ctx_stack_top(&parser->stack) != CTX_OBJECT)
                {
                    call_error(parser, "unexpected '}'");
                    return false;
                }
                ctx_stack_pop(&parser->stack);
                if (parser->handlers.end_object)
                    parser->handlers.end_object(parser->user_data);
                if (parser->stack.len == 0)
                    parser->state = ST_DONE;
                else
                {
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    parser->state = (top == CTX_ARRAY) ? ST_ARRAY_ELEM : ST_AFTER_COLON;
                }
                i++;
                continue;
            }
            else if (c == ',')
            {
                ctx_type_t top = ctx_stack_top(&parser->stack);
                if (top == CTX_ARRAY)
                    parser->state = ST_ARRAY_ELEM;
                else if (top == CTX_OBJECT)
                    parser->state = ST_OBJECT_KEY;
                else
                {
                    call_error(parser, "unexpected ',' in root");
                    return false;
                }
                i++;
                continue;
            }
            else if (c == ':')
            {
                parser->state = ST_AFTER_COLON;
                i++;
                continue;
            }
            else
            {
                call_error(parser, "unexpected character while parser value");
                return false;
            }
        }
        break;
        case ST_OBJECT_KEY:
        {
            if (iswhitespace(c))
            {
                i++;
                continue;
            }
            if (c == '}')
            {
                if (ctx_stack_top(&parser->stack) != CTX_OBJECT)
                {
                    call_error(parser, "unexpected '}'");
                    return false;
                }
                ctx_stack_pop(&parser->stack);
                if (parser->handlers.end_object)
                    parser->handlers.end_object(parser->user_data);
                if (parser->stack.len == 0)
                    parser->state = ST_DONE;
                else
                {
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    parser->state = (top == CTX_ARRAY) ? ST_ARRAY_ELEM : ST_AFTER_COLON;
                }
                i++;
                continue;
            }
            if (c == '"')
            {
                parser->strbuf.len = 0;
                parser->state = ST_STRING;
                // We use u_remaining in ST_STRING to determine if the string we're parsing is a key or a value
                parser->u_remaining = -1;
                i++;
                continue;
            }
            call_error(parser, "expected object key string");
            return false;
        }
        break;
        case ST_AFTER_COLON:
        {
            if (iswhitespace(c))
            {
                i++;
                continue;
            }
            parser->state = ST_VALUE;
            continue; // re-evaluate the same char in ST_VALUE
        }
        break;
        case ST_ARRAY_ELEM:
        {
            if (iswhitespace(c))
            {
                i++;
                continue;
            }
            if (c == ']')
            {
                if (ctx_stack_top(&parser->stack) != CTX_ARRAY)
                {
                    call_error(parser, "unexpected ']'");
                    return false;
                }
                ctx_stack_pop(&parser->stack);
                if (parser->handlers.end_array)
                    parser->handlers.end_array(parser->user_data);
                if (parser->stack.len == 0)
                    parser->state = ST_DONE;
                else
                {
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    parser->state = (top == CTX_ARRAY) ? ST_ARRAY_ELEM : ST_AFTER_COLON;
                }
                i++;
                continue;
            }
            parser->state = ST_VALUE;
            continue;
        }
        break;
        case ST_STRING:
        {
            // parse until closing " with escape handling and \uXXXX
            if (c == '"')
            {
                bool is_key = false;
                if (parser->u_remaining == -1)
                {
                    is_key = true;
                    parser->u_remaining = 0; // reset the marker
                }
                else
                {
                    is_key = false;
                }

                if (is_key && ctx_stack_top(&parser->stack) == CTX_OBJECT && parser->handlers.key)
                {
                    parser->handlers.key(parser->user_data, parser->strbuf.buf);
                }
                else
                {
                    if (parser->handlers.string)
                        parser->handlers.key(parser->user_data, parser->strbuf.buf);
                }
                parser->strbuf.len = 0;
                parser->state = ST_AFTER_COLON;
                i++;
                continue;
            }
            else if (c == '\\')
            {
                parser->state = ST_STRING_ESC;
                i++;
                continue;
            }
            else
            {
                if (!sbuf_append_char(&parser->strbuf, c))
                {
                    call_error(parser, "alloc failure");
                    return false;
                }
                i++;
                continue;
            }
        }
        break;
        case ST_STRING_ESC:
        {
            if (c == '"' || c == '\\' || c == '/')
            {
                if (!sbuf_append_char(&parser->strbuf, c))
                {
                    call_error(parser, "alloc failure");
                    return false;
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 'b')
            {
                if (!sbuf_append_char(&parser->strbuf, '\b'))
                {
                    call_error(parser, "alloc failure");
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 'f')
            {
                if (!sbuf_append_char(&parser->strbuf, '\f'))
                {
                    call_error(parser, "alloc failure");
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 'n')
            {
                if (!sbuf_append_char(&parser->strbuf, '\n'))
                {
                    call_error(parser, "alloc failure");
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 'r')
            {
                if (!sbuf_append_char(&parser->strbuf, '\r'))
                {
                    call_error(parser, "alloc failure");
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 't')
            {
                if (!sbuf_append_char(&parser->strbuf, '\t'))
                {
                    call_error(parser, "alloc failure");
                }
                parser->state = ST_STRING;
                i++;
                continue;
            }
            else if (c == 'u')
            {
                parser->u_remaining = 4;
                parser->u_value = 0;
                parser->state = ST_STRING;
                // parse \uXXXX (4 hex digits)
                i++;
                // Loop to process this hex code is after the switch
            }
            else
            {
                call_error(parser, "invalid escape in string");
                return false;
            }
        }
        break;
        case ST_NUMBER:
        {
            if ((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.' || c == 'e' || c == 'E')
            {
                if (!sbuf_append_char(&parser->numbuf, c))
                {
                    call_error(parser, "alloc failure");
                    return false;
                }
                i++;
                continue;
            }
            else
            {
                emit_number(parser);
                ctx_type_t top = ctx_stack_top(&parser->stack);
                if (top == CTX_ARRAY)
                    parser->state = ST_ARRAY_ELEM;
                else if (top == CTX_OBJECT)
                    parser->state = ST_AFTER_COLON;
                else
                    parser->state = ST_DONE;
                continue;
            }
        }
        break;
        case ST_TRUE:
        {
            const char *match = "rue";
            size_t have = parser->numbuf.len;
            if (have == 0)
            {
                call_error(parser, "internal true parser error");
                return false;
            }
            size_t expect_index = have - 1;
            if (expect_index >= 3)
            {
                call_error(parser, "internal true length error");
                return false;
            }

            if (c == match[expect_index])
            {
                if (!sbuf_append_char(&parser->numbuf, c))
                {
                    call_error(parser, "alloc failure");
                }
                i++;
                if (parser->numbuf.len == 4)
                {
                    if (parser->handlers.boolean)
                        parser->handlers.boolean(parser->user_data, true);
                    parser->numbuf.len = 0;
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    if (top == CTX_ARRAY)
                        parser->state = ST_ARRAY_ELEM;
                    else if (top == CTX_OBJECT)
                        parser->state = ST_AFTER_COLON;
                    else
                        parser->state = ST_DONE;
                }
                continue;
            }
            else
            {
                call_error(parser, "invalid token while parsing 'true'");
                return false;
            }
        }
        break;
        case ST_FALSE:
        {
            const char *match = "alse";
            size_t have = parser->numbuf.len;
            if (have == 0)
            {
                call_error(parser, "internal true parser error");
                return false;
            }
            size_t expect_index = have - 1;
            if (expect_index >= 4)
            {
                call_error(parser, "internal true length error");
                return false;
            }

            if (c == match[expect_index])
            {
                if (!sbuf_append_char(&parser->numbuf, c))
                {
                    call_error(parser, "alloc failure");
                }
                i++;
                if (parser->numbuf.len == 5)
                {
                    if (parser->handlers.boolean)
                        parser->handlers.boolean(parser->user_data, false);
                    parser->numbuf.len = 0;
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    if (top == CTX_ARRAY)
                        parser->state = ST_ARRAY_ELEM;
                    else if (top == CTX_OBJECT)
                        parser->state = ST_AFTER_COLON;
                    else
                        parser->state = ST_DONE;
                }
                continue;
            }
            else
            {
                call_error(parser, "invalid token while parsing 'true'");
                return false;
            }
        }
        break;
        case ST_NULL:
        {
            const char *match = "ull";
            size_t have = parser->numbuf.len;
            if (have == 0)
            {
                call_error(parser, "internal true parser error");
                return false;
            }
            size_t expect_index = have - 1;
            if (expect_index >= 3)
            {
                call_error(parser, "internal true length error");
                return false;
            }

            if (c == match[expect_index])
            {
                if (!sbuf_append_char(&parser->numbuf, c))
                {
                    call_error(parser, "alloc failure");
                }
                i++;
                if (parser->numbuf.len == 4)
                {
                    if (parser->handlers.boolean)
                        parser->handlers.boolean(parser->user_data, true);
                    parser->numbuf.len = 0;
                    ctx_type_t top = ctx_stack_top(&parser->stack);
                    if (top == CTX_ARRAY)
                        parser->state = ST_ARRAY_ELEM;
                    else if (top == CTX_OBJECT)
                        parser->state = ST_AFTER_COLON;
                    else
                        parser->state = ST_DONE;
                }
                continue;
            }
            else
            {
                call_error(parser, "invalid token while parsing 'true'");
                return false;
            }
        }
        break;
        default:
            call_error(parser, "unexpected parser state");
            return false;
        }

        // parse \uXXXX
        if (parser->u_remaining > 0 && parser->state == ST_STRING)
        {
            int needed = parser->u_remaining;
            size_t j = i;
            while (needed > 0 && j < (int)buflen)
            {
                int v = hex_val(buf[j]);
                if (v < 0)
                {
                    call_error(parser, "invalid hex in \\u espace");
                    return false;
                }
                parser->u_value = (uint16_t)((parser->u_value << 4) | (uint16_t)v);
                needed--;
                j++;
                parser->position++;
            }
            size_t consumed = j - (int)i;
            i += consumed;
            parser->u_remaining = needed;
            if (parser->u_remaining == 0)
            {
                uint16_t cu = parser->u_value;
                parser->u_value = 0;
                // handle surrogate pairs
                if (0xd800 <= cu && cu <= 0xdbff)
                {
                    // high surrogate
                    parser->expecting_surrogate = 1;
                    parser->u_remaining = 0;

                    parser->u_value = cu;
                    // this can fail across chunk boundries
                }
                else if (0xdc00 <= cu && cu <= 0xdfff)
                {
                    // low surrogate
                    call_error(parser, "unexpected low surrogate");
                    return false;
                }
                else
                {
                    if (!sbuf_append_utf8_codepoint(&parser->strbuf, (uint32_t)cu))
                    {
                        call_error(parser, "alloc failure");
                        return false;
                    }
                }
            }
        }
    }

    if (is_final)
    {
        if (parser->state == ST_DONE || parser->stack.len == 0)
        {
            return true;
        }
        else
        {
            call_error(parser, "unexpected end of input");
            return false;
        }
    }

    return true;
}

bool json_sax_parse_file(json_sax_parser_t *parser, FILE *f){
    char buf[READ_BUF_SIZE];
    while(1){
        size_t n = fread(buf,1,READ_BUF_SIZE,f);
        if(ferror(f)){
            call_error(parser, "read error");
            return false;
        }
        int is_final = feof(f);
        if(!process_chunk(parser, buf, n, is_final)) return false;
        if(is_final) break;
    }

    return true;
}

bool parse_file_with_sax(const char *filename, const json_sax_handler_t *h, void *ud){
    FILE *f = NULL;
    if(!filename)  f = stdin;
    else {
        f = fopen(filename, "rb");
        if(!f){
            perror("fopen");
            return false;
        }
    }
    json_sax_parser_t parser;
    if(!parser_init(&parser, h, ud)){
        if(f&& f != stdin) fclose(f);
        return false;
    }
    bool rc = json_sax_parse_file(&parser, f);
    parser_free(&parser);
    if(f && f != stdin) fclose(f);
    return rc;
}
