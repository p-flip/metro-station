#ifndef MINI_SNPRINTF_H
#define MINI_SNPRINTF_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

/* Hilfsfunktion: Zahl ? ASCII */
static char* itoa_simple(uint32_t value, char *buf, int base, int is_signed) {
    char *p = buf, *p1 = buf, tmp;
    uint32_t u = value;
    if (is_signed && ((int32_t)value) < 0) {
        *p++ = '-';
        p1++;
        u = (uint32_t)(-((int32_t)value));
    }
    do {
        int digit = u % base;
        *p++ = (digit < 10 ? '0' + digit : 'a' + digit - 10);
        u /= base;
    } while (u);
    *p = '\0';
    for (--p; p1 < p; ++p1, --p) {
        tmp = *p; *p = *p1; *p1 = tmp;
    }
    return buf;
}

/**
 * mini_snprintf ersetzt snprintf, liest für jeden Platzhalter den korrekten Typ
 * und unterstützt nur %s, %c, %d, %u, %x.
 */
static int mini_snprintf(char *out, size_t size, const char *fmt, ...) {
    va_list ap;
    char *dst = out;
    const char *end = out + size - 1;
    va_start(ap, fmt);

    while (*fmt && dst < end) {
        if (*fmt != '%') {
            *dst++ = *fmt++;
            continue;
        }
        ++fmt;
        switch (*fmt) {
            case 'c': {
                int c = va_arg(ap, int);
                *dst++ = (char)c;
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char*);
                while (*s && dst < end) {
                    *dst++ = *s++;
                }
                break;
            }
            case 'd': {
                int v = va_arg(ap, int);
                char numbuf[12];
                itoa_simple((uint32_t)v, numbuf, 10, 1);
                for (char *p = numbuf; *p && dst < end; ++p) {
                    *dst++ = *p;
                }
                break;
            }
            case 'u': {
                unsigned int v = va_arg(ap, unsigned int);
                char numbuf[12];
                itoa_simple((uint32_t)v, numbuf, 10, 0);
                for (char *p = numbuf; *p && dst < end; ++p) {
                    *dst++ = *p;
                }
                break;
            }
            case 'x': {
                unsigned int v = va_arg(ap, unsigned int);
                char numbuf[12];
                itoa_simple((uint32_t)v, numbuf, 16, 0);
                for (char *p = numbuf; *p && dst < end; ++p) {
                    *dst++ = *p;
                }
                break;
            }
            default:
                /* Unbekannter Platzhalter: literales % + Zeichen */
                if (dst < end) *dst++ = '%';
                if (dst < end) *dst++ = *fmt;
        }
        ++fmt;
    }

    *dst = '\0';
    va_end(ap);
    return (int)(dst - out);
}

#endif /* MINI_SNPRINTF_H */
