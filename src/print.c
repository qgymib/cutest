#include "runtime.h"
#include "print.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)

#include <windows.h>
#include <io.h>
#define fileno(x)                        _fileno(x)
#define isatty(x)                        _isatty(x)

static WORD _get_color_attribute(cutest_print_color_t color)
{
    switch (color)
    {
    case CUTEST_PRINT_COLOR_RED:
        return FOREGROUND_RED;
    case CUTEST_PRINT_COLOR_GREEN:
        return FOREGROUND_GREEN;
    case CUTEST_PRINT_COLOR_YELLOW:
        return FOREGROUND_RED | FOREGROUND_GREEN;
    default:
        return 0;
    }
}

static int _get_bit_offset(WORD color_mask)
{
    if (color_mask == 0) return 0;

    int bitOffset = 0;
    while ((color_mask & 1) == 0)
    {
        color_mask >>= 1;
        ++bitOffset;
    }
    return bitOffset;
}

static WORD _get_new_color(cutest_print_color_t color, WORD old_color_attrs)
{
    // Let's reuse the BG
    static const WORD background_mask = BACKGROUND_BLUE | BACKGROUND_GREEN |
        BACKGROUND_RED | BACKGROUND_INTENSITY;
    static const WORD foreground_mask = FOREGROUND_BLUE | FOREGROUND_GREEN |
        FOREGROUND_RED | FOREGROUND_INTENSITY;
    const WORD existing_bg = old_color_attrs & background_mask;

    WORD new_color =
        _get_color_attribute(color) | existing_bg | FOREGROUND_INTENSITY;
    const int bg_bitOffset = _get_bit_offset(background_mask);
    const int fg_bitOffset = _get_bit_offset(foreground_mask);

    if (((new_color & background_mask) >> bg_bitOffset) ==
        ((new_color & foreground_mask) >> fg_bitOffset))
    {
        new_color ^= FOREGROUND_INTENSITY;  // invert intensity
    }
    return new_color;
}

#else

#include <pthread.h>
#include <unistd.h>

typedef struct color_printf_ctx
{
    int terminal_color_support;
}color_printf_ctx_t;

static color_printf_ctx_t g_color_printf = {
    0,
};

static const char* _get_ansi_color_code_fg(cutest_print_color_t color)
{
    switch (color)
    {
        case CUTEST_PRINT_COLOR_RED:
            return "31";
        case CUTEST_PRINT_COLOR_GREEN:
            return "32";
        case CUTEST_PRINT_COLOR_YELLOW:
            return "33";
        default:
            break;
    }

    return NULL;
}

static void _initlize_color_unix(void)
{
    static const char* support_color_term_list[] = {
            "xterm",
            "xterm-color",
            "xterm-256color",
            "screen",
            "screen-256color",
            "tmux",
            "tmux-256color",
            "rxvt-unicode",
            "rxvt-unicode-256color",
            "linux",
            "cygwin",
    };

    /* On non-Windows platforms, we rely on the TERM variable. */
    const char* term = getenv("TERM");
    if (term == NULL)
    {
        return;
    }

    size_t i;
    for (i = 0; i < ARRAY_SIZE(support_color_term_list); i++)
    {
        if (strcmp(term, support_color_term_list[i]) == 0)
        {
            g_color_printf.terminal_color_support = 1;
            break;
        }
    }
}

#endif

static FILE* _get_logfile(void)
{
    return cutest_runtime.io.f_out != NULL ? cutest_runtime.io.f_out : stdout;
}

static const char* _cutest_get_log_level_str(cutest_log_level_t level)
{
    switch (level)
    {
    case CUTEST_LOG_DEBUG:
        return "D";
    case CUTEST_LOG_INFO:
        return "I";
    case CUTEST_LOG_WARN:
        return "W";
    case CUTEST_LOG_ERROR:
        return "E";
    case CUTEST_LOG_FATAL:
        return "F";
    default:
        break;
    }
    return "U";
}

static void _cutest_default_log(cutest_log_meta_t* info, const char* fmt, va_list ap, FILE* out)
{
    cutest_color_fprintf(CUTEST_PRINT_COLOR_DEFAULT, out, "[%s %s:%d] ",
        _cutest_get_log_level_str(info->leve), info->file, info->line);
    cutest_color_vfprintf(CUTEST_PRINT_COLOR_DEFAULT, out, fmt, ap);
    cutest_color_fprintf(CUTEST_PRINT_COLOR_DEFAULT, out, "\n");
}

static int _should_use_color(int is_tty)
{
#if defined(_WIN32)
    /**
     * On Windows the TERM variable is usually not set, but the console there
     * does support colors.
     */
    return is_tty;
#else
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    pthread_once(&once_control, _initlize_color_unix);

    return is_tty && g_color_printf.terminal_color_support;
#endif
}

void cutest_log(cutest_log_meta_t* info, const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    if (cutest_runtime.hook->on_log_print != NULL)
    {
        cutest_runtime.hook->on_log_print(info, fmt, ap, _get_logfile());
    }
    else
    {
        _cutest_default_log(info, fmt, ap, _get_logfile());
    }
    va_end(ap);
}

int cutest_color_vfprintf(cutest_print_color_t color, FILE* stream, const char* fmt, va_list ap)
{
    assert(stream != NULL);

    int stream_fd = fileno(stream);
    if (!_should_use_color(isatty(stream_fd)) || (color == CUTEST_PRINT_COLOR_DEFAULT))
    {
        return vfprintf(stream, fmt, ap);
    }

    int ret;
#if defined(_WIN32)
    const HANDLE stdout_handle = (HANDLE)_get_osfhandle(stream_fd);

    // Gets the current text color.
    CONSOLE_SCREEN_BUFFER_INFO buffer_info;
    GetConsoleScreenBufferInfo(stdout_handle, &buffer_info);
    const WORD old_color_attrs = buffer_info.wAttributes;
    const WORD new_color = _get_new_color(color, old_color_attrs);

    // We need to flush the stream buffers into the console before each
    // SetConsoleTextAttribute call lest it affect the text that is already
    // printed but has not yet reached the console.
    fflush(stream);
    SetConsoleTextAttribute(stdout_handle, new_color);

    ret = vfprintf(stream, fmt, ap);

    fflush(stream);
    // Restores the text color.
    SetConsoleTextAttribute(stdout_handle, old_color_attrs);
#else
    fprintf(stream, "\033[0;%sm", _get_ansi_color_code_fg(color));
    ret = vfprintf(stream, fmt, ap);
    fprintf(stream, "\033[m");  // Resets the terminal to default.
    fflush(stream);
#endif

    return ret;
}

int cutest_printf(const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(CUTEST_PRINT_COLOR_DEFAULT, _get_logfile(),
        fmt, ap);
    va_end(ap);

    return ret;
}

int cutest_color_fprintf(cutest_print_color_t color, FILE* stream, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(color, stream, fmt, ap);
    va_end(ap);

    return ret;
}

const char* cutest_pretty_file(const char* file)
{
    const char* pos = file;

    for (; *file; ++file)
    {
        if (*file == '\\' || *file == '/')
        {
            pos = file + 1;
        }
    }
    return pos;
}

int _cutest_color_printf(cutest_print_color_t color, const char* fmt, ...)
{
    int ret;
    va_list ap;

    va_start(ap, fmt);
    ret = cutest_color_vfprintf(color, _get_logfile(), fmt, ap);
    va_end(ap);

    return ret;
}
